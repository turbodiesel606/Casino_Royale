cmake_minimum_required(VERSION 3.22)

foreach(_required JOBTRACKER_BUILD_DIR JOBTRACKER_CONFIG JOBTRACKER_APP JOBTRACKER_QML_DIR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "Fresh runtime preparation requires ${_required}.")
    endif()
endforeach()

if(NOT EXISTS "${JOBTRACKER_APP}")
    message(FATAL_ERROR "Build the application before preparing its runtime: ${JOBTRACKER_APP}")
endif()

# Use only the current executable and SDK dependencies, never an old runtime.
# A new path on every invocation preserves previous successful and failed copies.
string(TIMESTAMP _timestamp "%Y%m%d-%H%M%S" UTC)
set(_runtime_root "${JOBTRACKER_BUILD_DIR}/dev-runtime-fresh/${JOBTRACKER_CONFIG}")
while(TRUE)
    string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef _suffix)
    set(_runtime_dir "${_runtime_root}/${_timestamp}-${_suffix}")
    if(NOT EXISTS "${_runtime_dir}")
        break()
    endif()
endwhile()
file(MAKE_DIRECTORY "${_runtime_dir}")
message(STATUS "Fresh development runtime directory: ${_runtime_dir}")

if(CMAKE_HOST_WIN32)
    if(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}")
        message(FATAL_ERROR "The selected Qt kit's windeployqt is unavailable.")
    endif()
    get_filename_component(_app_name "${JOBTRACKER_APP}" NAME)
    file(COPY "${JOBTRACKER_APP}" DESTINATION "${_runtime_dir}")
    set(_runtime_app "${_runtime_dir}/${_app_name}")
    execute_process(
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --qmldir "${JOBTRACKER_QML_DIR}"
            "${_runtime_app}"
        RESULT_VARIABLE _result
    )
    set(_launch "PowerShell: & '${_runtime_app}'")
else()
    # Clear DESTDIR only in the child, as for the ordinary development target.
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=DESTDIR
            "${CMAKE_COMMAND}" --install "${JOBTRACKER_BUILD_DIR}"
            --config "${JOBTRACKER_CONFIG}"
            --prefix "${_runtime_dir}"
            --component JobTrackerDevelopmentRuntime
        RESULT_VARIABLE _result
    )
    set(_runtime_app "${_runtime_dir}/bin/JobTrackerApp")
    set(_launch "Bash: '${_runtime_app}'")
endif()

if(NOT _result STREQUAL "0")
    message(FATAL_ERROR "Runtime preparation failed (${_result}); preserved ${_runtime_dir}")
endif()
message(STATUS "Fresh development runtime ready: ${_runtime_dir}")
message(STATUS "Launch with ${_launch}")
