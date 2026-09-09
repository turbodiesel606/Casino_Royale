cmake_minimum_required(VERSION 3.21)

project(JobTracker
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JOBTRACKER_BUILD_TESTS "Build JobTracker tests" OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Sql)

qt_standard_project_setup(REQUIRES 6.5)

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

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
    src/main.cpp
)

target_include_directories(JobTrackerApp
    PRIVATE src
)

qt_add_qml_module(JobTrackerApp
    URI JobTracker
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/Main.qml
        qml/components/NeonCard.qml
        qml/components/DangerButton.qml
        qml/components/DestructiveConfirmationDialog.qml
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

target_link_libraries(JobTrackerApp
    PRIVATE Qt6::Quick Qt6::Sql
)

target_compile_definitions(JobTrackerApp PRIVATE
    $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

if(MSVC)
    target_compile_options(JobTrackerApp PRIVATE
        $<$<CONFIG:Debug>:/JMC>
    )
endif()

# Windows deployment remains unchanged: windeployqt copies the runtime next to
# the executable in the Windows build directory.
if(WIN32)
    if(TARGET Qt6::qmake)
        get_target_property(_qt6_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt6_bin_dir "${_qt6_qmake_executable}" DIRECTORY)
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt6_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET JobTrackerApp POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                "$<TARGET_FILE:JobTrackerApp>"
            COMMENT "Deploying Qt runtime dependencies with windeployqt"
        )
    else()
        message(WARNING "windeployqt was not found; Qt runtime dependencies will not be deployed.")
    endif()
endif()

# On Linux, a normal build also creates a self-contained deploy directory.
# Usage stays exactly:
#   cmake --preset linux-debug-local
#   cmake --build --preset linux-debug-local
# The second command produces ${CMAKE_SOURCE_DIR}/deploy automatically.
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(TARGETS JobTrackerApp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    qt_generate_deploy_qml_app_script(
        TARGET JobTrackerApp
        OUTPUT_SCRIPT JOBTRACKER_LINUX_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    install(SCRIPT ${JOBTRACKER_LINUX_DEPLOY_SCRIPT})

    add_custom_command(TARGET JobTrackerApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_SOURCE_DIR}/deploy"
        COMMAND ${CMAKE_COMMAND}
            --install "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --prefix "${CMAKE_SOURCE_DIR}/deploy"
        COMMENT "Deploying JobTracker and Qt runtime dependencies to ${CMAKE_SOURCE_DIR}/deploy"
        VERBATIM
    )
endif()

if(JOBTRACKER_BUILD_TESTS)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    enable_testing()

    function(jobtracker_deploy_test_runtime target_name)
        if(WIN32)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::Core>"
                    "$<TARGET_FILE:Qt6::Gui>"
                    "$<TARGET_FILE:Qt6::Sql>"
                    "$<TARGET_FILE:Qt6::Test>"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Deploying Qt test runtime dependencies for ${target_name}"
            )
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

        add_test(NAME ${target_name} COMMAND ${target_name})
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

    add_test(NAME JobTrackerCommonTests COMMAND JobTrackerCommonTests)
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

    add_test(NAME JobTrackerJobsTests COMMAND JobTrackerJobsTests)
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

    add_test(NAME JobTrackerCvTests COMMAND JobTrackerCvTests)
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

    add_test(NAME JobTrackerDashboardTests COMMAND JobTrackerDashboardTests)
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

    add_test(NAME JobTrackerDirectoryTests COMMAND JobTrackerDirectoryTests)
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
        JobTrackerCvMutationQueueTests
        tests/cvs/CvMutationQueueTest.cpp
        src/cvs/CvLockWrapper.hpp
    )
    set_tests_properties(JobTrackerCvMutationQueueTests PROPERTIES TIMEOUT 15)

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
        JobTrackerIntegrationTests
        tests/storage/EndToEndIntegrationTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_DIRECTORY_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_MAINTENANCE_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )
endif()

include(GNUInstallDirs)

install(TARGETS JobTrackerApp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
