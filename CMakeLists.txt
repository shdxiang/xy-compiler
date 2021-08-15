cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(xy-compiler-proj C CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Support Unix Makefiles and Ninja
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_LIBRARY_OUTPUT_DIRECTORY" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY" FORCE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_ARCHIVE_OUTPUT_DIRECTORY" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG" FORCE)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set to 11 or 14 to enable C++11 or C++17 builds, respectively." FORCE)
set(CMAKE_C_STANDARD 17 CACHE STRING "Set to 99 or 11 to enable C99 or C11 builds, respectively." FORCE)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY C_STANDARD_REQUIRED ON)

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if(UNIX)
        if(APPLE)
            add_compile_options("-m64" "-fPIC" "-march=native")
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                add_compile_options("-g" "-O0")
            else()
                add_compile_options("-O3")
            endif()
            set(CMAKE_MACOSX_RPATH 1 CACHE STRING "CMAKE_MACOSX_RPATH" FORCE)
            option(DISABLE_COTIRE "DISABLE_COTIRE" on)
            if(DISABLE_COTIRE)
                set(__COTIRE_INCLUDED TRUE CACHE BOOL "__COTIRE_INCLUDED" FORCE)
                function (cotire)
                endfunction()
            endif()
        else()
            add_compile_options("-fPIC")
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                add_compile_options("-g")
            else()
                #add_compile_options("-O3")
            endif()
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
                add_compile_options("-m64" "-march=westmere")
            endif()
        endif()
    endif()
endif()

include(external/InitializeExternalComponents.cmake)

add_subdirectory(src)