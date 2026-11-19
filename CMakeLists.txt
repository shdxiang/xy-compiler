cmake_minimum_required(VERSION 3.21)

# LLVMConfig.cmake runs C checks, so C must be enabled too
project(xy-compiler C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

add_compile_options(-Wall)

# ---- Dependencies ----

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} in ${LLVM_DIR}")
llvm_map_components_to_libnames(LLVM_LIBS core support executionengine mcjit native)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

# cxxopts is header only, so its CMakeLists.txt is not added
include(FetchContent)
FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG 44380e5a44706ab7347f400698c703eb2a196202 # v3.3.1
    SOURCE_SUBDIR do-not-add-subdirectory)
FetchContent_MakeAvailable(cxxopts)

# ---- Version ----

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE XY_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
endif()
if(NOT XY_VERSION)
    set(XY_VERSION "unknown")
endif()
message(STATUS "xy-compiler version: ${XY_VERSION}")

# ---- Compiler ----

# Fail the build on grammar conflicts instead of letting bison silently
# pick a resolution
BISON_TARGET(parser src/syntactic.y ${CMAKE_CURRENT_BINARY_DIR}/syntactic.cpp
    COMPILE_FLAGS "-Wall -Werror=conflicts-sr -Werror=conflicts-rr")
FLEX_TARGET(lexer src/lexical.l ${CMAKE_CURRENT_BINARY_DIR}/lexical.cpp)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

add_executable(xy-compiler
    src/main.cpp
    src/gen.cpp
    src/runtime.cpp
    ${BISON_parser_OUTPUTS}
    ${FLEX_lexer_OUTPUTS})
target_include_directories(xy-compiler PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(xy-compiler SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${cxxopts_SOURCE_DIR}/include)
target_compile_definitions(xy-compiler PRIVATE
    ${LLVM_DEFINITIONS_LIST}
    XY_VERSION="${XY_VERSION}")
target_link_libraries(xy-compiler PRIVATE ${LLVM_LIBS})

# ---- Tests ----

option(XY_BUILD_TESTS "Build the tests" ON)
if(XY_BUILD_TESTS)
    enable_testing()
    set(RUN_TEST ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_test.cmake)
    set(DEMO ${CMAKE_CURRENT_SOURCE_DIR}/demo.xy)

    # Each tests/cases/*.xy file is a test; see tests/run_test.cmake
    file(GLOB TEST_CASES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/cases/*.xy)
    foreach(test_case IN LISTS TEST_CASES)
        get_filename_component(test_name ${test_case} NAME_WE)
        add_test(NAME ${test_name}
            COMMAND ${CMAKE_COMMAND} -DCOMPILER=$<TARGET_FILE:xy-compiler> -DSOURCE=${test_case} -P ${RUN_TEST})
    endforeach()

    add_test(NAME demo
        COMMAND ${CMAKE_COMMAND} -DCOMPILER=$<TARGET_FILE:xy-compiler> -DSOURCE=${DEMO} -DEXPECT_STDOUT=11 -P ${RUN_TEST})

    add_test(NAME cli_input_file COMMAND xy-compiler -f ${DEMO})
    set_tests_properties(cli_input_file PROPERTIES PASS_REGULAR_EXPRESSION "^11\n$")

    add_test(NAME cli_missing_file COMMAND xy-compiler -f ${CMAKE_CURRENT_BINARY_DIR}/does-not-exist.xy)
    set_tests_properties(cli_missing_file PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli_emit_ir COMMAND xy-compiler --no-run -o - ${DEMO})
    set_tests_properties(cli_emit_ir PROPERTIES PASS_REGULAR_EXPRESSION "define internal i64 @sum\\(i64 %a, i64 %b\\)")

    add_test(NAME cli_version COMMAND xy-compiler --version)
    set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "^xy-compiler [^\n]+\n$")

    add_test(NAME cli_bad_option COMMAND xy-compiler --no-such-option)
    set_tests_properties(cli_bad_option PROPERTIES WILL_FAIL TRUE)
endif()
