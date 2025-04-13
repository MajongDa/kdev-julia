#include <julia.h>
#include <iostream>
#include <string>

// Function to process a Julia array
void process_julia_array(jl_array_t* array) {
    // Get the length of the array
    size_t n = jl_array_len(array);

    std::cout << "Array contains " << n << " elements:" << std::endl;

    // Iterate over the array and access each element
    for (size_t i = 0; i < n; ++i) {
        // Access the i-th element of the array using jl_array_ptr_ref
        //jl_value_t* element = jl_array_ptr_ref(array, i);

        // Convert the element to a string representation
        jl_function_t* string_func = jl_get_function(jl_base_module, "string");
        jl_value_t* element_str = jl_call1(string_func, jl_array_ptr_ref(array, i));

        const char* element_cstr = jl_string_ptr(element_str);
        std::cout << "Element " << i << ": " << element_cstr << std::endl;
    }
}

int main() {
    // Initialize Julia runtime
    jl_init();

    try {
        // Example Julia source code
        const char* julia_code = "x = 1 + 2; y = 3 * 4; println(x + y)";
        const char* filename = "input.jl";
        size_t lineno = 1;

        // Parse the Julia code into an array of expressions
        jl_value_t* result = jl_parse_all(julia_code, strlen(julia_code), const_cast<char*>(filename), strlen(filename), lineno);

        // Check if parsing succeeded
        if (jl_exception_occurred()) {
            jl_call2(jl_get_function(jl_base_module, "show"), jl_stderr_obj(), jl_exception_occurred());
            jl_printf(jl_stderr_stream(), "\n");
            throw std::runtime_error("Error occurred while parsing Julia code.");
        }

        // Process the resulting array
        if (jl_is_expr(result)) {
            process_julia_array((jl_array_t*)result);
        } else {
            throw std::runtime_error("Unexpected result from jl_parse_all (not an array).");
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // Shutdown Julia runtime
    jl_atexit_hook(0);

    return 0;
}
// Function to parse a Julia source file using jl_parse_all
void parse_julia_file(const std::string& julia_code) {
    // Initialize Julia runtime (only once per application)
    static bool initialized = false;
    if (!initialized) {
        jl_init();
        initialized = true;
    }

    // Prepare the input arguments for jl_parse_all
    const char* code = julia_code.c_str();       // Julia source code
    size_t code_len = julia_code.size();        // Length of the source code
    const char* filename = "input.jl";          // Dummy filename for error reporting
    size_t filename_len = strlen(filename);     // Length of the filename
    size_t lineno = 1;                          // Starting line number (1-based)

    // Call jl_parse_all to parse the Julia code
    jl_value_t* result = jl_parse_all(code, code_len, const_cast<char*>(filename), filename_len, lineno);

    // Check for errors
    if (jl_exception_occurred()) {
        jl_call2(jl_get_function(jl_base_module, "show"), jl_stderr_obj(), jl_exception_occurred());
        jl_printf(jl_stderr_stream(), "\n");
        throw std::runtime_error("Error occurred while parsing Julia code.");
    }

    // Process the resulting AST (array of Expr objects)
    if (jl_is_array(result)) {
        jl_array_t* ast_array = (jl_array_t*)result;

        // Iterate over the array of expressions
        size_t n = jl_array_len(ast_array);
        std::cout << "Parsed " << n << " top-level expressions:" << std::endl;
        for (size_t i = 0; i < n; ++i) {
            jl_value_t* expr = jl_array_ptr_ref(ast_array, i);

            // Convert each expression to a string representation
            jl_function_t* string_func = jl_get_function(jl_base_module, "string");
            jl_value_t* expr_str = jl_call1(string_func, expr);

            const char* expr_cstr = jl_string_ptr(expr_str);
            std::cout << "Expression " << i + 1 << ": " << expr_cstr << std::endl;
        }
    } else {
        throw std::runtime_error("Unexpected result from jl_parse_all (not an array).");
    }
}

// int main() {
//     try {
//         // Example Julia source code
//         std::string julia_code = R"(
//             x = 1 + 2
//             y = 3 * 4
//             println(x + y)
//         )";
//
//         // Parse the Julia code
//         parse_julia_file(julia_code);
//     } catch (const std::exception& e) {
//         std::cerr << "Exception: " << e.what() << std::endl;
//     }
//
//     // Shutdown Julia runtime
//     jl_atexit_hook(0);
//
//     return 0;
// }

// // Function to parse a Julia statement using JuliaSyntax.parsestmt
// std::string parse_julia_code(const std::string& julia_code) {
//     // Initialize Julia runtime (only once per application)
//     static bool initialized = false;
//     if (!initialized) {
//         jl_init();
//         initialized = true;
//
//         // Load the JuliaSyntax package
//         jl_eval_string("using JuliaSyntax");
//     }
//
//     // Convert the input Julia code to a Julia string
//     jl_value_t* julia_code_str = jl_cstr_to_string(julia_code.c_str());
//
//     // Get the Main module
//     jl_module_t* main_module = jl_main_module;
//
//     // Get the JuliaSyntax module
//     jl_module_t* julia_syntax_module = (jl_module_t*)jl_get_global(main_module, jl_symbol("JuliaSyntax"));
//
//     // Get the parsestmt function from the JuliaSyntax module
//     jl_function_t* parse_func = jl_get_function(julia_syntax_module, "parse");
//
//     // Create the ParseMode object (default mode)
//     jl_value_t* output_container= jl_eval_string("String");
//
//     // Call JuliaSyntax.parsestmt with the Julia code string and ParseMode
//     jl_value_t* args[2] = {output_container, julia_code_str};
//     jl_value_t* result = jl_call(parse_func, args, 2);
//
//     // Check for errors
//     if (jl_exception_occurred()) {
//         jl_call2(jl_get_function(jl_base_module, "show"), jl_stderr_obj(), jl_exception_occurred());
//         jl_printf(jl_stderr_stream(), "\n");
//         throw std::runtime_error("Error occurred while parsing Julia code.");
//     }
//
//     // Convert the result back to a C++ string (if possible)
//     // Note: JuliaSyntax.parsestmt returns a syntax tree, so we may need to convert it to a string representation
//     jl_function_t* string_func = jl_get_function(jl_base_module, "string");
//     jl_value_t* result_str = jl_call1(string_func, result);
//
//     const char* parsed_result = jl_string_ptr(result_str);
//     return std::string(parsed_result);
// }
//
// int main() {
//     try {
//         // Example Julia code to parse
//         std::string julia_code = "x = 1 + ]";
//
//         // Parse the Julia code
//         std::string parsed_result = parse_julia_code(julia_code);
//
//         // Print the parsed result
//         std::cout << "Parsed Julia Code: " << parsed_result << std::endl;
//     } catch (const std::exception& e) {
//         std::cerr << "Exception: " << e.what() << std::endl;
//     }
//
//     // Shutdown Julia runtime
//     jl_atexit_hook(0);
//
//     return 0;
// }
