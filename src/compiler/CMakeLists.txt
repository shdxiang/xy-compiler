cmake_minimum_required(VERSION 3.14)

project(xy-compiler C CXX)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/"
    "${CMAKE_CURRENT_LIST_DIR}/"
)

if(WIN32)
    execute_process(
        COMMAND CMD /c "git describe --tags --always --dirty" 
        OUTPUT_VARIABLE VER 
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
elseif(UNIX)
    execute_process(
        COMMAND git describe --tags --always --dirty 
        OUTPUT_VARIABLE VER 
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    if(APPLE)
        execute_process(COMMAND sw_vers -productVersion OUTPUT_VARIABLE MACOSX_DEPLOYMENT_TARGET)
        string(REGEX REPLACE "\n$" "" MACOSX_DEPLOYMENT_TARGET "${MACOSX_DEPLOYMENT_TARGET}")
        message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${MACOSX_DEPLOYMENT_TARGET}")
    endif()
else()
    message(fatal "Unsupported platform detected.")
endif()

string(REGEX REPLACE "\n$" "" VER "${VER}")
message(STATUS "xy-compiler version: ${VER}")

configure_file(version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/version.hpp")

file(GLOB_RECURSE COMPILER_SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/*.c
    ${CMAKE_CURRENT_BINARY_DIR}/*.cpp
)

add_executable(xy-compiler ${COMPILER_SRC})

target_compile_definitions(xy-compiler
    PRIVATE MODULE_API_EXPORTS)

target_link_libraries(xy-compiler
    PUBLIC xy-compiler-parser
    PUBLIC cxxopts)

target_include_directories(xy-compiler
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/"
)