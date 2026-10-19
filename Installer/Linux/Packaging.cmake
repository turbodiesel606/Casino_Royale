# Packaging is opt-in; ordinary builds do not require Debian tools or metadata.
if(CMAKE_CROSSCOMPILING OR CMAKE_CONFIGURATION_TYPES
        OR NOT CMAKE_BUILD_TYPE STREQUAL "Release" OR JOBTRACKER_BUILD_TESTS)
    message(FATAL_ERROR "DEB packaging requires a native single-config production Release build with tests OFF.")
endif()
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/opt/jobtracker"
        OR NOT CMAKE_INSTALL_BINDIR STREQUAL "bin"
        OR NOT CMAKE_INSTALL_LIBDIR STREQUAL "lib")
    message(FATAL_ERROR "DEB packaging requires /opt/jobtracker with bin/ and lib/.")
endif()

# Dependencies below target the confirmed Ubuntu release, not arbitrary Linux.
file(STRINGS "/etc/os-release" jobtracker_os)
if(NOT "ID=ubuntu" IN_LIST jobtracker_os OR NOT "VERSION_ID=\"24.04\"" IN_LIST jobtracker_os)
    message(FATAL_ERROR "The DEB workflow currently targets native Ubuntu 24.04 only.")
endif()
find_program(JOBTRACKER_DPKG_EXECUTABLE dpkg REQUIRED)
find_program(JOBTRACKER_DPKG_DEB_EXECUTABLE dpkg-deb REQUIRED)
find_program(JOBTRACKER_SHLIBDEPS_EXECUTABLE dpkg-shlibdeps REQUIRED)
find_program(JOBTRACKER_READELF_EXECUTABLE readelf REQUIRED)
find_program(JOBTRACKER_FILE_EXECUTABLE file REQUIRED)
execute_process(COMMAND "${JOBTRACKER_DPKG_EXECUTABLE}" --print-architecture
    OUTPUT_VARIABLE jobtracker_deb_arch OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
if(NOT jobtracker_deb_arch STREQUAL "amd64" OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    message(FATAL_ERROR "Only the verified native Ubuntu 24.04 amd64 target is supported.")
endif()

set(JOBTRACKER_DEB_MAINTAINER "" CACHE STRING "Confirmed DEB maintainer: Name <email>")
set(JOBTRACKER_DEB_REVISION "1" CACHE STRING "Debian packaging revision, separate from PROJECT_VERSION")
option(JOBTRACKER_DEB_LOCAL_TEST "Build a local-test package with unresolved release metadata" OFF)
if(NOT JOBTRACKER_DEB_REVISION MATCHES "^[0-9][A-Za-z0-9.+]*$")
    message(FATAL_ERROR "Use a Debian revision starting with a digit and containing only letters, digits, dot or plus.")
endif()

set(CPACK_PACKAGE_NAME "jobtracker")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JobTracker job application tracker")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "Track job applications with a private Qt 6.10.3 runtime on Ubuntu 24.04 amd64.")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${JOBTRACKER_DEB_REVISION}")
if(JOBTRACKER_DEB_LOCAL_TEST)
    string(APPEND CPACK_DEBIAN_PACKAGE_RELEASE "~localtest")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Unspecified local-test maintainer <local-test@invalid>")
    string(APPEND CPACK_PACKAGE_DESCRIPTION_SUMMARY " (local-test, not a release)")
    string(APPEND CPACK_DEBIAN_PACKAGE_DESCRIPTION
        " Local engineering package only; release maintainer and notice review remain unresolved.")
elseif(NOT JOBTRACKER_DEB_MAINTAINER MATCHES "^[^<>;\n\r]+ <[^<> ;\n\r]+@[^<> ;\n\r]+>$")
    message(FATAL_ERROR "Set JOBTRACKER_DEB_MAINTAINER to a confirmed Name <email>, or explicitly enable JOBTRACKER_DEB_LOCAL_TEST.")
else()
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${JOBTRACKER_DEB_MAINTAINER}")
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "")
set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${jobtracker_deb_arch}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_FILE_NAME
    "jobtracker_${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${jobtracker_deb_arch}")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_THREADS 1)
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/jobtracker")
set(CPACK_STRIP_FILES OFF)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Qt's OpenSSL backend dlopens these libraries; DT_NEEDED cannot infer them.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3t64 (>= 3.0.0)")
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_CHECKSUM SHA256)

foreach(script IN ITEMS PackageGuard CheckDependencies CheckPackage)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/${script}.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/${script}.cmake" @ONLY)
endforeach()
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/PackageGuard.cmake")
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/CheckDependencies.cmake")
set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/CheckPackage.cmake")
include(CPack)
