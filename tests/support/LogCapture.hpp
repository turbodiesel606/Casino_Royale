#ifndef JOBTRACKER_TESTS_SUPPORT_LOGCAPTURE_HPP
#define JOBTRACKER_TESTS_SUPPORT_LOGCAPTURE_HPP

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

namespace testsupport {

class LogCapture final
{
public:
    LogCapture()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(name_);
        const auto state = QStandardPaths::writableLocation(QStandardPaths::StateLocation);
        if (state.isEmpty())
            return;
        QDir ancestor{state};
        while (!ancestor.isRoot() && ancestor.dirName() != name_)
            ancestor.setPath(QFileInfo{ancestor.absolutePath()}.absolutePath());
        if (ancestor.dirName() == name_ && !ancestor.exists() && QDir{}.mkpath(state)) {
            ownedRoot_ = ancestor.absolutePath();
            directory_ = QDir{state}.filePath(QStringLiteral("Logs"));
        }
    }

    ~LogCapture()
    {
        QCoreApplication::setApplicationName(oldName_);
        QStandardPaths::setTestModeEnabled(oldTestMode_);
        if (!ownedRoot_.isEmpty())
            QDir{ownedRoot_}.removeRecursively();
    }

    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;
    bool isValid() const { return !directory_.isEmpty(); }
    const QString& directory() const { return directory_; }
    QByteArray contents() const
    {
        QByteArray result;
        QDir logs{directory_};
        for (const auto& segment : logs.entryList({QStringLiteral("jobtracker-*.log")}, QDir::Files, QDir::Name)) {
            QFile file{logs.filePath(segment)};
            if (file.open(QIODevice::ReadOnly))
                result += file.readAll();
        }
        return result;
    }

private:
    const bool oldTestMode_ = QStandardPaths::isTestModeEnabled();
    const QString oldName_ = QCoreApplication::applicationName();
    const QString name_ = QStringLiteral("JobTracker-log-test-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString ownedRoot_;
    QString directory_;
};

} // namespace testsupport

#endif // JOBTRACKER_TESTS_SUPPORT_LOGCAPTURE_HPP
