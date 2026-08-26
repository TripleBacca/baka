cmake_minimum_required(VERSION 3.10)

set(EXEC_SOURCES
    parser.cc
    expressions.cc
    function.cc
    jumpStatement.cc
    identifier.cc
)

add_library(parser
    ${EXEC_SOURCES}
)

target_link_libraries(parser PUBLIC
    base
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)


# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)
