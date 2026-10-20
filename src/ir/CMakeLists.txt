add_library(ir
    ir.cc
    lowering.cc
    expressions.cc
    expression_regions.cc
    statements.cc
    initializers.cc
    runtime_initialization.cc
    lifetimes.cc
    cfg.cc
    verifier.cc
    optimization.cc
    printer.cc
)
target_link_libraries(ir PUBLIC semantic)
target_include_directories(ir PUBLIC ${CMAKE_SOURCE_DIR})
