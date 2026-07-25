#include "CvFileAccessService.hpp"

#include "CvDocument.hpp"
#include "storage/StoragePaths.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

namespace {

bool containsTraversalSegment(const QString& path)
{
    return path.split(QLatin1Char('/'), Qt::SkipEmptyParts).contains(QStringLiteral(".."));
}

bool isAbsoluteOnSupportedPlatform(const QString& path)
{
    static const QRegularExpression windowsDrivePath{QStringLiteral("^[A-Za-z]:/")};
    return QDir::isAbsolutePath(path)
        || path.startsWith(QLatin1Char('/'))
        || windowsDrivePath.match(path).hasMatch();
}

bool isPathWithinDirectory(const QString& directoryPath, const QString& filePath)
{
    const auto relativePath = QDir(directoryPath).relativeFilePath(filePath);
    return !QDir::isAbsolutePath(relativePath)
        && relativePath != QStringLiteral("..")
        && !relativePath.startsWith(QStringLiteral("../"));
}

}

CvFileAccessService::CvFileAccessService(const StoragePaths& paths)
    : paths_{paths}
{
}

CvFileAccessResult CvFileAccessService::openDocument(const CvDocument& document) const
{
    auto relativePath = document.relativePath_;
    relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    if (relativePath.isEmpty()
        || isAbsoluteOnSupportedPlatform(relativePath)
        || containsTraversalSegment(relativePath)) {
        return {false, QStringLiteral("The CV has an invalid managed file path.")};
    }

    const QFileInfo dataDirectoryInfo{paths_.dataDirectory()};
    const auto canonicalDataDirectory = dataDirectoryInfo.canonicalFilePath();
    if (canonicalDataDirectory.isEmpty()) {
        return {false, QStringLiteral("JobTracker's managed data directory is unavailable.")};
    }

    const auto candidatePath = QDir(canonicalDataDirectory).absoluteFilePath(QDir::cleanPath(relativePath));
    if (!isPathWithinDirectory(canonicalDataDirectory, candidatePath)) {
        return {false, QStringLiteral("The CV has an invalid managed file path.")};
    }

    const QFileInfo candidateInfo{candidatePath};
    if (!candidateInfo.exists() || !candidateInfo.isFile() || candidateInfo.isSymLink() || !candidateInfo.isReadable()) {
        return {false, QStringLiteral("The managed CV file is missing or unreadable.")};
    }

    const auto canonicalFilePath = candidateInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty() || !isPathWithinDirectory(canonicalDataDirectory, canonicalFilePath)) {
        return {false, QStringLiteral("The CV has an invalid managed file path.")};
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(canonicalFilePath))) {
        return {false, QStringLiteral("The managed CV file could not be opened.")};
    }

    return {true, {}};
}
