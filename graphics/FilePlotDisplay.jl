module FilePlotDisplay

using Dates
using Base.Threads: @threads
using Plots
export activate!, deactivate!, active_display, default_format_priority

# Настройки расширяемые
const default_format_priority = (MIME("image/png"), MIME("image/svg+xml"))

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
    if occursin("png", s) return "png"
    end
    if occursin("svg", s) return "svg"
    end
    # fallback
    return replace(replace(s, '/' => "_"), '+' => "_")
end

function Base.display(d::FileDisplay, x)
    if typeof(x) <: Plots.Plot
        @info "FileDisplay: display called for Plot object (likely from plot() call)"
    end
    # пытаемся найти подходящий формат по приоритету
    for m in d.fmt_priority
        if showable(m, x)
            io = IOBuffer()
            try
                show(io, m, x)
            catch err
                @warn "Ошибка сериализации в MIME $m: $err"
                continue
            end
            data = take!(io)
            # формируем имя
            lock(d.lock) do
                d.counter[] += 1
                idx = d.counter[]
                name_parts = [d.prefix, lpad(string(idx), 4, '0')]
                if d.timestamp_in_name
                    push!(name_parts, Dates.format(now(), "yyyy-mm-dd_HHMMSS"))
                end
                ext = _mime_to_ext(m)
                fname = joinpath(d.dir, join(name_parts, "_") * "." * ext)
                # атомарная запись: tmp -> mv
                if d.atomic
                    tmp = fname * ".tmp"
                    open(tmp, "w") do f
                        write(f, data)
                    end
                    mv(tmp, fname; force=true)
                else
                    open(fname, "w") do f
                        write(f, data)
                    end
                end
                @info "FileDisplay: saved plot -> $fname"
            end
            return
        end
    end
    # если ни один mime не подходит — fallback: показать текст
    try
        show(stdout, "text/plain", x)
    catch
        @warn "FileDisplay: cannot display object (no suitable MIME)"
    end
end

"""
activate!(dir; prefix="plot", fmt_priority=default_format_priority,
          atomic=true, timestamp_in_name=false)

Включает перехват графики и сохранение в каталог `dir`.
Возвращает объект FileDisplay.
"""

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

"""
deactivate!()

Отключает перехват (popdisplay) если ранее вызывался activate!.
"""
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
    return
end

active_display() = _file_display_ref[]

end #module
