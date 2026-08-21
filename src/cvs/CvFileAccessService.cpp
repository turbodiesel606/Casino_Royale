#include "CvFileAccessService.hpp"

#include "CvDocument.hpp"
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
        return {false, resolution.message_};
    }
    const QFileInfo candidateInfo{resolution.absolutePath_};
    if (!resolution.exists_ || !candidateInfo.isReadable()) {
        return {false, QStringLiteral("The managed CV file is missing or unreadable.")};
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(resolution.absolutePath_))) {
        return {false, QStringLiteral("The managed CV file could not be opened.")};
    }

    return {true, {}};
}
