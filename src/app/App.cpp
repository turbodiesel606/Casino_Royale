#include "App.hpp"

#include "AppBootstrap.hpp"

#include <QDebug>
#include <QGuiApplication>

#include <cstdlib>
#include <exception>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
namespace {
	void initializeConsole()
	{
#ifdef Q_OS_WIN
		AllocConsole();
		(void)freopen("CONOUT$", "w", stdout);
		(void)freopen("CONOUT$", "w", stderr);
#endif
	}

} // namespace

int App::start(int argc, char* argv[])
{
	try {
		initializeConsole();
		QGuiApplication app(argc, argv); // Initialize the Qt GUI application and event loop.
		QCoreApplication::setApplicationName(QStringLiteral("JobTracker")); // Set the app name used by Qt framework features.
		AppBootstrap bootstrap(app);

		return bootstrap.run();
	}
	catch (const std::exception& exception) {
		qCritical() << "Application startup failed:" << exception.what();
	}
	catch (...) {
		qCritical() << "Application startup failed with an unknown exception.";
		// only startup can be failed??? clarify regarding 'stack unwinding' deeper in the code
	}

	return EXIT_FAILURE;
}
