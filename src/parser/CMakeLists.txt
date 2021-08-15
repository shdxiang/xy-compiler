cmake_minimum_required(VERSION 3.14)

project(xy-compiler-parser C CXX)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/"
    "${CMAKE_CURRENT_LIST_DIR}/"
)

BISON_TARGET(xy_parser syntactic.y ${CMAKE_CURRENT_BINARY_DIR}/syntactic.cpp)
FLEX_TARGET(xy_scanner lexical.l  ${CMAKE_CURRENT_BINARY_DIR}/lexical.cpp)
ADD_FLEX_BISON_DEPENDENCY(xy_scanner xy_parser)

file(GLOB SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/*.c
    ${CMAKE_CURRENT_BINARY_DIR}/*.cpp
)

add_library(xy-compiler-parser STATIC
    ${SRC}
    ${BISON_xy_parser_OUTPUTS}
    ${FLEX_xy_scanner_OUTPUTS})

target_link_libraries(xy-compiler-parser 
    PUBLIC ${llvm_libs}
    PUBLIC ${FLEX_LIBRARIES})

target_include_directories(xy-compiler-parser
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(WIN32)
    add_dependencies(xy-compiler-parser
        win_bison
        win_flex)
endif()
