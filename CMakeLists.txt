cmake_minimum_required(VERSION 3.14)

project(xy-compiler-proj C CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    # Support Unix Makefiles and Ninja
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_LIBRARY_OUTPUT_DIRECTORY" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY" FORCE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_ARCHIVE_OUTPUT_DIRECTORY" FORCE)

set(OUTPUT_DEBUG ${CMAKE_BINARY_DIR}/bin CACHE STRING "OUTPUT_DEBUG" FORCE)
set(OUTPUT_RELEASE ${CMAKE_BINARY_DIR}/bin CACHE STRING "OUTPUT_RELEASE" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin CACHE STRING "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG" FORCE)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set to 11 or 14 to enable C++11 or C++17 builds, respectively." FORCE)
set(CMAKE_C_STANDARD 11 CACHE STRING "Set to 99 or 11 to enable C99 or C11 builds, respectively." FORCE)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif()
endif()

if(WIN32)
    add_compile_definitions("WIN32_LEAN_AND_MEAN" "_CRT_SECURE_NO_WARNINGS" "NOMINMAX")
    if(MSVC)
        add_compile_options("/source-charset:utf-8" "/EHsc" "/Zc:threadSafeInit-")

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

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Linking with: ${LLVM_DEFINITIONS}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs core support mcparser option bitreader profiledata irreader executionengine mcjit x86codegen x86asmparser)
message(STATUS "Linking with: ${llvm_libs}")

if(WIN32)
    execute_process(COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_CURRENT_LIST_DIR}/external/winflexbision
        -B ${CMAKE_BINARY_DIR}/external/winflexbision
        -G ${CMAKE_GENERATOR}
        -D CMAKE_BUILD_TYPE=Debug
        -D OUTPUT_DEBUG=${OUTPUT_DEBUG}
        -D OUTPUT_RELEASE=${OUTPUT_RELEASE}
        -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        -D CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
        -D CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}
    )

    execute_process(COMMAND ${CMAKE_COMMAND}
        --build ${CMAKE_BINARY_DIR}/external/winflexbision
    )

    execute_process(COMMAND ${CMAKE_COMMAND}
        --install ${CMAKE_BINARY_DIR}/external/winflexbision --prefix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

    add_subdirectory(external/winflexbision)

    set(BISON_ROOT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "BISON_ROOT_DIR" FORCE)
    set(FLEX_ROOT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "FLEX_ROOT_DIR" FORCE)

    set(BISON_EXECUTABLE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/win_bison.exe" CACHE STRING "BISON_EXECUTABLE" FORCE)
    set(BISON_version_result "0" CACHE STRING "BISON_version_result" FORCE)
    set(BISON_version_output "bison++ Version 1,0,0" CACHE STRING "BISON_version_result" FORCE)

    set(FLEX_EXECUTABLE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/win_flex.exe" CACHE STRING "FLEX_EXECUTABLE" FORCE)
    set(FLEX_version_result "0" CACHE STRING "FLEX_version_result" FORCE)
    set(FLEX_FIND_REQUIRED "0" CACHE STRING "FLEX_FIND_REQUIRED" FORCE)

    include(UseBISON)
    include(UseFLEX)

    include_directories(external/unistd_h)
else()
    find_package(BISON REQUIRED)
    find_package(FLEX REQUIRED)
endif()

add_subdirectory(src)