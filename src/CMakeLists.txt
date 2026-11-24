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
add_subdirectory(codegen)


target_link_libraries(${PROJECT_NAME} PRIVATE
    driver
    base
    lexer
    parser
    semantic
    mips_codegen
)

# The stage runners use the same source fixtures as custom_test_hook.
include(CTest)
set(BAKA_TEST_FIXTURES "${CMAKE_SOURCE_DIR}/../custom_tests")
if(NOT EXISTS "${BAKA_TEST_FIXTURES}/ir/execute.cc")
    set(BAKA_TEST_FIXTURES "${CMAKE_SOURCE_DIR}/../test")
endif()
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    if(EXISTS "${BAKA_TEST_FIXTURES}/ir/execute.cc")
        add_executable(ir_execution_tests "${BAKA_TEST_FIXTURES}/ir/execute.cc")
        target_link_libraries(ir_execution_tests PRIVATE ir lexer driver baka_heap_runtime)
        add_test(NAME ir COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/ir/run.py" --compiler $<TARGET_FILE:baka>)
    endif()
    if(EXISTS "${BAKA_TEST_FIXTURES}/ir_opt/run.py")
        add_test(NAME ir_opt COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/ir_opt/run.py" --compiler $<TARGET_FILE:baka>)
    endif()
    if(EXISTS "${BAKA_TEST_FIXTURES}/codegen/peephole_tests.cc")
        add_executable(peephole_tests "${BAKA_TEST_FIXTURES}/codegen/peephole_tests.cc")
        target_link_libraries(peephole_tests PRIVATE mips_codegen)
        add_test(NAME peephole COMMAND peephole_tests)
    endif()
    # Native execution needs the baka-mips64el Docker image (tools/mips/Dockerfile).
    find_program(DOCKER_EXECUTABLE docker)
    if(DOCKER_EXECUTABLE AND EXISTS "${BAKA_TEST_FIXTURES}/codegen/run.py")
        add_test(NAME codegen COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/codegen/run.py" --compiler $<TARGET_FILE:baka>)
    endif()
    if(DOCKER_EXECUTABLE AND EXISTS "${BAKA_TEST_FIXTURES}/codegen/test_driver.py")
        add_test(NAME mips_driver COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/codegen/test_driver.py" --compiler $<TARGET_FILE:baka>)
    endif()
    if(DOCKER_EXECUTABLE AND EXISTS "${BAKA_TEST_FIXTURES}/codegen/test_runtime.py")
        add_test(NAME mips_runtime COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/codegen/test_runtime.py" --compiler $<TARGET_FILE:baka>)
    endif()
    if(EXISTS "${BAKA_TEST_FIXTURES}/semantic/run.py")
        add_test(NAME semantic COMMAND ${Python3_EXECUTABLE}
            "${BAKA_TEST_FIXTURES}/semantic/run.py" --compiler $<TARGET_FILE:baka>)
    endif()
endif()
