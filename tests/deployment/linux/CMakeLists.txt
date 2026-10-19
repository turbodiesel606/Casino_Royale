cmake_minimum_required(VERSION 3.22)
project(JobTrackerLinuxDeploymentProbe LANGUAGES CXX)

# Standalone consumer only; never include this project from production CMake.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This deployment probe requires Linux")
endif()
find_package(Qt6 6.10.3 EXACT REQUIRED COMPONENTS Core Sql)

add_executable(JobTrackerSqlProbe SqlProbe.cpp)
target_compile_features(JobTrackerSqlProbe PRIVATE cxx_std_20)
target_link_libraries(JobTrackerSqlProbe PRIVATE Qt6::Core Qt6::Sql)
# Run beside the packaged qt.conf, using only the package-relative library path.
set_target_properties(JobTrackerSqlProbe PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH_USE_LINK_PATH FALSE
)
