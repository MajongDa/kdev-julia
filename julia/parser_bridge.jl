"""
JSON Bridge for Julia Syntax Parser

This module provides functionality to parse Julia source code using JuliaSyntax.jl
and export the AST to a JSON format that can be consumed by C++ code.
"""

using JuliaSyntax
using JSON
using Dates

export export_ast_to_json, parse_julia_file

"""
Convert JuliaSyntax Kind enum to string representation
"""
function kind_to_string(kind)
    return string(kind)
end

"""
Convert a JuliaSyntax node to a JSON-serializable dictionary
"""
function node_to_dict(node)
    result = Dict{String, Any}()
    
    # Basic node information
    result["kind"] = kind_to_string(kind(node))
    result["is_leaf"] = JuliaSyntax.is_leaf(node)
    result["span"] = JuliaSyntax.span(node)
    
    # Position information - add range for ALL nodes (not just non-leaf)
    try
        source_file = JuliaSyntax.sourcefile(node)
        if source_file !== nothing
            byte_range = JuliaSyntax.byte_range(node)
            if byte_range !== nothing
                # Use first and last byte positions to get line/column
                start_byte = first(byte_range)
                end_byte = last(byte_range)
                
                # Get line from source file
                start_line = JuliaSyntax.source_line(source_file, start_byte)
                end_line = JuliaSyntax.source_line(source_file, end_byte)
                
                # Get line from source file
                start_line = JuliaSyntax.source_line(source_file, start_byte)
                end_line = JuliaSyntax.source_line(source_file, end_byte)
                
                # Calculate column - find position of last newline before start_byte
                source_text = JuliaSyntax.sourcetext(source_file)
                start_col = 1
                
                if source_text !== nothing && start_byte > 1
                    # Find the last newline BEFORE start_byte
                    line_start = start_byte
                    for i in (start_byte-1):-1:1
                        if source_text[i] == '\n'
                            line_start = i + 1
                            break
                        end
                    end
                    # Column is the offset from the line start (1-indexed)
                    start_col = start_byte - line_start + 1
                end
                
                # Calculate end column the same way (add 1 for exclusive end)
                end_col = 1
                if source_text !== nothing && end_byte > 1
                    line_start = end_byte
                    for i in (end_byte-1):-1:1
                        if source_text[i] == '\n'
                            line_start = i + 1
                            break
                        end
                    end
                    # Column is offset from line start + 1 for exclusive end
                    end_col = end_byte - line_start + 2
                end
                
                result["range"] = Dict(
                    "start_line" => start_line,
                    "start_column" => start_col,
                    "end_line" => end_line,
                    "end_column" => end_col
                )
            end
        end
    catch e
        # Skip range info if there's any error
        # println("Range error: ", e)
    end
    
    # Add source text if available
    try
        source_text = JuliaSyntax.sourcetext(node)
        if source_text !== nothing
            result["text"] = source_text
        end
    catch
        # Skip if source text is not available
    end
    
    # Process children for non-leaf nodes
    if !JuliaSyntax.is_leaf(node)
        children = JuliaSyntax.children(node)
        if children !== nothing
            result["children"] = [node_to_dict(child) for child in children]
        end
    end
    
    return result
end

"""
Parse Julia source code and export AST to JSON format
"""
function export_ast_to_json(source_code::String; filename::String="untitled.jl")
    try
        # Parse the source code - need to specify tree type
        tree = JuliaSyntax.parseall(JuliaSyntax.SyntaxNode, source_code, filename=filename)
        
        # Convert to JSON-serializable format
        json_dict = node_to_dict(tree)
        
        # Add metadata
        json_dict["metadata"] = Dict(
            "filename" => filename,
            "julia_version" => string(VERSION),
            "juliasyntax_version" => "1.0.0",  # TODO: Get actual version
            "timestamp" => string(now())
        )
        
        return JSON.json(json_dict)
        
    catch e
        # Return error information
        error_dict = Dict(
            "error" => true,
            "message" => string(e),
            "kind" => "ParseError",
            "metadata" => Dict(
                "filename" => filename,
                "timestamp" => string(now())
            )
        )
        return JSON.json(error_dict)
    end
end

"""
Parse a Julia file and export AST to JSON
"""
function parse_julia_file(filepath::String)
    try
        # Read the file
        source_code = read(filepath, String)
        
        # Extract filename for error reporting
        filename = basename(filepath)
        
        return export_ast_to_json(source_code, filename=filename)
        
    catch e
        error_dict = Dict(
            "error" => true,
            "message" => "Failed to read file: " * string(e),
            "kind" => "FileError",
            "metadata" => Dict(
                "filepath" => filepath,
                "timestamp" => string(now())
            )
        )
        return JSON.json(error_dict)
    end
end

"""
Main function for command-line usage
"""
function main()
    if length(ARGS) < 1
        println(stderr, "Usage: julia parser_bridge.jl <julia_file_or_code> [--stdin]")
        exit(1)
    end
    
    if ARGS[1] == "--stdin" && length(ARGS) >= 2
        # Parse from standard input
        mode = ARGS[2]  # "file" or "code"
        
        if mode == "file"
            # Read filename from stdin and parse file
            filename = strip(readline(stdin))
            result = parse_julia_file(filename)
        elseif mode == "code"
            # Read source code from stdin
            source_code = read(stdin, String)
            filename = "stdin.jl"
            result = export_ast_to_json(source_code, filename=filename)
        else
            println(stderr, "Unknown mode: $mode. Use 'file' or 'code'")
            exit(1)
        end
    else
        # Parse file or code from command line
        arg = ARGS[1]
        
        if isfile(arg)
            result = parse_julia_file(arg)
        else
            # Treat as source code
            result = export_ast_to_json(arg, filename="command_line.jl")
        end
    end
    
    # Output JSON result
    println(result)
end

# Run main function if this script is executed directly
if abspath(PROGRAM_FILE) == @__FILE__
    main()
end