cmake_minimum_required(VERSION 3.10)

set(EXEC_SOURCES
    mmap_file.cc
    validate_file.cc
    file_size.cc
    validate_file.cc
    ascii.cc
)

add_library(base
    ${EXEC_SOURCES}
)

target_include_directories(base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)
