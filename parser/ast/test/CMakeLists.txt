cmake_minimum_required(VERSION 3.16)

project(test_ast)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

find_package(KDevPlatform REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${KDevPlatform_INCLUDE_DIRS}
)

set(TEST_SOURCES
    test_ast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ast.cpp
)

add_executable(test_ast ${TEST_SOURCES})

target_link_libraries(test_ast PRIVATE
    Qt6::Core
    Qt6::Test
    KDev::Language
)

add_test(NAME AstTest COMMAND test_ast)
