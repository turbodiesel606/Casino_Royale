#ifndef JOBTRACKER_SRC_STORAGE_STORAGEPATHS_HPP
#define JOBTRACKER_SRC_STORAGE_STORAGEPATHS_HPP

#include "logging/LogEvent.hpp"
#include <QString>

class StoragePaths final
{
public:
    explicit StoragePaths(QString dataDirectory = {}, LogFact operation = LogFact::Startup,
                          qint64 affectedCount = 0);

    const QString& dataDirectory() const;
    const QString& resumesDirectory() const;
    const QString& databasePath() const;

    void ensureDirectories() const;

private:
    QString dataDirectory_;
    QString resumesDirectory_;
    QString databasePath_;
};

#endif // JOBTRACKER_SRC_STORAGE_STORAGEPATHS_HPP
