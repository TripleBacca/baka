cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME baka)
set(PROJECT_VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)

project(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    LANGUAGES CXX
)

set(PROJECT_SOURCES
    main.cc
)

# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)

# main bin
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

set(COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
)

# compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMPILE_OPTIONS}
)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(base)
add_subdirectory(driver)
add_subdirectory(lexer)


target_link_libraries(${PROJECT_NAME} PRIVATE
    driver
    base
    lexer
)
