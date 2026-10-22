cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED TEST_LOG)
    message(FATAL_ERROR "TEST_EXECUTABLE and TEST_LOG are required")
endif()
get_filename_component(log_dir "${TEST_LOG}" DIRECTORY)
file(MAKE_DIRECTORY "${log_dir}")
file(LOCK "${TEST_LOG}.lock" GUARD PROCESS TIMEOUT 10)
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef invocation)
set(invocation_log "${TEST_LOG}.${invocation}.txt")
message(STATUS "Qt Test invocation log (if initialized): ${invocation_log}")
# A unique file distinguishes this run from a prior log after a startup crash.
# TEST_ENVIRONMENT_OPTIONS is set only by the QML runtime preparation wrapper.
execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${TEST_ENVIRONMENT_OPTIONS}
    "${TEST_EXECUTABLE}" -o "${invocation_log},txt" -o -,txt -v1
    OUTPUT_VARIABLE console ERROR_VARIABLE errors RESULT_VARIABLE result
    ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE)
if(EXISTS "${invocation_log}")
    file(READ "${invocation_log}" diagnostics)
    file(RENAME "${invocation_log}" "${TEST_LOG}")
    if(console STREQUAL "" AND NOT diagnostics STREQUAL "")
        message("${diagnostics}")
    endif()
else()
    message("Qt Test produced no log for this invocation; any previous log is unchanged. See process/CTest diagnostics.")
endif()
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Qt Test process failed (${result}). See the output above and current Qt Test log, when produced: ${TEST_LOG}")
endif()
