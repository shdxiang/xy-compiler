# Runs one test program and checks the result against the directives in its
# comments:
#   // expect-stdout: <line>   (repeatable; stdout must match exactly)
#   // expect-stderr: <text>   (stderr must contain it; otherwise stderr must be empty)
#   // expect-exit: <code>     (default 0)
#
# Usage: cmake -DCOMPILER=<path> -DSOURCE=<file.xy> [-DEXPECT_STDOUT=<text>] -P run_test.cmake

file(READ "${SOURCE}" content)
string(REGEX MATCHALL "// expect-[a-z]+: [^\r\n]*" directives "${content}")

set(expected_stdout "")
set(expected_stderr "")
set(expected_exit 0)
foreach(directive IN LISTS directives)
    if(directive MATCHES "^// expect-stdout: (.*)$")
        string(APPEND expected_stdout "${CMAKE_MATCH_1}\n")
    elseif(directive MATCHES "^// expect-stderr: (.*)$")
        set(expected_stderr "${CMAKE_MATCH_1}")
    elseif(directive MATCHES "^// expect-exit: (.*)$")
        set(expected_exit "${CMAKE_MATCH_1}")
    endif()
endforeach()
if(DEFINED EXPECT_STDOUT)
    set(expected_stdout "${EXPECT_STDOUT}\n")
endif()

execute_process(
    COMMAND "${COMPILER}"
    INPUT_FILE "${SOURCE}"
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
    RESULT_VARIABLE actual_exit)

set(failures "")
if(NOT actual_exit STREQUAL expected_exit)
    string(APPEND failures "exit code: expected ${expected_exit}, got ${actual_exit}\n")
endif()
if(NOT actual_stdout STREQUAL expected_stdout)
    string(APPEND failures "stdout: expected\n${expected_stdout}got\n${actual_stdout}\n")
endif()
if(expected_stderr STREQUAL "")
    if(NOT actual_stderr STREQUAL "")
        string(APPEND failures "unexpected stderr:\n${actual_stderr}\n")
    endif()
else()
    string(FIND "${actual_stderr}" "${expected_stderr}" position)
    if(position EQUAL -1)
        string(APPEND failures "stderr: expected to contain\n${expected_stderr}\ngot\n${actual_stderr}\n")
    endif()
endif()

if(NOT failures STREQUAL "")
    message(FATAL_ERROR "${SOURCE}:\n${failures}")
endif()
