module MakiePlotDisplay

using Makie
using CairoMakie
using SHA
using Dates

export activate!, deactivate!, active_screen

const _active_screen = Ref{Union{MakieScreen, Nothing}}(nothing)

mutable struct MakieScreen <: Makie.MakieScreen
    output_dir::String
    format::Symbol
    px_per_unit::Float64
end

Base.size(screen::MakieScreen) = (800, 600)
Base.empty!(screen::MakieScreen) = nothing

function _generate_hash()
    return bytes2hex(sha1(string(Dates.now(), rand(1000:9999))))[1:8]
end

function _render_figure(fig::Union{Makie.Figure, Makie.FigureAxisPlot}, screen::MakieScreen)
    ext = screen.format == :svg ? "svg" : 
          screen.format == :png ? "png" : "pdf"
    
    # Use temp file approach
    temp_file = tempname() * ".$ext"
    
    Makie.save(temp_file, fig; px_per_unit=screen.px_per_unit)
    
    data = read(temp_file)
    rm(temp_file)
    
    hash = _generate_hash()
    filepath = joinpath(screen.output_dir, "makie_$(hash).$(ext)")
    
    write(filepath, data)
    
    @info "MakiePlotDisplay: saved figure -> $(filepath)"
    
    return filepath
end

function activate!(output_dir::String; format=:svg, px_per_unit=2.0)
    # Ensure directory exists
    if !isdir(output_dir)
        mkpath(output_dir)
    end
    
    screen = MakieScreen(output_dir, format, px_per_unit)
    _active_screen[] = screen
    
    return screen
end

function deactivate!()
    _active_screen[] = nothing
end

active_screen() = _active_screen[]

const _original_display = Makie.display

function Makie.display(figlike::Union{Makie.Figure, Makie.FigureAxisPlot})
    screen = _active_screen[]
    if screen === nothing
        return _original_display(figlike)
    end
    return _render_figure(figlike, screen)
end

end # module
