add_library(ir
    ir.cc
    lowering.cc
    expressions.cc
    primary.cc
    addresses.cc
    operators.cc
    calls.cc
    conditional.cc
    statements.cc
    functions.cc
    iteration.cc
    selection.cc
    jumps.cc
    initializers.cc
    runtime_initialization.cc
    lifetimes.cc
    cfg.cc
    verifier.cc
    optimization.cc
    constant_folding.cc
    dead_code.cc
    printer.cc
)
target_link_libraries(ir PUBLIC semantic)
target_include_directories(ir PUBLIC ${CMAKE_SOURCE_DIR})
