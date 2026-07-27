#ifndef JOBTRACKER_TESTS_SUPPORT_STORAGETESTFIXTURES_HPP
#define JOBTRACKER_TESTS_SUPPORT_STORAGETESTFIXTURES_HPP

#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace testsupport {

class TemporaryStorageFixture final
{
public:
    TemporaryStorageFixture()
        : paths_{QDir{temporaryDirectory_.path()}.filePath(QStringLiteral("Data"))}
    {
    }

    bool isValid() const
    {
        return temporaryDirectory_.isValid();
    }

    QString rootPath() const
    {
        return temporaryDirectory_.path();
    }

    StoragePaths& paths()
    {
        return paths_;
    }

    const StoragePaths& paths() const
    {
        return paths_;
    }

    QString createFile(
        const QString& name = QStringLiteral("resume.pdf"),
        const QByteArray& contents = QByteArrayLiteral("%PDF-1.4 JobTracker test CV"),
        const QString& directory = {}) const
    {
        const auto targetDirectory = directory.isEmpty() ? rootPath() : directory;
        const auto path = QDir{targetDirectory}.filePath(name);
        QFile file{path};
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }
        if (file.write(contents) != contents.size()) {
            return {};
        }
        return path;
    }

private:
    QTemporaryDir temporaryDirectory_;
    StoragePaths paths_;
};

class TemporaryDatabaseFixture final
{
public:
    TemporaryDatabaseFixture()
        : database_{storage_.paths().databasePath()}
    {
    }

    bool isValid() const
    {
        return storage_.isValid();
    }

    TemporaryStorageFixture& storage()
    {
        return storage_;
    }

    const TemporaryStorageFixture& storage() const
    {
        return storage_;
    }

    SqliteDatabase& database()
    {
        return database_;
    }

private:
    TemporaryStorageFixture storage_;
    SqliteDatabase database_;
};

} // namespace testsupport

#endif // JOBTRACKER_TESTS_SUPPORT_STORAGETESTFIXTURES_HPP
