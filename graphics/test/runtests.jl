include("../src/SocketPlotDisplay.jl")
using Test
using Sockets

@testset "SocketPlotDisplay.jl" begin
    @testset "Basic functionality" begin
        # Test that module loads
        @test typeof(SocketPlotDisplay) == Module

        # Test that exports exist
        @test isdefined(SocketPlotDisplay, :start_server!)
        @test isdefined(SocketPlotDisplay, :stop_server!)
        @test isdefined(SocketPlotDisplay, :connect_client!)
        @test isdefined(SocketPlotDisplay, :disconnect_all!)
        @test isdefined(SocketPlotDisplay, :activate!)
        @test isdefined(SocketPlotDisplay, :deactivate!)
        @test isdefined(SocketPlotDisplay, :active_display)
    end

    @testset "Input validation" begin
        # Test invalid port ranges
        @test_throws ArgumentError SocketPlotDisplay.start_server!(1023)
        @test_throws ArgumentError SocketPlotDisplay.start_server!(65536)

        # Test invalid connect_client! parameters
        @test_throws ArgumentError SocketPlotDisplay.connect_client!("", 9000)
        @test_throws ArgumentError SocketPlotDisplay.connect_client!("localhost", 0)
        @test_throws ArgumentError SocketPlotDisplay.connect_client!("localhost", 9000; attempts=0)
        @test_throws ArgumentError SocketPlotDisplay.connect_client!("localhost", 9000; delay=-1.0)
    end

    @testset "Display creation" begin
        # Test display creation
        sd = SocketPlotDisplay._make_socket_display()
        @test typeof(sd) == SocketPlotDisplay.SocketDisplay
        @test isempty(sd.clients)
        @test sd.server === nothing
        @test sd.accept_task === nothing
        @test length(sd.fmt_priority) > 0
    end

    @testset "Payload size limits" begin
        # Test that large payloads are rejected
        sock = SocketPlotDisplay.SocketDisplay([], ReentrantLock(), nothing, nothing, [MIME("image/png")])
        large_data = Vector{UInt8}(undef, 60_000_000)  # 60MB > 50MB limit
        @test !SocketPlotDisplay._send_to_socket(sock.clients[0], large_data)
    end

    @testset "Server lifecycle" begin
        # Test server start/stop (without actual network)
        # Note: This is a basic test; integration tests handle full networking
        @test SocketPlotDisplay.active_display() === nothing

        # Clean up any existing state
        SocketPlotDisplay._socket_display_ref[] = nothing
    end
end