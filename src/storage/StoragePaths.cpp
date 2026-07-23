#include "StoragePaths.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <stdexcept>
#include <vector>
StoragePaths::StoragePaths(QString dataDirectory)
{
    if (dataDirectory.isEmpty()) {
        const auto applicationData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (applicationData.isEmpty()) 
            throw std::runtime_error("Could not resolve the application data directory.");
        
        dataDirectory = QDir(applicationData).filePath(QStringLiteral("Data"));
    }

    dataDirectory_ = QDir::cleanPath(dataDirectory);
    resumesDirectory_ = QDir(dataDirectory_).filePath(QStringLiteral("Resumes"));
    databasePath_ = QDir(dataDirectory_).filePath(QStringLiteral("database.sqlite"));
    ensureDirectories();
    removeTemporaryFiles();
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

void StoragePaths::removeTemporaryFiles() const
{
    const QDir directory(resumesDirectory_);
    const auto temporaryFiles = directory.entryInfoList(
        {QStringLiteral("*.part")},
        QDir::Files | QDir::NoDotAndDotDot);

    for (const auto& file : temporaryFiles) 
        QFile::remove(file.absoluteFilePath());
    
}
