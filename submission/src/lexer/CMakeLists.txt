cmake_minimum_required(VERSION 3.10)

set(EXEC_SOURCES
    lexer.cc
)

add_library(lexer
    ${EXEC_SOURCES}
)

target_link_libraries(lexer PUBLIC
    base
)

target_include_directories(lexer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)


# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)
