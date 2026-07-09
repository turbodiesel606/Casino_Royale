#pragma once

#include <QString>

class StoragePaths final
{
public:
    explicit StoragePaths(QString dataDirectory = {});

    const QString& dataDirectory() const;
    const QString& resumesDirectory() const;
    const QString& databasePath() const;

    void ensureDirectories() const;
    void removeTemporaryFiles() const;

private:
    QString dataDirectory_;
    QString resumesDirectory_;
    QString databasePath_;
};
