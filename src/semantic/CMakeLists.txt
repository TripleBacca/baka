cmake_minimum_required(VERSION 3.10)

add_library(semantic
    types.cc
    symbols.cc
    declarations.cc
    ast_declarations.cc
    array_bounds.cc
    aggregates.cc
    expressions.cc
    expression_types.cc
    primary.cc
    operators.cc
    calls.cc
    initializers.cc
    members.cc
    constants.cc
    execution_plans.cc
    initialization_plans.cc
    call_plans.cc
    cleanup_plans.cc
    conversions.cc
    call_conversions.cc
    operator_conversions.cc
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
