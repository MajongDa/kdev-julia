# GraphicsLoader.jl - Lazy activation using @eval and display stack

module GraphicsLoader

using SHA
using Dates

_plots_activated = false
_makie_activated = false

function _get_dir()
    get(ENV, "JULIA_GRAPHICS_DIR", "/tmp/graphics")
end

# Activate Plots by pushing FileDisplay to the display stack
function activate_plots()
    global _plots_activated
    _plots_activated && return
    
    @eval begin
        using Plots
        include(joinpath(@__DIR__, "FilePlotDisplay.jl"))
        using .FilePlotDisplay
        
        dir = get(ENV, "JULIA_GRAPHICS_DIR", "/tmp/graphics")
        isdir(dir) || mkpath(dir)
        FilePlotDisplay.activate!(dir)
    end
    
    global _plots_activated = true
    println("Plots graphics support activated")
end

# Activate Makie by pushing to display stack  
function activate_makie()
    global _makie_activated
    _makie_activated && return
    
    @eval begin
        using CairoMakie
        include(joinpath(@__DIR__, "MakiePlotDisplay.jl"))
        using .MakiePlotDisplay
        
        dir = get(ENV, "JULIA_GRAPHICS_DIR", "/tmp/graphics")
        isdir(dir) || mkpath(dir)
        MakiePlotDisplay.activate!(dir; format=:svg)
    end
    
    global _makie_activated = true
    println("Makie graphics support activated")
end

# Try to activate based on type of x
function try_activate(x)
    # Check for Plots types
    if hasproperty(Main, :Plots) && !_plots_activated
        try
            if x isa Main.Plots.Plot
                activate_plots()
            end
        catch e
        end
    end
    
    # Check for Makie types  
    if (hasproperty(Main, :CairoMakie) || hasproperty(Main, :Makie)) && !_makie_activated
        try
            if isdefined(Main, :Makie)
                Makie = Main.Makie
                if x isa Makie.Figure || x isa Makie.FigureAxisPlot || x isa Makie.Scene
                    activate_makie()
                end
            end
        catch e
        end
    end
end

end # module
