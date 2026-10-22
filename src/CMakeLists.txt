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
    -O2
)

# compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMPILE_OPTIONS}
)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(base)
add_subdirectory(driver)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(semantic)
add_subdirectory(ir)


target_link_libraries(${PROJECT_NAME} PRIVATE
    driver
    base
    lexer
    parser
    semantic
)

# The stage runners use the same source fixtures as custom_test_hook.
include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_executable(ir_execution_tests ../custom_tests/ir/execute.cc)
    target_link_libraries(ir_execution_tests PRIVATE ir lexer driver)
    add_test(NAME semantic COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/../custom_tests/semantic/run.py --compiler $<TARGET_FILE:baka>)
    add_test(NAME ir COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/../custom_tests/ir/run.py --compiler $<TARGET_FILE:baka>)
endif()
