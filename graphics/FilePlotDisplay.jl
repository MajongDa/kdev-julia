# FilePlotDisplay.jl - File-based display for Plots.jl
# Works with Plots when loaded at top-level

module FilePlotDisplay

using Dates
using Base.Threads: @threads
using Plots
export activate!, deactivate!, active_display, default_format_priority, supported_mimes, has_plots, has_makie

# Optional Makie support (Plots is required)
const HAS_MAKIE = Ref(false)
function __init__()
    try
        @eval using Makie
        HAS_MAKIE[] = true
    catch
        HAS_MAKIE[] = false
    end
end

has_plots() = true
has_makie() = HAS_MAKIE[]

# Supported MIME types
const SUPPORTED_MIMES = Dict(
    "image/png" => "png",
    "image/svg+xml" => "svg",
    "application/pdf" => "pdf",
    "image/jpeg" => "jpg",
)

const default_format_priority = (MIME("image/svg+xml"), MIME("image/png"))

function supported_mimes()
    return [MIME(m) for m in keys(SUPPORTED_MIMES)]
end

mutable struct FileDisplay <: AbstractDisplay
    dir::String
    prefix::String
    fmt_priority::Vector{MIME}
    counter::Base.RefValue{Int}
    lock::ReentrantLock
    atomic::Bool
    timestamp_in_name::Bool
end

const _file_display_ref = Ref{Union{FileDisplay, Nothing}}(nothing)

function _ensure_dir(dir::String)
    if !isdir(dir)
        mkpath(dir)
    end
end

function _mime_to_ext(m::MIME)
    s = string(m)
    if haskey(SUPPORTED_MIMES, s)
        return SUPPORTED_MIMES[s]
    end
    return replace(replace(s, '/' => "_"), '+' => "_")
end

function Base.display(d::FileDisplay, x)
    # Special handling for Makie figures
    if HAS_MAKIE[] && (x isa Makie.Figure || x isa Makie.FigureAxisPlot)
        try
            temp_file = joinpath(d.dir, "temp_makie_$(d.counter[]).png")
            Makie.save(temp_file, x, px_per_unit=2)
            data = read(temp_file)
            rm(temp_file)
            
            lock(d.lock) do
                d.counter[] += 1
                idx = d.counter[]
                name_parts = [d.prefix, lpad(string(idx), 4, '0')]
                if d.timestamp_in_name
                    push!(name_parts, Dates.format(now(), "yyyy-mm-dd_HHMMSS"))
                end
                fname = joinpath(d.dir, join(name_parts, "_") * ".png")
                if d.atomic
                    tmp = fname * ".tmp"
                    open(tmp, "w") do f write(f, data) end
                    mv(tmp, fname; force=true)
                else
                    open(fname, "w") do f write(f, data) end
                end
                @info "FileDisplay: saved Makie figure -> $fname"
            end
            return
        catch err
            @warn "FileDisplay: error saving Makie figure: $err"
        end
    end

    # Plots and other types
    for m in d.fmt_priority
        mime_str = string(m)
        if showable(mime_str, x)
            io = IOBuffer()
            try
                show(io, mime_str, x)
            catch err
                @warn "Error serializing in MIME $m: $err"
                continue
            end
            data = take!(io)
            lock(d.lock) do
                d.counter[] += 1
                idx = d.counter[]
                name_parts = [d.prefix, lpad(string(idx), 4, '0')]
                if d.timestamp_in_name
                    push!(name_parts, Dates.format(now(), "yyyy-mm-dd_HHMMSS"))
                end
                ext = _mime_to_ext(m)
                fname = joinpath(d.dir, join(name_parts, "_") * "." * ext)
                if d.atomic
                    tmp = fname * ".tmp"
                    open(tmp, "w") do f write(f, data) end
                    mv(tmp, fname; force=true)
                else
                    open(fname, "w") do f write(f, data) end
                end
                @info "FileDisplay: saved plot -> $fname"
            end
            return
        end
    end
    
    try
        show(stdout, "text/plain", x)
    catch
        @warn "FileDisplay: cannot display object (no suitable MIME)"
    end
end

function activate!(dir::AbstractString; prefix::AbstractString="plot",
                    fmt_priority=default_format_priority,
                    atomic::Bool=true,
                    timestamp_in_name::Bool=false)
    _ensure_dir(dir)
    d = FileDisplay(String(dir), String(prefix), collect(fmt_priority), Ref(0), ReentrantLock(), atomic, timestamp_in_name)
    pushdisplay(d)
    _file_display_ref[] = d
    return d
end

function deactivate!()
    d = _file_display_ref[]
    if d === nothing
        @warn "FilePlotDisplay: not active"
        return
    end
    try
        popdisplay()
    catch err
        @warn "FilePlotDisplay: popdisplay failed: $err"
    end
    _file_display_ref[] = nothing
end

active_display() = _file_display_ref[]

end #module
