cmake_minimum_required(VERSION 3.14)

project(xy-compiler-demo C CXX)

configure_file("demo.xy" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo.xy" NEWLINE_STYLE UNIX)

option(COMPILE_DEMO "Compile demo.xy to LLVM IR (demo.ll) at build time" OFF)
if(COMPILE_DEMO)
    set(DEMO_IR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/demo.ll")
    add_custom_command(OUTPUT ${DEMO_IR}
        COMMAND xy-compiler --no-run -f "${CMAKE_CURRENT_LIST_DIR}/demo.xy" -o "${DEMO_IR}"
        DEPENDS xy-compiler "${CMAKE_CURRENT_LIST_DIR}/demo.xy"
    )

    add_custom_target(compile_demo ALL
        DEPENDS ${DEMO_IR}
    )
endif()
