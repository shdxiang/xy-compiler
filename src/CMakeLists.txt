cmake_minimum_required(VERSION 3.14)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "CMAKE_TOOLCHAIN_FILE")
endif()

project(xy-compiler C CXX)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/"
    "${CMAKE_CURRENT_LIST_DIR}/src/"
)

if(WIN32)
    execute_process(COMMAND CMD /c "git describe --tags --always --dirty" OUTPUT_VARIABLE VER)
elseif(UNIX)
    execute_process(COMMAND git describe --tags --always --dirty OUTPUT_VARIABLE VER)
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

configure_file(version.hpp.in "version.hpp")

BISON_TARGET(xy_parser syntactic.y ${CMAKE_CURRENT_BINARY_DIR}/syntactic.cpp)
FLEX_TARGET(xy_scanner lexical.l  ${CMAKE_CURRENT_BINARY_DIR}/lexical.cpp)
ADD_FLEX_BISON_DEPENDENCY(xy_scanner xy_parser)

file(GLOB SRC
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/*.c
    ${CMAKE_CURRENT_BINARY_DIR}/*.cpp
)

add_executable(xy-compiler ${SRC})
target_link_libraries(xy-compiler ${llvm_libs})

if(WIN32)
    add_dependencies(xy-compiler
        win_bison
        win_flex)
endif()