#include "app/AppBootstrap.h"

#include <QDebug>
#include <QGuiApplication>

#include <cstdlib>
#include <exception>
#include <iostream>
int main(int argc, char *argv[])
{
    try {
        QGuiApplication app(argc, argv);
        AppBootstrap bootstrap(app);

        return bootstrap.run();
    } catch (const std::exception& exception) {
        qCritical() << "Application startup failed:" << exception.what();
    } catch (...) {
        qCritical() << "Application startup failed with an unknown exception.";
    }

    return EXIT_FAILURE;
}
