cmake_minimum_required(VERSION 3.14)

project(xy-compiler-parser C CXX)

# Fail the build on grammar conflicts instead of letting bison silently
# pick a resolution
BISON_TARGET(xy_parser syntactic.y ${CMAKE_CURRENT_BINARY_DIR}/syntactic.cpp
    COMPILE_FLAGS "-Wall -Werror=conflicts-sr -Werror=conflicts-rr")
FLEX_TARGET(xy_scanner lexical.l ${CMAKE_CURRENT_BINARY_DIR}/lexical.cpp)
ADD_FLEX_BISON_DEPENDENCY(xy_scanner xy_parser)

add_library(xy-compiler-parser STATIC
    ast.h
    ${BISON_xy_parser_OUTPUTS}
    ${FLEX_xy_scanner_OUTPUTS})

target_include_directories(xy-compiler-parser
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(WIN32)
    add_dependencies(xy-compiler-parser
        win_bison
        win_flex)
endif()
