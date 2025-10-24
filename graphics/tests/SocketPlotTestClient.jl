include("../SocketPlotDisplay.jl")
using .SocketPlotDisplay, Plots

SocketPlotDisplay.connect_client!("127.0.0.1", 9000)
SocketPlotDisplay.activate!()

plot(rand(30))
# ...
SocketPlotDisplay.deactivate!()
SocketPlotDisplay.disconnect_all!()
