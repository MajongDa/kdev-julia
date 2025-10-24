include("../SocketPlotDisplay.jl")
using .SocketPlotDisplay, Plots

# Запустим сервер на localhost:9000
SocketPlotDisplay.start_server!(9000)   # Julia теперь слушает
# Сделаем дисплей активным
# теперь ждем подключения клиента (Qt/Python)
SocketPlotDisplay.activate!()

# Теперь любой plot -> отсылается всем подключённым клиентам в виде PNG (8-byte len + bytes)
plot(rand(20))

# Чтобы остановить:
# SocketPlotDisplay.deactivate!()
# SocketPlotDisplay.stop_server!()
