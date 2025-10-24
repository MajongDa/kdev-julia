using Sockets, Plots

function send_image(io, plt)
    data = sprint(io -> show(io, MIME("image/png"), plt))
    n = length(data)
    write(io, UInt64(n))
    write(io, data)
    flush(io)
    @info "Sent $n bytes"
end

function test_send()
    sock = connect("127.0.0.1", 9000)
    @info "Connected to Python viewer"
    plt = plot(rand(10))
    send_image(sock, plt)
    close(sock)
end

test_send()