cmake_minimum_required(VERSION 3.21)
project(JobTrackerDeploymentChecks LANGUAGES CXX)

# Standalone verification only: the production project never includes this tree.
if(NOT WIN32)
    message(FATAL_ERROR "These staging checks require Windows")
endif()
find_package(Qt6 6.10.3 EXACT REQUIRED COMPONENTS Quick Sql)
find_program(JOBTRACKER_POWERSHELL NAMES pwsh powershell REQUIRED)
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
