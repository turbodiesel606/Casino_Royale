#include "App.hpp"

#include "AppBootstrap.hpp"
#include "logging/ApplicationLog.hpp"

#include <QGuiApplication>

#include <cstdlib>

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
		QGuiApplication app{ argc, argv };
		QCoreApplication::setApplicationName(QStringLiteral("JobTracker"));
		ApplicationLogSession logging;
		ApplicationLog::record(LogEventType::ApplicationStartupBegan);
		try {
			AppBootstrap bootstrap{ app };
			return bootstrap.run();
		}
		catch (...) {
			// Codex Comment: Storage, recovery and QML report their own specific failures.
			return EXIT_FAILURE;
		}
	}
	catch (...) {
		return EXIT_FAILURE;
	}
}
