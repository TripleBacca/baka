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

# Each semantic pass has a structural positive/negative test. The same target
# uses the real lexer/parser and semantic library, so phase contracts are tested
# independently of the command-line output format.
include(CTest)
if(BUILD_TESTING)
    add_executable(semantic_pass_tests ../custom_tests/semantic/pass_tests.cc)
    target_link_libraries(semantic_pass_tests PRIVATE semantic lexer driver)
    add_executable(semantic_conversion_tests ../custom_tests/semantic/conversion_tests.cc)
    target_link_libraries(semantic_conversion_tests PRIVATE semantic lexer driver)
    add_executable(semantic_member_call_tests ../custom_tests/semantic/member_call_tests.cc)
    target_link_libraries(semantic_member_call_tests PRIVATE semantic lexer driver)
    add_test(NAME semantic_member_calls COMMAND semantic_member_call_tests)
    foreach(scenario numeric contexts calls compound pointers metadata)
        add_test(NAME semantic_conversions_${scenario} COMMAND semantic_conversion_tests ${scenario})
    endforeach()
    foreach(phase registration resolution annotation checks)
        add_test(NAME semantic_${phase} COMMAND semantic_pass_tests ${phase})
        add_test(NAME semantic_${phase}_error COMMAND semantic_pass_tests ${phase}_error)
    endforeach()
    add_test(NAME semantic_resolution_loop_targets COMMAND semantic_pass_tests resolution_loop_targets)
    add_test(NAME semantic_resolution_this COMMAND semantic_pass_tests resolution_this)
    add_test(NAME semantic_annotation_scoped_type COMMAND semantic_pass_tests annotation_scoped_type)
endif()

if(BUILD_TESTING)
    add_executable(ir_pass_tests ../custom_tests/ir/pass_tests.cc)
    target_link_libraries(ir_pass_tests PRIVATE ir lexer driver)
    target_compile_definitions(ir_pass_tests PRIVATE IR_TEST_ROOT="${CMAKE_SOURCE_DIR}/../custom_tests/ir")
    foreach(scenario 01_scalars 02_control 03_calls 04_arrays 05_sequence 06_switch
                     07_globals 08_inheritance 09_returns 10_lvalues 11_wrap 12_arguments
                     13_operators 14_labeled 15_once 16_constants 17_relocations 18_static_conversions 19_variadic 20_casts 21_mutation_conversions 22_forward_types verifier cfg contracts optimization)
        add_test(NAME ir_${scenario} COMMAND ir_pass_tests ${scenario})
    endforeach()
endif()
