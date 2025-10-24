include("../src/SocketPlotDisplay.jl")
using Test
using Sockets

@testset "Integration Tests" begin
    @testset "Server-Client Communication" begin
        # Clean up any existing state
        SocketPlotDisplay._socket_display_ref[] = nothing

        # Start server on a test port
        port = 9999
        server_task = @async begin
            try
                SocketPlotDisplay.start_server!(port)
                sleep(10)  # Keep server running for test
            catch e
                @error "Server error: $e"
            end
        end

        # Give server time to start
        sleep(1)

        # Test client connection
        client_sock = SocketPlotDisplay.connect_client!("127.0.0.1", port)
        @test client_sock !== nothing

        # Test that server has the client
        sd = SocketPlotDisplay.active_display()
        @test sd !== nothing
        # Note: Due to async nature, there might be more clients from previous tests
        @test length(sd.clients) >= 1

        # Test sending data (mock display)
        test_data = "Hello World"
        io = IOBuffer()
        show(io, "text/plain", test_data)
        data = take!(io)

        # This should work without errors
        Base.display(sd, test_data)

        # Clean up
        SocketPlotDisplay.disconnect_all!()
        SocketPlotDisplay.stop_server!()

        # Wait for server task to finish
        try
            wait(server_task)
        catch
            # Expected if server was stopped
        end
    end

    @testset "Multiple Clients" begin
        # Clean up any existing state
        SocketPlotDisplay._socket_display_ref[] = nothing

        port = 9998
        server_task = @async SocketPlotDisplay.start_server!(port)

        sleep(1)

        # Connect multiple clients
        clients = []
        for i in 1:3
            sock = SocketPlotDisplay.connect_client!("127.0.0.1", port)
            @test sock !== nothing
            push!(clients, sock)
        end

        # Check server has all clients (may have more from previous tests)
        sd = SocketPlotDisplay.active_display()
        @test length(sd.clients) >= 3

        # Clean up
        SocketPlotDisplay.disconnect_all!()
        SocketPlotDisplay.stop_server!()
    end

    @testset "Error Handling" begin
        # Test connecting to non-existent server
        sock = SocketPlotDisplay.connect_client!("127.0.0.1", 9997; attempts=2, delay=0.1)
        @test sock === nothing

        # Test server on invalid port
        @test_throws ArgumentError SocketPlotDisplay.start_server!(80)  # Privileged port
    end
end