# Julia startup script for graphics redirection
# This script is generated based on project configuration

# Load required packages
using Plots

# Check graphics redirection method from environment variables
const GRAPHICS_METHOD = get(ENV, "JULIA_GRAPHICS_METHOD", "file")
const GRAPHICS_DIR = get(ENV, "JULIA_GRAPHICS_DIR", "/tmp/graphics")
const SOCKET_HOST = get(ENV, "JULIA_SOCKET_HOST", "localhost")
const SOCKET_PORT = parse(Int, get(ENV, "JULIA_SOCKET_PORT", "8080"))

if GRAPHICS_METHOD == "file"
    # File-based graphics redirection
    try
        # Import the FilePlotDisplay module
        include(joinpath(@__DIR__,"graphics", "FilePlotDisplay.jl"))
        using .FilePlotDisplay

        # Activate file-based display
        FilePlotDisplay.activate!(GRAPHICS_DIR)
        println("Graphics redirection activated: File-based to directory '$GRAPHICS_DIR'")
    catch err
        @warn "Failed to activate file-based graphics redirection: $err"
    end

elseif GRAPHICS_METHOD == "socket"
    # TCP socket graphics redirection
    try
        # Import the SocketPlotDisplay module
        include(joinpath(@__DIR__,"graphics", "SocketPlotDisplay.jl"))
        using .SocketPlotDisplay

        # Start server and activate socket display
        SocketPlotDisplay.start_server!(SOCKET_PORT)
        SocketPlotDisplay.activate!()
        println("Graphics redirection activated: TCP socket on port $SOCKET_PORT")
    catch err
        @warn "Failed to activate socket-based graphics redirection: $err"
    end

else
    @warn "Unknown graphics method: $GRAPHICS_METHOD. Supported: 'file', 'socket'"
end
