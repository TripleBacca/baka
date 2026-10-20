cmake_minimum_required(VERSION 3.10)

add_library(semantic
    types.cc
    symbols.cc
    declarations.cc
    ast_declarations.cc
    aggregates.cc
    expressions.cc
    conversions.cc
    overloads.cc
    semantic_analyzer.cc
    registration.cc
    resolution.cc
    annotation.cc
    checks.cc
)

target_link_libraries(semantic PUBLIC
    base
    parser
)

target_include_directories(semantic PUBLIC
    ${CMAKE_SOURCE_DIR}
)
