# Julia startup script for graphics redirection
# Loads both Plots and Makie support

const GRAPHICS_METHOD = get(ENV, "JULIA_GRAPHICS_METHOD", "file")
const GRAPHICS_DIR = get(ENV, "JULIA_GRAPHICS_DIR", "/tmp/graphics")
const SOCKET_HOST = get(ENV, "JULIA_SOCKET_HOST", "localhost")
const SOCKET_PORT = parse(Int, get(ENV, "JULIA_SOCKET_PORT", "8080"))

# Ensure directory exists
if GRAPHICS_METHOD == "file"
    if !isdir(GRAPHICS_DIR)
        mkpath(GRAPHICS_DIR)
    end
end

if GRAPHICS_METHOD == "file"
    try
        # Load Plots support (for Plots.jl users)
        include(joinpath(@__DIR__, "graphics", "FilePlotDisplay.jl"))
        using .FilePlotDisplay
        FilePlotDisplay.activate!(GRAPHICS_DIR)
        
        # Load Makie support (for CairoMakie.jl users)
        include(joinpath(@__DIR__, "graphics", "MakiePlotDisplay.jl"))
        using .MakiePlotDisplay
        MakiePlotDisplay.activate!(GRAPHICS_DIR; format=:svg)
        
        println("Graphics: Plots and Makie ready")
    catch err
        @warn "Graphics init failed: $err"
    end

elseif GRAPHICS_METHOD == "socket"
    try
        include(joinpath(@__DIR__, "graphics", "SocketPlotDisplay.jl"))
        using .SocketPlotDisplay
        SocketPlotDisplay.connect_client!(SOCKET_HOST, SOCKET_PORT)
        SocketPlotDisplay.activate!()
        println("Socket graphics ready")
    catch err
        @warn "Socket failed: $err"
    end
else
    @warn "Unknown: $GRAPHICS_METHOD"
end
