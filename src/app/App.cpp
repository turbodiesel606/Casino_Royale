#include "App.hpp"

#include "AppBootstrap.hpp"

#include <QDebug>
#include <QGuiApplication>

#include <cstdlib>
#include <exception>

#if defined(Q_OS_WIN) && defined(QT_DEBUG)
#include <cstdio>
#include <Windows.h>
#endif

namespace {

void initializeDevelopmentConsole()
{
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    AllocConsole();
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
#endif
}

} // namespace

int App::start(int argc, char* argv[])
{
    try {
        initializeDevelopmentConsole();
        QGuiApplication app{argc, argv};
        QCoreApplication::setApplicationName(QStringLiteral("JobTracker"));
        AppBootstrap bootstrap{app};

        return bootstrap.run();
    } catch (const std::exception& exception) {
        qCritical() << "Application startup failed:" << exception.what();
    } catch (...) {
        qCritical() << "Application startup failed with an unknown exception.";
    }

    return EXIT_FAILURE;
}
