set (parser_SRC
    astbuilder.cpp
)

include_directories("~/.juliaup/bin/julia-1.11.3/include/julia/")
set(ENABLE_CXX_EXCEPTIONS 1)



add_executable(astbuilder ${parser_SRC})
#target_link_directories( astbuilder PRIVATE "~/.juliaup/bin/julia-1.11.3/lib/julia/")
target_link_libraries(astbuilder
    Qt6::Core
    ${Julia_LIBRARY}
)


target_compile_options (astbuilder PUBLIC -fexceptions)
