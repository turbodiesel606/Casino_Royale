cmake_minimum_required(VERSION 3.22)

project(JobTracker
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JOBTRACKER_BUILD_TESTS "Build JobTracker tests" OFF)

set(JOBTRACKER_QT_VERSION 6.10.3)
find_package(Qt6 ${JOBTRACKER_QT_VERSION} EXACT REQUIRED COMPONENTS Quick Sql)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/opt/jobtracker" CACHE PATH "Install prefix" FORCE)
    endif()
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Private runtime library directory")
endif()

qt_standard_project_setup(REQUIRES ${JOBTRACKER_QT_VERSION})

add_library(JobTrackerLogging STATIC
    src/logging/ApplicationLog.cpp
    src/logging/ApplicationLog.hpp
    src/logging/CommonFacts.inc
    src/logging/LogCatalog.inc
    src/logging/LogEvent.cpp
    src/logging/LogEvent.hpp
    src/logging/LogFileWriter.cpp
    src/logging/LogFileWriter.hpp
    src/logging/LogInbox.cpp
    src/logging/LogInbox.hpp
    src/logging/PersistenceFacts.inc
    src/logging/StorageFacts.inc
    src/logging/WorkerFacts.inc
)
target_link_libraries(JobTrackerLogging PUBLIC Qt6::Core)
target_include_directories(JobTrackerLogging PUBLIC src)
target_compile_definitions(JobTrackerLogging PRIVATE JOBTRACKER_LOG_VERSION="${PROJECT_VERSION}")

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

set(JOBTRACKER_FIXED_STR_SOURCES
    src/fixed_str/detail/FixedStringImpl.hpp
    src/fixed_str/detail/FixedStringQtAdapter.hpp
    src/fixed_str/detail/FixedStringQtDetail.hpp
    src/fixed_str/FixedString.hpp
    src/fixed_str/FixedStringQt.hpp

)

set(JOBTRACKER_COMMON_LEAF_SOURCES
    src/common/CancellationState.cpp
    src/common/CancellationState.hpp
    src/common/ExceptionUtils.cpp
    src/common/ExceptionUtils.hpp
    src/common/ModelPresentation.cpp
    src/common/ModelPresentation.hpp
    src/common/ModelRoleUtils.cpp
    src/common/ModelRoleUtils.hpp
    src/common/SerialOperationQueue.hpp
    src/common/SingleActiveWorkerFacade.hpp
    src/common/SingleActiveWorkerRuntime.cpp
    src/common/SingleActiveWorkerRuntime.hpp
    src/common/TimeUtils.cpp
    src/common/TimeUtils.hpp
   
)

set(JOBTRACKER_COMMON_SOURCES
    ${JOBTRACKER_COMMON_LEAF_SOURCES}
    src/common/BulkIdSelectionTracker.cpp
    src/common/BulkIdSelectionTracker.hpp
    src/common/LimitedSortedProxyModel.cpp
    src/common/LimitedSortedProxyModel.hpp
    src/common/RelationFilterProxyModel.cpp
    src/common/RelationFilterProxyModel.hpp
    src/common/RoleFilterProxyModel.cpp
    src/common/RoleFilterProxyModel.hpp
    src/common/StableIdSelectionTracker.cpp
    src/common/StableIdSelectionTracker.hpp

)

set(JOBTRACKER_JOB_DOMAIN_SOURCES
    src/jobs/JobApplication.cpp
    src/jobs/JobApplication.hpp
    src/jobs/JobApplicationDraft.hpp
    src/jobs/JobApplicationFactory.cpp
    src/jobs/JobApplicationFactory.hpp
    src/jobs/JobApplicationValidator.cpp
    src/jobs/JobApplicationValidator.hpp
)

set(JOBTRACKER_JOB_SOURCES
    ${JOBTRACKER_JOB_DOMAIN_SOURCES}
    src/jobs/JobApplicationListModel.cpp
    src/jobs/JobApplicationListModel.hpp
    src/jobs/JobApplicationsController.cpp
    src/jobs/JobApplicationsController.hpp
)

set(JOBTRACKER_CV_PERSISTENCE_SOURCES
    src/cvs/CvPersistenceObservation.cpp
    src/cvs/CvPersistenceObservation.hpp
    src/cvs/CvImportService.cpp
    src/cvs/CvImportService.hpp
    src/cvs/CvImportExecutor.cpp
    src/cvs/CvImportExecutor.hpp
    src/cvs/CvImportWorker.cpp
    src/cvs/CvImportWorker.hpp
    src/cvs/CvImportWorkerTypes.hpp
    src/cvs/CvManagedFileStore.cpp
    src/cvs/CvManagedFileStore.hpp
    src/cvs/CvLockWrapper.hpp
    src/cvs/CvManagedPathResolver.cpp
    src/cvs/CvManagedPathResolver.hpp
    src/cvs/CvRepository.cpp
    src/cvs/CvRepository.hpp
)

set(JOBTRACKER_DIRECTORY_PERSISTENCE_SOURCES
    src/directory/CompanyRepository.cpp
    src/directory/CompanyRepository.hpp
)

set(JOBTRACKER_JOB_PERSISTENCE_SOURCES
    src/jobs/AddJobService.cpp
    src/jobs/AddJobService.hpp
    src/jobs/JobSaveExecutor.cpp
    src/jobs/JobSaveExecutor.hpp
    src/jobs/JobSaveWorker.cpp
    src/jobs/JobSaveWorker.hpp
    src/jobs/JobSaveWorkerTypes.hpp
    src/jobs/JobRepository.cpp
    src/jobs/JobRepository.hpp
    src/jobs/UpdateJobService.cpp
    src/jobs/UpdateJobService.hpp
)

set(JOBTRACKER_STORAGE_SOURCES
    src/storage/SchemaInitializationV4.cpp
    src/storage/SchemaMigrationSteps.hpp
    src/storage/SchemaMigrationV1ToV2.cpp
    src/storage/SchemaMigrationV2ToV3.cpp
    src/storage/SchemaMigrationV3ToV4.cpp
    src/storage/SchemaMigrator.cpp
    src/storage/SchemaMigrator.hpp
    src/storage/SqlQuery.cpp
    src/storage/SqlQuery.hpp
    src/storage/SqlTransaction.cpp
    src/storage/SqlTransaction.hpp
    src/storage/SqliteDatabase.cpp
    src/storage/SqliteDatabase.hpp
    src/storage/StoragePaths.cpp
    src/storage/StoragePaths.hpp
)

set(JOBTRACKER_PERSISTENCE_SOURCES
    ${JOBTRACKER_CV_PERSISTENCE_SOURCES}
    ${JOBTRACKER_DIRECTORY_PERSISTENCE_SOURCES}
    ${JOBTRACKER_JOB_PERSISTENCE_SOURCES}
    ${JOBTRACKER_STORAGE_SOURCES}
)

set(JOBTRACKER_MAINTENANCE_SOURCES
    src/maintenance/DataRemovalExecutor.cpp
    src/maintenance/DataRemovalExecutor.hpp
    src/maintenance/DataRemovalTypes.hpp
    src/maintenance/DataRemovalService.cpp
    src/maintenance/DataRemovalService.hpp
    src/maintenance/DataRemovalWorker.cpp
    src/maintenance/DataRemovalWorker.hpp
    src/maintenance/StorageMutationGate.cpp
    src/maintenance/StorageMutationGate.hpp
)

set(JOBTRACKER_CV_SOURCES
    src/cvs/CvDocument.hpp
    src/cvs/CvFileAccessService.cpp
    src/cvs/CvFileAccessService.hpp
    src/cvs/CvLibraryController.cpp
    src/cvs/CvLibraryController.hpp
    src/cvs/CvListModel.cpp
    src/cvs/CvListModel.hpp
)

set(JOBTRACKER_DASHBOARD_SOURCES
    src/dashboard/DashboardController.cpp
    src/dashboard/DashboardController.hpp
    src/dashboard/DashboardMetric.hpp
    src/dashboard/DashboardMetricListModel.cpp
    src/dashboard/DashboardMetricListModel.hpp
)

set(JOBTRACKER_DIRECTORY_SOURCES
    src/directory/Company.hpp
    src/directory/CompanyDirectoryController.cpp
    src/directory/CompanyDirectoryController.hpp
    src/directory/CompanyListModel.cpp
    src/directory/CompanyListModel.hpp
    src/directory/Contact.hpp
    src/directory/ContactDirectoryController.cpp
    src/directory/ContactDirectoryController.hpp
    src/directory/ContactInteractionListModel.cpp
    src/directory/ContactInteractionListModel.hpp
    src/directory/ContactListModel.cpp
    src/directory/ContactListModel.hpp
)

qt_add_executable(JobTrackerApp
    src/app/App.cpp
    src/app/App.hpp
    src/app/AppBootstrap.cpp
    src/app/AppBootstrap.hpp
    ${JOBTRACKER_COMMON_SOURCES}
    ${JOBTRACKER_CV_SOURCES}
    ${JOBTRACKER_DASHBOARD_SOURCES}
    ${JOBTRACKER_DIRECTORY_SOURCES}
    ${JOBTRACKER_JOB_SOURCES}
    ${JOBTRACKER_MAINTENANCE_SOURCES}
    ${JOBTRACKER_PERSISTENCE_SOURCES}
    ${JOBTRACKER_FIXED_STR_SOURCES}
    src/main.cpp
)

target_include_directories(JobTrackerApp
    PRIVATE src
)

if(WIN32)
    enable_language(RC)
    set(JOBTRACKER_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Installer/Windows/JobTracker.ico")
    set(JOBTRACKER_WINDOWS_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0")
    set(JOBTRACKER_WINDOWS_VERSION_TUPLE "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    set(JOBTRACKER_IDENTITY_DIR "${CMAKE_CURRENT_BINARY_DIR}/Installer")

    # Generate per configuration so resource flags and target filenames stay accurate.
    foreach(_identity_file IN ITEMS JobTracker.rc JobTrackerIdentity.cmake JobTrackerIdentity.iss)
        configure_file("Installer/Windows/${_identity_file}.in"
            "${JOBTRACKER_IDENTITY_DIR}/${_identity_file}.in" @ONLY)
        file(GENERATE OUTPUT "${JOBTRACKER_IDENTITY_DIR}/$<CONFIG>/${_identity_file}"
            INPUT "${JOBTRACKER_IDENTITY_DIR}/${_identity_file}.in")
    endforeach()
    target_sources(JobTrackerApp PRIVATE
        "${JOBTRACKER_IDENTITY_DIR}/$<CONFIG>/JobTracker.rc")
endif()

set(JOBTRACKER_QML_FILES
        qml/Main.qml
        qml/components/NeonCard.qml
        qml/components/DangerButton.qml
        qml/components/DestructiveConfirmationDialog.qml
        qml/components/StorageMutationFeedback.qml
        qml/components/Panel.qml
        qml/components/PrimaryButton.qml
        qml/components/SelectionCheckBox.qml
        qml/components/Sidebar.qml
        qml/components/SidebarButton.qml
        qml/components/StatusChip.qml
        qml/pages/CompaniesPage.qml
        qml/pages/ContactsPage.qml
        qml/pages/CvLibraryBrowserPane.qml
        qml/pages/CvLibraryPage.qml
        qml/pages/CvLibraryPreviewPanel.qml
        qml/pages/CvSearchField.qml
        qml/pages/DashboardActivityPanels.qml
        qml/pages/DashboardPage.qml
        qml/pages/DashboardRecentCvsPanel.qml
        qml/pages/DashboardStatsRow.qml
        qml/pages/JobDescriptionPane.qml
        qml/pages/JobFormPage.qml
        qml/pages/JobsApplicationsPane.qml
        qml/pages/JobsPage.qml
        qml/pages/MemoryPage.qml
        qml/pages/ProfilePage.qml
        qml/pages/SettingsPage.qml
)

qt_add_qml_module(JobTrackerApp
    URI JobTracker
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES ${JOBTRACKER_QML_FILES}
)

target_link_libraries(JobTrackerApp
    PRIVATE JobTrackerLogging Qt6::Quick Qt6::Sql
)

target_compile_definitions(JobTrackerApp PRIVATE
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

if(MSVC)
    target_compile_options(JobTrackerApp PRIVATE
        $<$<CONFIG:Debug>:/JMC>
    )
endif()

# Prepare the local development runtime only when explicitly requested.
# This target is not part of ALL and does not produce a release package.
if(WIN32)
    get_filename_component(_qt6_kit_dir "${Qt6_DIR}/../../.." REALPATH)
    set(_qt6_windeployqt "${_qt6_kit_dir}/bin/windeployqt.exe")
    if(NOT EXISTS "${_qt6_windeployqt}")
        message(FATAL_ERROR
            "The selected Qt ${JOBTRACKER_QT_VERSION} kit has no windeployqt: "
            "${_qt6_windeployqt}. Install that kit's deployment tools.")
    endif()

    # find_program trusts cached values, so validate them before searching.
    if(DEFINED WINDEPLOYQT_EXECUTABLE
            AND NOT WINDEPLOYQT_EXECUTABLE MATCHES "-NOTFOUND$")
        get_filename_component(_cached_windeployqt "${WINDEPLOYQT_EXECUTABLE}" REALPATH)
        get_filename_component(_selected_windeployqt "${_qt6_windeployqt}" REALPATH)
        string(TOLOWER "${_cached_windeployqt}" _cached_windeployqt)
        string(TOLOWER "${_selected_windeployqt}" _selected_windeployqt)
        if(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}"
                OR NOT _cached_windeployqt STREQUAL _selected_windeployqt)
            message(FATAL_ERROR
                "WINDEPLOYQT_EXECUTABLE='${WINDEPLOYQT_EXECUTABLE}' does not match "
                "the selected Qt kit (${_qt6_windeployqt}). Reconfigure with "
                "-UWINDEPLOYQT_EXECUTABLE to select this kit's tool.")
        endif()
    endif()
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt
        PATHS "${_qt6_kit_dir}/bin" NO_DEFAULT_PATH REQUIRED)
    set(WINDEPLOYQT_EXECUTABLE "${_qt6_windeployqt}")

    add_custom_target(JobTrackerApp_dev_runtime
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
            "$<TARGET_FILE:JobTrackerApp>"
        DEPENDS JobTrackerApp
        COMMENT "Preparing Qt runtime dependencies for local development"
        VERBATIM
    )
endif()

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_target(JobTrackerApp_dev_runtime_fresh
        COMMAND "${CMAKE_COMMAND}"
            "-DJOBTRACKER_BUILD_DIR=${CMAKE_BINARY_DIR}"
            "-DJOBTRACKER_CONFIG=$<CONFIG>"
            "-DJOBTRACKER_APP=$<TARGET_FILE:JobTrackerApp>"
            "-DJOBTRACKER_QML_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml"
            "-DWINDEPLOYQT_EXECUTABLE=${WINDEPLOYQT_EXECUTABLE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareFreshRuntime.cmake"
        DEPENDS JobTrackerApp
        COMMENT "Preparing a fresh local development runtime"
        VERBATIM
    )
endif()

if(JOBTRACKER_BUILD_TESTS)
    find_package(Qt6 ${JOBTRACKER_QT_VERSION} EXACT REQUIRED COMPONENTS Test)
    enable_testing()
    add_custom_target(JobTrackerTests)

    function(jobtracker_deploy_test_runtime target_name)
        add_dependencies(JobTrackerTests ${target_name})
        if(WIN32)
            # Each test owns its runtime files, including Qt Quick deployment.
            # A generator expression prevents an extra multi-config suffix.
            set_target_properties(${target_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY
                    "${CMAKE_CURRENT_BINARY_DIR}/test-runtime/$<CONFIG>/${target_name}"
            )
            # SQLite is loaded as a plugin, so copying QtSql alone is insufficient.
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::Core>"
                    "$<TARGET_FILE:Qt6::Gui>"
                    "$<TARGET_FILE:Qt6::Sql>"
                    "$<TARGET_FILE:Qt6::Test>"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target_name}>/sqldrivers"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::QSQLiteDriverPlugin>"
                    "$<TARGET_FILE_DIR:${target_name}>/sqldrivers"
                COMMENT "Deploying Qt test runtime dependencies for ${target_name}"
                VERBATIM
            )
        endif()
    endfunction()

    function(jobtracker_register_test target_name)
        target_link_libraries(${target_name} PRIVATE JobTrackerLogging)
        set(test_log "${CMAKE_CURRENT_BINARY_DIR}/test-logs/$<CONFIG>/${target_name}.txt")
        if(CMAKE_CONFIGURATION_TYPES)
            foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
                file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-logs/${config}")
            endforeach()
        else()
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-logs/${CMAKE_BUILD_TYPE}")
        endif()
        if(WIN32 AND target_name STREQUAL "JobTrackerMutationFeedbackTests")
            add_test(NAME ${target_name} COMMAND "${CMAKE_COMMAND}"
                "-DTEST_EXECUTABLE=$<TARGET_FILE:${target_name}>"
                "-DTEST_QML_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml"
                "-DTEST_QT_ROOT=${_qt6_kit_dir}"
                "-DTEST_WINDEPLOYQT=${WINDEPLOYQT_EXECUTABLE}"
                "-DTEST_OFFSCREEN=$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
                "-DTEST_LOG=${test_log}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/RunQmlTest.cmake")
        else()
            add_test(NAME ${target_name} COMMAND "${CMAKE_COMMAND}"
                "-DTEST_EXECUTABLE=$<TARGET_FILE:${target_name}>"
                "-DTEST_LOG=${test_log}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/RunQtTest.cmake")
        endif()
    endfunction()

    function(jobtracker_add_backend_test target_name test_source)
        add_executable(${target_name}
            ${test_source}
            ${JOBTRACKER_COMMON_LEAF_SOURCES}
            ${ARGN}
        )

        target_include_directories(${target_name}
            PRIVATE src
        )

        target_link_libraries(${target_name}
            PRIVATE Qt6::Core Qt6::Sql Qt6::Test
        )

        jobtracker_register_test(${target_name})
        jobtracker_deploy_test_runtime(${target_name})
    endfunction()

    add_executable(JobTrackerCommonTests
        tests/common/FilterAndValidationTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_JOB_DOMAIN_SOURCES}
    )

    target_include_directories(JobTrackerCommonTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerCommonTests
        PRIVATE Qt6::Core Qt6::Test
    )

    jobtracker_register_test(JobTrackerCommonTests)
    jobtracker_deploy_test_runtime(JobTrackerCommonTests)

    jobtracker_add_backend_test(
        JobTrackerRelationProxyTests
        tests/common/RelationFilterProxyModelTest.cpp
        src/common/RelationFilterProxyModel.cpp
        src/common/RelationFilterProxyModel.hpp
    )

    jobtracker_add_backend_test(
        JobTrackerSelectionTrackerTests
        tests/common/StableIdSelectionTrackerTest.cpp
        src/common/RoleFilterProxyModel.cpp
        src/common/RoleFilterProxyModel.hpp
        src/common/BulkIdSelectionTracker.cpp
        src/common/BulkIdSelectionTracker.hpp
        src/common/StableIdSelectionTracker.cpp
        src/common/StableIdSelectionTracker.hpp
    )

    jobtracker_add_backend_test(
        JobTrackerLogCatalogTests
        tests/logging/LogCatalogTest.cpp
    )

    jobtracker_add_backend_test(
        JobTrackerLogWriterTests
        tests/logging/LogWriterTest.cpp
    )

    jobtracker_add_backend_test(
        JobTrackerAsyncInfrastructureTests
        tests/common/AsyncInfrastructureTest.cpp
    )

    add_executable(JobTrackerJobsTests
        tests/jobs/JobApplicationsControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerJobsTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerJobsTests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )

    jobtracker_register_test(JobTrackerJobsTests)
    jobtracker_deploy_test_runtime(JobTrackerJobsTests)

    add_executable(JobTrackerCvTests
        tests/cvs/CvLibraryControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_CV_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerCvTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerCvTests
        PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )

    jobtracker_register_test(JobTrackerCvTests)
    jobtracker_deploy_test_runtime(JobTrackerCvTests)

    add_executable(JobTrackerDashboardTests
        tests/dashboard/DashboardControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_CV_SOURCES}
        ${JOBTRACKER_DASHBOARD_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerDashboardTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerDashboardTests
        PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )

    jobtracker_register_test(JobTrackerDashboardTests)
    jobtracker_deploy_test_runtime(JobTrackerDashboardTests)

    add_executable(JobTrackerDirectoryTests
        tests/directory/DirectoryControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_DIRECTORY_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerDirectoryTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerDirectoryTests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )

    jobtracker_register_test(JobTrackerDirectoryTests)
    jobtracker_deploy_test_runtime(JobTrackerDirectoryTests)

    jobtracker_add_backend_test(
        JobTrackerMigrationTests
        tests/storage/MigrationTest.cpp
        ${JOBTRACKER_JOB_DOMAIN_SOURCES}
        src/cvs/CvRepository.cpp
        src/cvs/CvRepository.hpp
        ${JOBTRACKER_DIRECTORY_PERSISTENCE_SOURCES}
        src/jobs/JobRepository.cpp
        src/jobs/JobRepository.hpp
        ${JOBTRACKER_STORAGE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerRepositoryTests
        tests/storage/RepositoryTest.cpp
        src/cvs/CvRepository.cpp
        src/cvs/CvRepository.hpp
        ${JOBTRACKER_STORAGE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerCvImportTests
        tests/storage/CvFileStoreImportTest.cpp
        ${JOBTRACKER_CV_PERSISTENCE_SOURCES}
        ${JOBTRACKER_STORAGE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerCvLockTests
        tests/cvs/CvLockWrapperTest.cpp
        src/cvs/CvLockWrapper.hpp
    )
    set_tests_properties(JobTrackerCvLockTests PROPERTIES TIMEOUT 15)

    jobtracker_add_backend_test(
        JobTrackerAddJobTests
        tests/storage/AddJobServiceTest.cpp
        ${JOBTRACKER_JOB_DOMAIN_SOURCES}
        ${JOBTRACKER_CV_PERSISTENCE_SOURCES}
        ${JOBTRACKER_DIRECTORY_PERSISTENCE_SOURCES}
        ${JOBTRACKER_JOB_PERSISTENCE_SOURCES}
        ${JOBTRACKER_STORAGE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerUpdateJobTests
        tests/storage/UpdateJobServiceTest.cpp
        ${JOBTRACKER_JOB_DOMAIN_SOURCES}
        ${JOBTRACKER_CV_PERSISTENCE_SOURCES}
        ${JOBTRACKER_DIRECTORY_PERSISTENCE_SOURCES}
        ${JOBTRACKER_JOB_PERSISTENCE_SOURCES}
        ${JOBTRACKER_STORAGE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerRemovalTests
        tests/storage/DataRemovalServiceTest.cpp
        ${JOBTRACKER_JOB_DOMAIN_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    jobtracker_add_backend_test(
        JobTrackerMutationGateTests
        tests/maintenance/StorageMutationGateTest.cpp
        src/maintenance/StorageMutationGate.cpp
        src/maintenance/StorageMutationGate.hpp
    )

    jobtracker_add_backend_test(
        JobTrackerMutationFeedbackTests
        tests/maintenance/StorageMutationFeedbackTest.cpp
        src/app/AppBootstrap.cpp
        src/app/AppBootstrap.hpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_CV_SOURCES}
        ${JOBTRACKER_DASHBOARD_SOURCES}
        ${JOBTRACKER_DIRECTORY_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )
    target_link_libraries(JobTrackerMutationFeedbackTests PRIVATE Qt6::Quick)
    qt_add_resources(JobTrackerMutationFeedbackTests startup_qml
        PREFIX "/JobTracker"
        FILES ${JOBTRACKER_QML_FILES}
    )
    set_tests_properties(JobTrackerMutationFeedbackTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
        TIMEOUT 30
    )
    if(WIN32)
        set_property(TEST JobTrackerMutationFeedbackTests APPEND PROPERTY
            ENVIRONMENT "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
    endif()

    jobtracker_add_backend_test(
        JobTrackerIntegrationTests
        tests/storage/EndToEndIntegrationTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_CV_SOURCES}
        ${JOBTRACKER_DIRECTORY_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )
    target_link_libraries(JobTrackerIntegrationTests PRIVATE Qt6::Gui)
endif()

include(GNUInstallDirs)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(Installer/Linux/Runtime.cmake)
endif()

install(TARGETS JobTrackerApp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# The common Release payload excludes MSVC runtime policy.
# Keep deployment in the install lifecycle and leave Debug installs unchanged.
if(WIN32)
    qt_generate_deploy_qml_app_script(
        TARGET JobTrackerApp
        OUTPUT_SCRIPT JOBTRACKER_WINDOWS_DEPLOY_SCRIPT
        NO_COMPILER_RUNTIME
        EXCLUDE_PLUGIN_TYPES qmltooling designer qmllint qmlls platforminputcontexts
        EXCLUDE_PLUGINS qsqlibase qsqlmimer qsqloci qsqlodbc qsqlpsql
    )
    install(CODE "
        if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$\")
            include(\"${JOBTRACKER_WINDOWS_DEPLOY_SCRIPT}\")
        endif()
    ")
endif()

# Retain Linux runtime installation as an explicit install operation.
# Normal builds leave existing deployment trees untouched.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(SCRIPT "${JOBTRACKER_LINUX_DEPLOY_SCRIPT}")
    install(SCRIPT "${JOBTRACKER_LINUX_VERIFY_SCRIPT}")
    include(Installer/Linux/Desktop.cmake)
    option(JOBTRACKER_ENABLE_DEB_PACKAGING "Enable explicit native DEB packaging" OFF)
    if(JOBTRACKER_ENABLE_DEB_PACKAGING)
        include(Installer/Linux/Packaging.cmake)
    endif()
endif()
