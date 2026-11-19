cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(xy-compiler-proj C CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Support Unix Makefiles and Ninja
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_MACOSX_RPATH 1)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions("WIN32_LEAN_AND_MEAN" "_CRT_SECURE_NO_WARNINGS" "NOMINMAX")
    if(MSVC)
        add_compile_definitions("__extension__")
        add_compile_options("/source-charset:utf-8")

        if((NOT CMAKE_BUILD_TYPE) OR (NOT CMAKE_BUILD_TYPE STREQUAL "Debug"))
            add_compile_options("/MT")
        else()
            add_compile_options("/MTd")
        endif()

        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO)
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
            message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
        endforeach()
    endif()
else()
    add_compile_options("-Wall")
endif()

# Version string shared by the compiler and the runtime library
find_package(Git QUIET)
set(VER "")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE VER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
endif()
message(STATUS "xy-compiler version: ${VER}")

include(external/InitializeExternalComponents.cmake)

add_subdirectory(src)

option(XY_BUILD_TESTS "Build the tests" ON)
if(XY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
