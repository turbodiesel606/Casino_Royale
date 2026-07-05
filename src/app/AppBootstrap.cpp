#include "AppBootstrap.h"

#include <QObject>
#include <QQmlContext>
#include <QUrl>

#include <cstdlib>
#include <stdexcept>
//t
AppBootstrap::AppBootstrap(QCoreApplication& app)
    : app_(app)
    , cvLibraryController_(jobApplicationsController_.jobApplicationListModel())
    , dashboardController_(jobApplicationsController_.jobApplicationListModel(), cvLibraryController_.cvListModel())
    , companyDirectoryController_(jobApplicationsController_.jobApplicationListModel(), contactModel_)
    , contactDirectoryController_(contactModel_)
{
}

int AppBootstrap::run()
{
    connectEngineFailureHandler();
    configureContextProperties();
    loadMainQml();

    return QCoreApplication::exec();
}

void AppBootstrap::connectEngineFailureHandler()
{
    QObject::connect(
        &engine_,
        &QQmlApplicationEngine::objectCreationFailed,
        &app_,
        []() { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
}

void AppBootstrap::configureContextProperties()
{
    engine_.rootContext()->setContextProperty(QStringLiteral("jobApplicationsController"), &jobApplicationsController_);
    engine_.rootContext()->setContextProperty(QStringLiteral("cvLibraryController"), &cvLibraryController_);
    engine_.rootContext()->setContextProperty(QStringLiteral("dashboardController"), &dashboardController_);
    engine_.rootContext()->setContextProperty(QStringLiteral("companyDirectoryController"), &companyDirectoryController_);
    engine_.rootContext()->setContextProperty(QStringLiteral("contactDirectoryController"), &contactDirectoryController_);
}

void AppBootstrap::loadMainQml()
{
    engine_.load(QUrl(QStringLiteral("qrc:/JobTracker/qml/Main.qml")));

    if (engine_.rootObjects().isEmpty()) {
        throw std::runtime_error("Failed to load qrc:/JobTracker/qml/Main.qml");
    }
}
