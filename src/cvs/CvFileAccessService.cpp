#include "CvFileAccessService.hpp"

#include "CvDocument.hpp"
#include "logging/ApplicationLog.hpp"
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

CvFileAccessService::CvFileAccessService(const StoragePaths& paths)
    : pathResolver_{paths}
{
}

CvFileAccessResult CvFileAccessService::openDocument(const CvDocument& document) const
{
    const auto resolution = pathResolver_.resolve(document);
    if (!resolution.valid_) {
        if (resolution.failure_ == LogFact::StorageUnavailable) {
            ApplicationLog::record(LogEventType::ManagedCvOpenFailed,
                                   LogFact::CheckManagedStorage, resolution.failure_);
        } else {
            ApplicationLog::record(LogEventType::InvalidManagedCvReferenceRejected,
                                   LogFact::OpenCv, resolution.failure_);
        }
        return {false, resolution.message_};
    }
    const QFileInfo candidateInfo{resolution.absolutePath_};
    if (!resolution.exists_ || !candidateInfo.isReadable()) {
        ApplicationLog::record(LogEventType::ManagedCvOpenFailed,
                               LogFact::CheckManagedFile, resolution.exists_ ? LogFact::FileUnreadable : LogFact::Missing);
        return {false, QStringLiteral("The managed CV file is missing or unreadable.")};
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(resolution.absolutePath_))) {
        ApplicationLog::record(LogEventType::ManagedCvOpenFailed, LogFact::DesktopHandoff, LogFact::DesktopRejected);
        return {false, QStringLiteral("The managed CV file could not be opened.")};
    }

    return {true, {}};
}
