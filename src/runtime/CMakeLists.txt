cmake_minimum_required(VERSION 3.14)

set(project_name "xy-rt")
project(${project_name} C CXX)

file(GLOB_RECURSE src
    ${CMAKE_CURRENT_LIST_DIR}/implementation/*.c
    ${CMAKE_CURRENT_LIST_DIR}/implementation/*.cpp
)

# ---------------------------------------
set(LIBXY_VER "${VER}")
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

target_include_directories(${project_name}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include/public)
