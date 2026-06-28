cmake_minimum_required(VERSION 3.21)

project(CyberHire
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CYBERHIRE_BUILD_TESTS "Build CyberHire tests" OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(CyberHireApp
    src/main.cpp
)

qt_add_qml_module(CyberHireApp
    URI CyberHire
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/Main.qml
        qml/components/Sidebar.qml
        qml/components/Panel.qml
        qml/components/StatusChip.qml
        qml/pages/DashboardPage.qml
        qml/pages/JobsPage.qml
        qml/pages/JobFormPage.qml
        qml/pages/CvLibraryPage.qml
        qml/pages/CompaniesPage.qml
        qml/pages/ContactsPage.qml
)

target_link_libraries(CyberHireApp
    PRIVATE Qt6::Quick
)

if(WIN32)
    if(TARGET Qt6::qmake)
        get_target_property(_qt6_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt6_bin_dir "${_qt6_qmake_executable}" DIRECTORY)
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt6_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET CyberHireApp POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                "$<TARGET_FILE:CyberHireApp>"
            COMMENT "Deploying Qt runtime dependencies with windeployqt"
        )
    else()
        message(WARNING "windeployqt was not found; Qt runtime dependencies will not be deployed.")
    endif()
endif()

if(CYBERHIRE_BUILD_TESTS)
    enable_testing()
endif()

include(GNUInstallDirs)

install(TARGETS CyberHireApp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
