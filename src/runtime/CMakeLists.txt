cmake_minimum_required(VERSION 3.14)

set(project_name "xy-rt")
project(${project_name} C CXX)

file(GLOB_RECURSE src
    ${CMAKE_CURRENT_LIST_DIR}/implementation/*.c
    ${CMAKE_CURRENT_LIST_DIR}/implementation/*.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/implementation/*.c
    ${CMAKE_CURRENT_BINARY_DIR}/implementation/*.cpp
)

# ---------------------------------------
if(WIN32)
    execute_process(
        COMMAND CMD /c "git describe --tags --always --dirty" 
        OUTPUT_VARIABLE LIBXY_VER
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
elseif(UNIX)
    execute_process(
        COMMAND git describe --tags --always --dirty
        OUTPUT_VARIABLE LIBXY_VER
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    if(APPLE)
        execute_process(COMMAND sw_vers -productVersion OUTPUT_VARIABLE MACOSX_DEPLOYMENT_TARGET)
        string(REGEX REPLACE "\n$" "" MACOSX_DEPLOYMENT_TARGET "${MACOSX_DEPLOYMENT_TARGET}")
        message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${MACOSX_DEPLOYMENT_TARGET}")
    endif()
else()
    message(fatal "Unsupported platform detected.")
endif()

string(REGEX REPLACE "\n$" "" LIBXY_VER "${LIBXY_VER}")
message(STATUS "libxy version: ${LIBXY_VER}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/include/public/libxy/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/libxy/version.h")

# ----------------------------------------
file(GLOB_RECURSE module_defs
    ${CMAKE_CURRENT_LIST_DIR}/*.def
)

include(GenerateExportHeader)

if(WIN32)
    add_library(${project_name}
        SHARED ${src} ${module_defs}
    )
else()
    add_library(${project_name}
        SHARED ${src}
    )
endif()

generate_export_header(${project_name}
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/libxy/exports.h"
    EXPORT_MACRO_NAME LIB_XY_EXPORTS
)

target_link_libraries(${project_name}
    PUBLIC ${llvm_libs})

target_include_directories(${project_name}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/private)

# ----------------------------------------
add_library(${project_name}-static STATIC
    ${src})

target_compile_definitions(${project_name}-static
    PUBLIC XY_RT_STATIC_DEFINE)

target_link_libraries(${project_name}-static
    PUBLIC ${llvm_libs})

target_include_directories(${project_name}-static
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include/public
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/private)