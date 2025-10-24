#!/usr/bin/env julia
"""
Client example for SocketPlotDisplay.

This example shows how to connect to a SocketPlotDisplay server
and receive plot data.
"""

using SocketPlotDisplay
using Sockets

# Connect to server
println("Connecting to server at 127.0.0.1:9000...")
sock = connect_client!("127.0.0.1", 9000)

if sock === nothing
    println("Failed to connect to server. Make sure the server is running.")
    exit(1)
end

println("Connected! Activating display...")
activate!()

println("Waiting for plot data... Press Ctrl+C to exit.")

try
    while true
        # The display is active, so any plots from the server will be shown
        # In a real application, you might want to save received data
        sleep(1)
    end
catch e
    if e isa InterruptException
        println("\nDisconnecting...")
    else
        @error "Error: $e"
    end
finally
    deactivate!()
    disconnect_all!()
    println("Disconnected.")
end