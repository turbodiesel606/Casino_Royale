# This file registers explicit installation only; normal builds do not deploy.
set_target_properties(JobTrackerApp PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH_USE_LINK_PATH FALSE
)

# Run before install(TARGETS), including when CPack supplies DESTDIR later.
install(CODE "
    if(NOT CMAKE_INSTALL_PREFIX STREQUAL \"/opt/jobtracker\"
            OR NOT \"${CMAKE_INSTALL_BINDIR}\" STREQUAL \"bin\"
            OR NOT \"${CMAKE_INSTALL_LIBDIR}\" STREQUAL \"lib\")
        message(FATAL_ERROR \"Linux runtime installation requires /opt/jobtracker with bin/ and lib/.\")
    endif()
    if(NOT IS_ABSOLUTE \"\$ENV{DESTDIR}\" OR \"\$ENV{DESTDIR}\" STREQUAL \"/\")
        message(FATAL_ERROR \"Set DESTDIR to a separate absolute staging root; use Installer/Linux/Stage.sh.\")
    endif()
    if(${JOBTRACKER_BUILD_TESTS})
        message(FATAL_ERROR \"Deploy from a production build with JOBTRACKER_BUILD_TESTS=OFF.\")
    endif()
")

# Qt 6.10.3's Linux per-plugin exclusion is ineffective. Select needed plugins
# by excluding whole categories and explicitly including the desired names.
# TIFF needs libtiff.so.5, absent on Ubuntu 24.04; the app has no TIFF workflow.
# Keep desktop input contexts/themes, without the unused virtual keyboard.
# Leave POST_EXCLUDE_REGEXES unset to preserve Qt's system-library exclusions.
qt_generate_deploy_qml_app_script(
    TARGET JobTrackerApp
    OUTPUT_SCRIPT JOBTRACKER_LINUX_DEPLOY_SCRIPT
    EXCLUDE_PLUGIN_TYPES
        sqldrivers imageformats platforminputcontexts vectorimageformats
        qmltooling designer qmllint qmlls egldeviceintegrations
    INCLUDE_PLUGINS
        qsqlite qxcb qwayland qgif qico qjpeg qsvg qwebp
        composeplatforminputcontextplugin ibusplatforminputcontextplugin
)

get_filename_component(JOBTRACKER_QT_KIT "${Qt6_DIR}/../../.." ABSOLUTE)
set(JOBTRACKER_QT_LICENSE_FILE "${JOBTRACKER_QT_KIT}/../../Licenses/LICENSE"
    CACHE FILEPATH "Qt license text from the matching Online Installer SDK")
set(JOBTRACKER_LINUX_VERIFY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/VerifyRuntime.cmake")
configure_file("${CMAKE_CURRENT_LIST_DIR}/VerifyRuntime.cmake.in"
    "${JOBTRACKER_LINUX_VERIFY_SCRIPT}" @ONLY)
