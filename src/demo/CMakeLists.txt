cmake_minimum_required(VERSION 3.14)

project(xy-compiler-demo C CXX)

configure_file("demo.xy" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo.xy" NEWLINE_STYLE UNIX)

if(COMPILE_DEMO)
    add_custom_command(OUTPUT demo
        COMMAND xy-compiler < "${CMAKE_CURRENT_LIST_DIR}/demo.xy" > "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo"
    )

    add_custom_target(compile_demo ALL
        DEPENDS demo xy-compiler
    )

    set_source_files_properties(
        "demo.xy" PROPERTIES GENERATED TRUE)

    add_dependencies(compile_demo xy-compiler)
endif()