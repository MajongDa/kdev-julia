"""
# SocketPlotDisplay.jl

A Julia package for real-time plotting and visualization over network sockets.

This package enables sending plot data from Julia to remote clients over TCP sockets,
supporting both server and client modes. It works with popular plotting backends like
Plots.jl and Makie.jl.

## Features

- **Real-time plotting**: Send plots to connected clients instantly
- **Multiple backends**: Support for Plots.jl and Makie.jl
- **Flexible serialization**: PNG, SVG, and other MIME types
- **Security**: Payload size limits and input validation
- **Error handling**: Robust connection management and error recovery

## Quick Start

```julia
using SocketPlotDisplay, Plots

# Start server
start_server!(9000)
activate!()

# Create and display plots
plot(sin.(0:0.1:2π))
```

## Backends

### Plots.jl
```julia
using Plots
plot(rand(10))
```

### Makie.jl
```julia
using Makie
using CairoMakie
f = Figure()
lines(f[1,1], 1:10, rand(10))
display(f)
```
"""
module SocketPlotDisplay

using Sockets
using Dates

export start_server!, stop_server!, connect_client!, disconnect_all!, activate!, deactivate!, active_display

# Optional Makie support
const HAS_MAKIE = Ref(false)
function __init__()
    try
        @eval using Makie
        HAS_MAKIE[] = true
    catch
        HAS_MAKIE[] = false
    end
end

const FRAME_HEADER_BYTES = 8

mutable struct SocketDisplay <: AbstractDisplay
    clients::Vector{Sockets.TCPSocket}
    lock::ReentrantLock
    server::Union{Sockets.TCPServer, Nothing}
    accept_task::Union{Task, Nothing}
    fmt_priority::Vector{MIME}
end

const _socket_display_ref = Ref{Union{SocketDisplay, Nothing}}(nothing)

# helper factory
function _make_socket_display(fmt_priority=(MIME("image/png"), MIME("image/svg+xml")))
    # Add Makie-specific MIME if Makie is available
    if HAS_MAKIE[]
        fmt_priority = (fmt_priority..., MIME("image/png"))  # Ensure PNG is included for Makie
    end
    return SocketDisplay(Vector{Sockets.TCPSocket}(), ReentrantLock(), nothing, nothing, collect(fmt_priority))
end

# framing: send 8-byte big-endian length then payload
function _send_to_socket(sock::Sockets.TCPSocket, bytes::Vector{UInt8})
    len = UInt64(length(bytes))
    # Security check: limit payload size to prevent DoS
    if len > 50_000_000  # 50MB limit
        @info "SocketDisplay: payload too large ($len bytes), rejecting"
        return true
    end
    hdr = Vector{UInt8}(undef, FRAME_HEADER_BYTES)
    for i in 1:FRAME_HEADER_BYTES
        shift = (FRAME_HEADER_BYTES - i)*8
        hdr[i] = UInt8((len >> shift) & 0xFF)
    end
    try
        write(sock, hdr)
        write(sock, bytes)
        flush(sock)
        return true
    catch err
        @warn "SocketDisplay: error sending to client: $err"
        return false
    end
end

# main display: serialize to preferred MIME and send to all clients
function Base.display(d::SocketDisplay, x)
    # Validate input to prevent malicious data
    if x === nothing
        @warn "SocketDisplay: cannot display nothing"
        return
    end

    # Special handling for Makie figures if Makie is available
    if HAS_MAKIE[] && x isa Makie.Figure
        try
            # Convert Makie figure to PNG
            io = IOBuffer()
            Makie.save(io, x, Makie.PNG)
            data = take!(io)
            lock(d.lock) do
                for sock in copy(d.clients)
                    ok = _send_to_socket(sock, data)
                    if !ok
                        @warn "SocketDisplay: failed to send data to client $sock"
                        try close(sock) catch end
                        filter!(s->s!==sock, d.clients)
                    end
                end
            end
            return
        catch err
            @warn "SocketDisplay: error converting Makie figure: $err"
            # Fall through to general MIME handling
        end
    end

    for m in d.fmt_priority
        # используем showable из Base.Multimedia
        if showable(m, x)
            io = IOBuffer()
            try
                show(io, m, x)
            catch err
                @warn "SocketDisplay: error serializing MIME $m: $err"
                continue
            end
            data = Vector{UInt8}(take!(io))
            lock(d.lock) do
                for sock in copy(d.clients)
                    ok = _send_to_socket(sock, data)
                    if !ok
                        @warn "SocketDisplay: failed to send data to client $sock"
                        try close(sock) catch end
                        filter!(s->s!==sock, d.clients)
                    end
                end
            end
            return
        end
    end
    # fallback: текстовый вывод
    try
        show(stdout, "text/plain", x)
    catch
        @warn "SocketDisplay: cannot display object (no suitable MIME)"
    end
end

# ----------------- SERVER -----------------
"""
    start_server!(port; fmt_priority=(MIME("image/png"), MIME("image/svg+xml")))

Start a TCP server that accepts clients. Connected clients will receive frames
(8-byte big-endian length + payload). The server listens on all interfaces.

# Arguments
- `port::Integer`: The port number to listen on (1024-65535).

# Keywords
- `fmt_priority::Tuple`: Priority order of MIME types for serialization.

# Returns
- The `SocketDisplay` instance.

# Throws
- `ArgumentError`: If port is not in valid range.

# Examples
```julia
using SocketPlotDisplay
sd = start_server!(9000)
```
"""
function start_server!(port::Integer; fmt_priority=(MIME("image/png"), MIME("image/svg+xml")))
    # Validate port number
    if !(1024 <= port <= 65535)
        throw(ArgumentError("Port must be between 1024 and 65535"))
    end
    # если уже запущен — вернуть ссылку
    if _socket_display_ref[] !== nothing && _socket_display_ref[].server !== nothing
        @warn "SocketPlotDisplay: server already running"
        return _socket_display_ref[]
    end

    # Попытка открыть TCPServer (listen на всех интерфейсах)
    server = try
        listen(port)
    catch err
        @error "SocketPlotDisplay: cannot bind/listen on port $port -> $err"
        rethrow(err)
    end

    sd = _make_socket_display(fmt_priority)
    sd.server = server   # тип согласован: TCPServer

    # вычисляем строковое представление адреса здесь, чтобы избежать замыканий/undef в логах
    srvaddr = string(server)

    sd.accept_task = @async begin
        try
            @info "SocketPlotDisplay: listening on $srvaddr"
            while true
                client = accept(server)   # возвращает TCPSocket
                @info "SocketPlotDisplay: client connected $(client)"
                lock(sd.lock) do
                    push!(sd.clients, client)
                end
            end
        catch err
            @info "SocketPlotDisplay: accept loop ended: $err"
        finally
            # при выходе закрываем клиентов
            disconnect_all!()
        end
    end

    _socket_display_ref[] = sd
    return sd
end

"""
    stop_server!()

Stop the running TCP server and disconnect all clients.

# Examples
```julia
stop_server!()
```
"""
function stop_server!()
    sd = _socket_display_ref[]
    if sd === nothing
        @warn "SocketPlotDisplay: server not running"
        return
    end
    # закрываем сервер (это разорвёт accept)
    try
        if sd.server !== nothing
            try close(sd.server) catch end
        end
    catch err
        @warn "SocketPlotDisplay: error closing server: $err"
    end
    # закрываем клиентов
    disconnect_all!()
    sd.server = nothing
    sd.accept_task = nothing
    _socket_display_ref[] = nothing
    return
end

# ----------------- CLIENT (Julia как клиент) -----------------
"""
    connect_client!(host, port; fmt_priority=(MIME("image/png"), MIME("image/svg+xml")), attempts=5, delay=0.5)

Attempt to connect to an external receiver. On success, adds TCPSocket to clients.

# Arguments
- `host::AbstractString`: The hostname or IP address (non-empty).
- `port::Integer`: The port number (1-65535).

# Keywords
- `fmt_priority::Tuple`: Priority order of MIME types.
- `attempts::Int`: Number of connection attempts (≥1).
- `delay::Float64`: Delay between attempts in seconds (≥0).

# Returns
- The connected `TCPSocket` or `nothing` on failure.

# Throws
- `ArgumentError`: If inputs are invalid.

# Examples
```julia
using SocketPlotDisplay
sock = connect_client!("127.0.0.1", 9000)
```
"""
function connect_client!(host::AbstractString, port::Integer; fmt_priority=(MIME("image/png"), MIME("image/svg+xml")), attempts::Int=5, delay::Float64=0.5)
    # Validate inputs
    if isempty(host)
        throw(ArgumentError("Host cannot be empty"))
    end
    if !(1 <= port <= 65535)
        throw(ArgumentError("Port must be between 1 and 65535"))
    end
    if attempts < 1
        throw(ArgumentError("Attempts must be at least 1"))
    end
    if delay < 0
        throw(ArgumentError("Delay must be non-negative"))
    end

    sd = _socket_display_ref[]
    if sd === nothing
        sd = _make_socket_display(fmt_priority)
        _socket_display_ref[] = sd
    end

    last_err = nothing
    for i in 1:attempts
        try
            sock = connect(host, port)
            lock(sd.lock) do
                push!(sd.clients, sock)
            end
            @info "SocketPlotDisplay: connected to $host:$port"
            return sock
        catch err
            last_err = err
            @warn "SocketPlotDisplay: connect attempt $i failed: $err — retrying in $(delay) sec..."
            sleep(delay)
        end
    end

    @error "SocketPlotDisplay: cannot connect to $host:$port after $attempts attempts -> $last_err"
    return nothing
end

"""
    disconnect_all!()

Disconnect and close all client connections.

# Examples
```julia
disconnect_all!()
```
"""
function disconnect_all!()
    sd = _socket_display_ref[]
    if sd === nothing
        return
    end
    lock(sd.lock) do
        for c in sd.clients
            try close(c) catch end
        end
        empty!(sd.clients)
    end
    return
end

# ----------------- DISPLAY stack helpers -----------------
"""
    activate!()

Activate the socket display by pushing it to the display stack.

# Examples
```julia
activate!()
plot(rand(10))  # Will be sent to connected clients
```
"""
function activate!()
    sd = _socket_display_ref[]
    if sd === nothing
        sd = _make_socket_display()
        _socket_display_ref[] = sd
    end
    pushdisplay(sd)
    return sd
end

"""
    deactivate!()

Deactivate the socket display by popping it from the display stack.

# Examples
```julia
deactivate!()
```
"""
function deactivate!()
    sd = _socket_display_ref[]
    if sd === nothing
        @warn "SocketPlotDisplay: not active"
        return
    end
    try
        popdisplay()
    catch err
        @warn "SocketPlotDisplay: popdisplay failed: $err"
    end
end

"""
    active_display()

Get the currently active socket display instance.

# Returns
- The current `SocketDisplay` instance or `nothing` if none is active.
"""
active_display() = _socket_display_ref[]

end # module
