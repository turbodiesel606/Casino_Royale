cmake_minimum_required(VERSION 3.22)

# CTest owns preparation: QML-only changes must not require relinking the test.
foreach(required IN ITEMS TEST_EXECUTABLE TEST_QML_DIR TEST_QT_ROOT
        TEST_WINDEPLOYQT TEST_OFFSCREEN TEST_LOG)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(REAL_PATH "${TEST_QT_ROOT}/bin/windeployqt.exe" selected_tool)
file(REAL_PATH "${TEST_WINDEPLOYQT}" actual_tool)
string(TOLOWER "${selected_tool}" selected_tool)
string(TOLOWER "${actual_tool}" actual_tool)
if(NOT EXISTS "${TEST_WINDEPLOYQT}" OR NOT actual_tool STREQUAL selected_tool)
    message(FATAL_ERROR "Test deployment tool does not belong to the selected Qt kit")
endif()
if(NOT EXISTS "${TEST_EXECUTABLE}")
    message(FATAL_ERROR "Build JobTrackerMutationFeedbackTests before running CTest")
endif()
get_filename_component(runtime "${TEST_EXECUTABLE}" DIRECTORY)
file(LOCK "${runtime}/qml-runtime.lock" GUARD PROCESS TIMEOUT 10)

# Scan the complete project QML subtree, including nested local components and
# module metadata. This is a runtime input manifest, not source registration.
file(GLOB_RECURSE qml_inputs LIST_DIRECTORIES FALSE RELATIVE "${TEST_QML_DIR}"
    "${TEST_QML_DIR}/*.qml" "${TEST_QML_DIR}/*.js" "${TEST_QML_DIR}/*.mjs"
    "${TEST_QML_DIR}/*.qmltypes" "${TEST_QML_DIR}/qmldir")
list(SORT qml_inputs)
set(inputs "qt=${TEST_QT_ROOT}\n")
foreach(path IN LISTS qml_inputs)
    file(SHA256 "${TEST_QML_DIR}/${path}" hash)
    string(APPEND inputs "${path}=${hash}\n")
endforeach()
foreach(path IN ITEMS "${TEST_EXECUTABLE}" "${TEST_WINDEPLOYQT}"
        "${TEST_OFFSCREEN}" "${CMAKE_CURRENT_LIST_FILE}")
    file(SHA256 "${path}" hash)
    string(APPEND inputs "${path}=${hash}\n")
endforeach()
string(SHA256 fingerprint "${inputs}")
set(stamp "${runtime}/qml-runtime.sha256")
set(previous "")
if(EXISTS "${stamp}")
    file(READ "${stamp}" previous)
endif()
if(NOT previous STREQUAL fingerprint)
    message(STATUS "Preparing QML test runtime")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        --unset=QML_IMPORT_PATH --unset=QML2_IMPORT_PATH --unset=QT_PLUGIN_PATH
        --unset=QT_QPA_PLATFORM_PLUGIN_PATH "${TEST_WINDEPLOYQT}"
        --qmldir "${TEST_QML_DIR}" "${TEST_EXECUTABLE}"
        COMMAND_ERROR_IS_FATAL ANY)
    file(MAKE_DIRECTORY "${runtime}/platforms")
    file(COPY "${TEST_OFFSCREEN}" DESTINATION "${runtime}/platforms")
    # Prevent a missing private import from falling back to the developer SDK.
    file(WRITE "${runtime}/qt.conf" "[Paths]\nPrefix=.\nPlugins=.\nQmlImports=qml\n")
    file(WRITE "${runtime}/qml-runtime-inputs.txt" "${inputs}")
    file(WRITE "${stamp}" "${fingerprint}")
else()
    message(STATUS "QML test runtime unchanged")
endif()
set(TEST_ENVIRONMENT_OPTIONS
    --unset=QML_IMPORT_PATH --unset=QML2_IMPORT_PATH --unset=QT_PLUGIN_PATH
    --unset=QT_QPA_PLATFORM_PLUGIN_PATH)
include("${CMAKE_CURRENT_LIST_DIR}/RunQtTest.cmake")
