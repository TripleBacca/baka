add_library(mips_codegen
    codegen.cc
    mips.cc
    select.cc
    peephole.cc
    printer.cc
    data.cc
    runtime.cc
)
target_link_libraries(mips_codegen PUBLIC ir)
target_include_directories(mips_codegen PUBLIC ${CMAKE_SOURCE_DIR})
