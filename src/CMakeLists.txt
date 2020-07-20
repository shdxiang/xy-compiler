cmake_minimum_required(VERSION 3.14)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "CMAKE_TOOLCHAIN_FILE")
endif()

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

llvm_map_components_to_libnames(llvm_libs all)
message(STATUS "Linking with: ${llvm_libs}")

configure_file("demo.xy" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo.xy" COPYONLY)

add_executable(xy-compiler 
    ${SRC}
    ${BISON_xy_parser_OUTPUTS}
    ${FLEX_xy_scanner_OUTPUTS})

target_link_libraries(xy-compiler 
    PUBLIC ${llvm_libs}
    PUBLIC ${FLEX_LIBRARIES})

if(WIN32)
    add_dependencies(xy-compiler
        win_bison
        win_flex)
endif()

add_custom_command(OUTPUT demo
    COMMAND xy-compiler < "${CMAKE_CURRENT_LIST_DIR}/demo.xy" > "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo"
)

add_custom_target(compile_demo ALL
    DEPENDS demo xy-compiler
)

set_source_files_properties(
    "demo.xy" PROPERTIES GENERATED TRUE)

add_dependencies(compile_demo xy-compiler)