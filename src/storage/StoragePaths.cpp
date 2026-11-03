#include "StoragePaths.hpp"
#include "logging/ApplicationLog.hpp"

#include <QDir>
#include <QStandardPaths>

#include <stdexcept>

namespace {
void reportUnavailableStorage(LogFact operation, qint64 count, LogFact area, LogFact stage) noexcept
{
    if (operation == LogFact::Startup) {
        ApplicationLog::record({LogEventType::ApplicationStorageSetupFailed,
                                {area, stage, LogFact::StorageUnavailable}});
    } else {
        ApplicationLog::record({LogEventType::BackgroundWorkStartFailed,
                                {operation, stage, LogFact::StorageUnavailable, count}});
    }
}
}

StoragePaths::StoragePaths(QString dataDirectory, LogFact operation, qint64 affectedCount)
{
    if (dataDirectory.isEmpty()) {
        const auto applicationData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (applicationData.isEmpty()) {
            reportUnavailableStorage(operation, affectedCount, LogFact::AppDataArea, LogFact::StoragePathResolution);
            throw std::runtime_error("Could not resolve the application data directory.");
        }
        
        dataDirectory = QDir(applicationData).filePath(QStringLiteral("Data"));
    }

    dataDirectory_ = QDir::cleanPath(dataDirectory);
    resumesDirectory_ = QDir(dataDirectory_).filePath(QStringLiteral("Resumes"));
    databasePath_ = QDir(dataDirectory_).filePath(QStringLiteral("database.sqlite"));
    try {
        ensureDirectories();
    } catch (...) {
        reportUnavailableStorage(operation, affectedCount, LogFact::ManagedCvArea, LogFact::StorageDirectoryCreation);
        throw;
    }
}

const QString& StoragePaths::dataDirectory() const
{
    return dataDirectory_;
}

const QString& StoragePaths::resumesDirectory() const
{
    return resumesDirectory_;
}

const QString& StoragePaths::databasePath() const
{
    return databasePath_;
}

void StoragePaths::ensureDirectories() const
{
    if (!QDir().mkpath(resumesDirectory_)) {
        throw std::runtime_error("Could not create the JobTracker data directories.");
    }
}
