#!/usr/bin/env julia
"""
Makie 3D plotting example.

This example demonstrates how to use SocketPlotDisplay with Makie.jl
for real-time 3D visualization over network sockets.
"""

include("../SocketPlotDisplay.jl")

# Check if Makie is available
if !SocketPlotDisplay.HAS_MAKIE[]
    error("Makie.jl is not available. Install it with: using Pkg; Pkg.add(\"Makie\")")
end

using Makie
using .SocketPlotDisplay

# Start the server
println("Starting SocketPlotDisplay server on port 9001...")
start_server!(9001)
activate!()

println("Server is running. Connect clients to see 3D plots.")
println("Press Ctrl+C to stop.")

try
    t = 0.0
    while true
        # Create 3D surface plot
        x = range(-2, 2, length=30)
        y = range(-2, 2, length=30)
        z = [sin(x[i] + t) * cos(y[j] + t) for i in 1:length(x), j in 1:length(y)]

        # Create Makie figure
        fig = Figure()
        ax = Axis3(fig[1,1],
                   title="Real-time 3D Surface (t = $(round(t, digits=2)))")
        surface!(ax, x, y, z, colormap=:viridis)

        # Display (will be sent to all connected clients as PNG)
        display(fig)

        # Update time
        t += 0.2

        # Delay for animation
        sleep(0.2)
    end
catch e
    if e isa InterruptException
        println("\nStopping server...")
    else
        @error "Error: $e"
    end
finally
    deactivate!()
    stop_server!()
    println("Server stopped.")
end