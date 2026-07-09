cmake_minimum_required(VERSION 3.21)

project(JobTracker
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JOBTRACKER_BUILD_TESTS "Build JobTracker tests" OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Sql Test)

qt_standard_project_setup(REQUIRES 6.5)

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

set(JOBTRACKER_COMMON_SOURCES
    src/common/RoleFilterProxyModel.cpp
    src/common/RoleFilterProxyModel.h
    src/common/ValidationResult.h
    src/common/ValidationService.cpp
    src/common/ValidationService.h
)

set(JOBTRACKER_JOB_SOURCES
    src/jobs/JobApplication.h
    src/jobs/JobApplicationDraft.h
    src/jobs/JobApplicationListModel.cpp
    src/jobs/JobApplicationListModel.h
    src/jobs/JobApplicationsController.cpp
    src/jobs/JobApplicationsController.h
)

set(JOBTRACKER_PERSISTENCE_SOURCES
    src/cvs/CvImportService.cpp
    src/cvs/CvImportService.h
    src/cvs/CvRepository.cpp
    src/cvs/CvRepository.h
    src/jobs/AddJobService.cpp
    src/jobs/AddJobService.h
    src/jobs/JobRepository.cpp
    src/jobs/JobRepository.h
    src/storage/SchemaMigrator.cpp
    src/storage/SchemaMigrator.h
    src/storage/SqliteDatabase.cpp
    src/storage/SqliteDatabase.h
    src/storage/StoragePaths.cpp
    src/storage/StoragePaths.h
)

set(JOBTRACKER_CV_SOURCES
    src/cvs/CvDocument.h
    src/cvs/CvLibraryController.cpp
    src/cvs/CvLibraryController.h
    src/cvs/CvListModel.cpp
    src/cvs/CvListModel.h
    src/cvs/LinkedApplicationListModel.cpp
    src/cvs/LinkedApplicationListModel.h
)

set(JOBTRACKER_DASHBOARD_SOURCES
    src/dashboard/DashboardController.cpp
    src/dashboard/DashboardController.h
    src/dashboard/DashboardMetric.h
    src/dashboard/DashboardMetricListModel.cpp
    src/dashboard/DashboardMetricListModel.h
    src/dashboard/DashboardRecentApplicationsModel.cpp
    src/dashboard/DashboardRecentApplicationsModel.h
    src/dashboard/DashboardRecentCvsModel.cpp
    src/dashboard/DashboardRecentCvsModel.h
)

set(JOBTRACKER_DIRECTORY_SOURCES
    src/directory/Company.h
    src/directory/CompanyDirectoryController.cpp
    src/directory/CompanyDirectoryController.h
    src/directory/CompanyListModel.cpp
    src/directory/CompanyListModel.h
    src/directory/Contact.h
    src/directory/ContactDirectoryController.cpp
    src/directory/ContactDirectoryController.h
    src/directory/ContactInteractionListModel.cpp
    src/directory/ContactInteractionListModel.h
    src/directory/ContactListModel.cpp
    src/directory/ContactListModel.h
    src/directory/LinkedCompanyContactsModel.cpp
    src/directory/LinkedCompanyContactsModel.h
    src/directory/LinkedCompanyJobsModel.cpp
    src/directory/LinkedCompanyJobsModel.h
)

qt_add_executable(JobTrackerApp
    src/app/App.cpp
    src/app/App.h
    src/app/AppBootstrap.cpp
    src/app/AppBootstrap.h
    ${JOBTRACKER_COMMON_SOURCES}
    ${JOBTRACKER_CV_SOURCES}
    ${JOBTRACKER_DASHBOARD_SOURCES}
    ${JOBTRACKER_DIRECTORY_SOURCES}
    ${JOBTRACKER_JOB_SOURCES}
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
        qml/components/Panel.qml
        qml/components/PrimaryButton.qml
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

if(JOBTRACKER_BUILD_TESTS)
    enable_testing()

    function(jobtracker_deploy_test_runtime target_name)
        if(WIN32)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::Core>"
                    "$<TARGET_FILE:Qt6::Sql>"
                    "$<TARGET_FILE:Qt6::Test>"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Deploying Qt test runtime dependencies for ${target_name}"
            )
        endif()
    endfunction()

    add_executable(JobTrackerCommonTests
        tests/common/FilterAndValidationTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
    )

    target_include_directories(JobTrackerCommonTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerCommonTests
        PRIVATE Qt6::Core Qt6::Test
    )

    add_test(NAME JobTrackerCommonTests COMMAND JobTrackerCommonTests)
    jobtracker_deploy_test_runtime(JobTrackerCommonTests)

    add_executable(JobTrackerJobsTests
        tests/jobs/JobApplicationsControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
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
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerCvTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerCvTests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )

    add_test(NAME JobTrackerCvTests COMMAND JobTrackerCvTests)
    jobtracker_deploy_test_runtime(JobTrackerCvTests)

    add_executable(JobTrackerDashboardTests
        tests/dashboard/DashboardControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_CV_SOURCES}
        ${JOBTRACKER_DASHBOARD_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerDashboardTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerDashboardTests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )

    add_test(NAME JobTrackerDashboardTests COMMAND JobTrackerDashboardTests)
    jobtracker_deploy_test_runtime(JobTrackerDashboardTests)

    add_executable(JobTrackerDirectoryTests
        tests/directory/DirectoryControllerTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_DIRECTORY_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
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

    add_executable(JobTrackerStorageTests
        tests/storage/StorageAndAddJobTest.cpp
        ${JOBTRACKER_COMMON_SOURCES}
        ${JOBTRACKER_JOB_SOURCES}
        ${JOBTRACKER_PERSISTENCE_SOURCES}
    )

    target_include_directories(JobTrackerStorageTests
        PRIVATE src
    )

    target_link_libraries(JobTrackerStorageTests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )

    add_test(NAME JobTrackerStorageTests COMMAND JobTrackerStorageTests)
    jobtracker_deploy_test_runtime(JobTrackerStorageTests)
endif()

include(GNUInstallDirs)

install(TARGETS JobTrackerApp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
