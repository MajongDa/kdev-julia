#!/usr/bin/env julia
"""
Real-time sine wave plotting server example.

This example demonstrates how to create a server that continuously sends
updated sine wave plots to connected clients.
"""

include("../SocketPlotDisplay.jl")
using Plots
using .SocketPlotDisplay

# Start the server
println("Starting SocketPlotDisplay server on port 9000...")
SocketPlotDisplay.start_server!(9000)
SocketPlotDisplay.activate!()

println("Server is running. Connect clients to see real-time plots.")
println("Press Ctrl+C to stop.")

try
    t = 0.0
    while true
        # Generate sine wave with phase shift
        x = range(0, 4π, length=100)
        y = sin.(x .+ t)

        # Create plot
        p = plot(x, y,
                 title="Real-time Sine Wave (t = $(round(t, digits=2)))",
                 xlabel="x",
                 ylabel="sin(x + t)",
                 ylim=(-1.5, 1.5),
                 legend=false)

        # Display (will be sent to all connected clients)
        display(p)

        # Update phase
        t += 0.1

        # Small delay for smooth animation
        sleep(0.1)
    end
catch e
    if e isa InterruptException
        println("\nStopping server...")
    else
        @error "Error: $e"
    end
finally
    SocketPlotDisplay.deactivate!()
    SocketPlotDisplay.stop_server!()
    println("Server stopped.")
end