# Register explicit installation and development runtime preparation.
# Building JobTrackerApp alone does not deploy.
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

# Qt 6.10.3 hashes the generated script content, not the OUTPUT_SCRIPT variable
# name. Different plugin policies produce separate files for this same target.
# Keep tooling in development copies; QT_QML_DEBUG itself remains Debug-only.
qt_generate_deploy_qml_app_script(
    TARGET JobTrackerApp
    OUTPUT_SCRIPT JOBTRACKER_LINUX_DEV_DEPLOY_SCRIPT
    EXCLUDE_PLUGIN_TYPES
        sqldrivers imageformats platforminputcontexts vectorimageformats
        designer qmllint qmlls egldeviceintegrations
    INCLUDE_PLUGIN_TYPES qmltooling
    INCLUDE_PLUGINS
        qsqlite qxcb qwayland qgif qico qjpeg qsvg qwebp
        composeplatforminputcontextplugin ibusplatforminputcontextplugin
)
if(JOBTRACKER_LINUX_DEV_DEPLOY_SCRIPT STREQUAL JOBTRACKER_LINUX_DEPLOY_SCRIPT)
    message(FATAL_ERROR "Development and production deployment scripts must be distinct.")
endif()

# A separate opt-in component allows Debug and test-enabled development trees.
# Full installation and CPack exclude it and retain the packaging guards above.
# Installing the executable also changes its RPATH to the private ../lib folder.
install(CODE "
    if(NOT \"${CMAKE_INSTALL_BINDIR}\" STREQUAL \"bin\"
            OR NOT \"${CMAKE_INSTALL_LIBDIR}\" STREQUAL \"lib\")
        message(FATAL_ERROR \"Linux development runtime requires bin/ and lib/.\")
    endif()
" COMPONENT JobTrackerDevelopmentRuntime EXCLUDE_FROM_ALL)

install(TARGETS JobTrackerApp
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT JobTrackerDevelopmentRuntime
    EXCLUDE_FROM_ALL
)
install(SCRIPT "${JOBTRACKER_LINUX_DEV_DEPLOY_SCRIPT}"
    COMPONENT JobTrackerDevelopmentRuntime
    EXCLUDE_FROM_ALL
)

# Ignore an inherited packaging DESTDIR for this build-local development copy.
# Keep configurations separate and preserve existing files between invocations.
add_custom_target(JobTrackerApp_dev_runtime
    COMMAND "${CMAKE_COMMAND}" -E env --unset=DESTDIR
        "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
        --config "$<CONFIG>"
        --prefix "${CMAKE_CURRENT_BINARY_DIR}/dev-runtime/$<CONFIG>"
        --component JobTrackerDevelopmentRuntime
    DEPENDS JobTrackerApp
    COMMENT "Preparing Qt runtime dependencies for local Linux development"
    VERBATIM
)

get_filename_component(JOBTRACKER_QT_KIT "${Qt6_DIR}/../../.." ABSOLUTE)
set(JOBTRACKER_QT_LICENSE_FILE "${JOBTRACKER_QT_KIT}/../../Licenses/LICENSE"
    CACHE FILEPATH "Qt license text from the matching Online Installer SDK")
set(JOBTRACKER_LINUX_VERIFY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Installer/Linux/VerifyRuntime.cmake")
configure_file("${CMAKE_CURRENT_LIST_DIR}/VerifyRuntime.cmake.in"
    "${JOBTRACKER_LINUX_VERIFY_SCRIPT}" @ONLY)
