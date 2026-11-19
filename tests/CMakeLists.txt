set(RUN_TEST "${CMAKE_CURRENT_LIST_DIR}/run_test.cmake")

file(GLOB TEST_CASES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/cases/*.xy")
foreach(test_case IN LISTS TEST_CASES)
    get_filename_component(test_name "${test_case}" NAME_WE)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND} -DCOMPILER=$<TARGET_FILE:xy-compiler> -DSOURCE=${test_case} -P ${RUN_TEST})
endforeach()

add_test(NAME demo
    COMMAND ${CMAKE_COMMAND} -DCOMPILER=$<TARGET_FILE:xy-compiler> -DSOURCE=${PROJECT_SOURCE_DIR}/src/demo/demo.xy -DEXPECT_STDOUT=11 -P ${RUN_TEST})

add_test(NAME cli_input_file
    COMMAND xy-compiler -f ${PROJECT_SOURCE_DIR}/src/demo/demo.xy)
set_tests_properties(cli_input_file PROPERTIES PASS_REGULAR_EXPRESSION "^11\n$")

add_test(NAME cli_missing_file
    COMMAND xy-compiler -f ${CMAKE_CURRENT_BINARY_DIR}/does-not-exist.xy)
set_tests_properties(cli_missing_file PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_emit_ir
    COMMAND xy-compiler --no-run -o - ${PROJECT_SOURCE_DIR}/src/demo/demo.xy)
set_tests_properties(cli_emit_ir PROPERTIES PASS_REGULAR_EXPRESSION "define internal i64 @sum\\(i64 %a, i64 %b\\)")

add_test(NAME cli_version COMMAND xy-compiler --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "libxy version: [^\n]+")

add_test(NAME cli_bad_option COMMAND xy-compiler --no-such-option)
set_tests_properties(cli_bad_option PROPERTIES WILL_FAIL TRUE)
