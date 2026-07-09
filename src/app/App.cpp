#include "App.h"

#include "AppBootstrap.h"

#include <QDebug>
#include <QGuiApplication>

#include <cstdlib>
#include <exception>

int App::start(int argc, char* argv[])
{
    try {
        QGuiApplication app(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("JobTracker"));
        AppBootstrap bootstrap(app);

        return bootstrap.run();
    } catch (const std::exception& exception) {
        qCritical() << "Application startup failed:" << exception.what();
    } catch (...) {
        qCritical() << "Application startup failed with an unknown exception.";
    }

    return EXIT_FAILURE;
}
