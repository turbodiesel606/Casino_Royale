#include "App.hpp"

#include "AppBootstrap.hpp"

#include <QDebug>
#include <QGuiApplication>

#include <cstdlib>
#include <exception>

int App::start(int argc, char* argv[])
{
    try {
        QGuiApplication app(argc, argv); // Initialize the Qt GUI application and event loop.
        QCoreApplication::setApplicationName(QStringLiteral("JobTracker")); // Set the app name used by Qt framework features.
        AppBootstrap bootstrap(app);

        return bootstrap.run();
    } catch (const std::exception& exception) {
        qCritical() << "Application startup failed:" << exception.what();
    } catch (...) {
        qCritical() << "Application startup failed with an unknown exception."; 
        // only startup can be failed??? clarify regarding 'stack unwinding' deeper in the code
    }

    return EXIT_FAILURE;
}
