#!/usr/bin/env julia
"""
Simple plotting example.
This example demonstrates basic usage of SocketPlotDisplay with Plots.jl.
"""

include("../SocketPlotDisplay.jl")
using Plots
using .SocketPlotDisplay

# Start server
println("Starting server on port 9000...")
start_server!(9000)
activate!()

println("Creating some example plots...")

# Simple line plot
x = 1:10
y = rand(10)
p1 = plot(x, y, title="Random Line Plot", xlabel="x", ylabel="y")
display(p1)
sleep(1)

# Scatter plot
x2 = rand(50)
y2 = rand(50)
p2 = scatter(x2, y2, title="Random Scatter Plot", xlabel="x", ylabel="y")
display(p2)
sleep(1)

# Histogram
data = randn(1000)
p3 = histogram(data, title="Normal Distribution", xlabel="value", ylabel="frequency")
display(p3)
sleep(1)

# Multiple subplots
p4 = plot(layout=(2,2))
plot!(p4[1], sin.(0:0.1:2π))
plot!(p4[2], cos.(0:0.1:2π))
histogram!(p4[3], randn(100))
scatter!(p4[4], rand(20), rand(20))
display(p4)

println("Plots sent! Server will continue running until stopped.")
println("Connect clients to see the plots.")

# Keep server running
try
    while true
        sleep(1)
    end
catch e
    if e isa InterruptException
        println("\nStopping server...")
    end
finally
    deactivate!()
    stop_server!()
    println("Server stopped.")
end