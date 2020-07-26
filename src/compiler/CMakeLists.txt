cmake_minimum_required(VERSION 3.14)

project(xy-compiler C CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set to 11 or 14 to enable C++11 or C++17 builds, respectively." FORCE)
set(CMAKE_C_STANDARD 11 CACHE STRING "Set to 99 or 11 to enable C99 or C11 builds, respectively." FORCE)

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

configure_file(version.hpp.in "${CMAKE_CURRENT_LIST_DIR}/version.hpp")

file(GLOB COMPILER_SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/*.c
    ${CMAKE_CURRENT_BINARY_DIR}/*.cpp
)

add_executable(xy-compiler ${COMPILER_SRC})

target_link_libraries(xy-compiler
    PUBLIC xy-compiler-parser)
