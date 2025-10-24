include("../FilePlotDisplay.jl")
using .FilePlotDisplay, Plots

# Активируем (сохранение в /tmp/julia_plots)
FilePlotDisplay.activate!("/tmp/julia_plots"; prefix="myplot", timestamp_in_name=true)

plot(rand(10))   # автоматически сохранится как PNG (если backend поддерживает)
# plot(...) снова -> новый файл
plot(rand(10))
FilePlotDisplay.deactivate!()

# Проверяем, что файлы созданы
println("Files in /tmp/julia_plots:")
for file in readdir("/tmp/julia_plots")
    println(file)
end
