cmake_minimum_required(VERSION 3.14)

project(xy-compiler C CXX)

configure_file(version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/version.hpp")

add_executable(xy-compiler
    main.cpp
    gen.cpp)

target_link_libraries(xy-compiler
    PRIVATE xy-compiler-parser
    PRIVATE cxxopts
    PRIVATE xy-rt
    PRIVATE ${llvm_libs})

target_include_directories(xy-compiler
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/"
)
