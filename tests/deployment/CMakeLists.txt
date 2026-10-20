cmake_minimum_required(VERSION 3.22)
project(JobTrackerDeploymentChecks LANGUAGES CXX)

# Standalone verification only: the production project never includes this tree.
if(NOT WIN32)
    message(FATAL_ERROR "These staging checks require Windows")
endif()
find_package(Qt6 6.10.3 EXACT REQUIRED COMPONENTS Quick Sql)
find_program(JOBTRACKER_POWERSHELL NAMES pwsh)
if(NOT JOBTRACKER_POWERSHELL)
    message(FATAL_ERROR "Deployment checks require PowerShell 7.2+ (pwsh). Install it or set JOBTRACKER_POWERSHELL to its executable.")
endif()
# find_program reuses cached values; validate the selected interpreter every time.
execute_process(
    COMMAND "${JOBTRACKER_POWERSHELL}" -NoLogo -NoProfile -NonInteractive
        -Command "$PSVersionTable.PSVersion.ToString()"
    RESULT_VARIABLE jobtracker_powershell_result
    OUTPUT_VARIABLE jobtracker_powershell_version
    ERROR_VARIABLE jobtracker_powershell_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
    TIMEOUT 15
)
if(NOT jobtracker_powershell_result STREQUAL "0"
        OR NOT jobtracker_powershell_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$"
        OR jobtracker_powershell_version VERSION_LESS "7.2")
    message(FATAL_ERROR
        "Deployment checks require PowerShell 7.2+ (pwsh). "
        "JOBTRACKER_POWERSHELL='${JOBTRACKER_POWERSHELL}' reported "
        "'${jobtracker_powershell_version}' (result: ${jobtracker_powershell_result}). "
        "${jobtracker_powershell_error} "
        "Reconfigure with -UJOBTRACKER_POWERSHELL or select a suitable executable.")
endif()
message(STATUS "Deployment checks use PowerShell ${jobtracker_powershell_version}: ${JOBTRACKER_POWERSHELL}")
set(JOBTRACKER_STAGE_DIR "" CACHE PATH "Existing clean Release staging prefix")
if(NOT IS_ABSOLUTE "${JOBTRACKER_STAGE_DIR}" OR
   NOT EXISTS "${JOBTRACKER_STAGE_DIR}/bin/JobTrackerApp.exe")
    message(FATAL_ERROR "JOBTRACKER_STAGE_DIR must identify an installed Release payload")
endif()

add_executable(JobTrackerDeploymentSmoke DeploymentSmoke.cpp)
target_compile_features(JobTrackerDeploymentSmoke PRIVATE cxx_std_20)
target_link_libraries(JobTrackerDeploymentSmoke PRIVATE Qt6::Quick Qt6::Sql)

enable_testing()
add_test(NAME JobTrackerReleaseStaging
    COMMAND "${JOBTRACKER_POWERSHELL}" -NoProfile -File
        "${CMAKE_CURRENT_SOURCE_DIR}/Verify-Staging.ps1"
        -StageDir "${JOBTRACKER_STAGE_DIR}"
        -Probe "$<TARGET_FILE:JobTrackerDeploymentSmoke>"
        -WorkRoot "${CMAKE_CURRENT_BINARY_DIR}/runs"
)
set_tests_properties(JobTrackerReleaseStaging PROPERTIES TIMEOUT 45)
