cmake_minimum_required(VERSION 3.10)

set(EXEC_SOURCES
    driver.cc
    gctx.cc
    # args.cc this is not diff tu
)

add_library(driver
    ${EXEC_SOURCES}
)

target_link_libraries(driver PUBLIC
    base
)

target_include_directories(driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)
