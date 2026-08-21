#include "CvManagedPathResolver.hpp"

#include "CvDocument.hpp"
#include "storage/StoragePaths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

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

CvManagedPathResolver::CvManagedPathResolver(const StoragePaths& paths)
    : paths_{paths}
{
}

CvManagedPathResolution CvManagedPathResolver::resolve(const CvDocument& document) const
{
    auto relativePath = document.relativePath_;
    relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    if (relativePath.isEmpty()
        || document.storedFileName_.isEmpty()
        || QFileInfo{document.storedFileName_}.fileName() != document.storedFileName_
        || isAbsoluteOnSupportedPlatform(relativePath)
        || containsTraversalSegment(relativePath)) {
        return {{}, QStringLiteral("The CV has an invalid managed file path."), false, false};
    }

    const auto canonicalDataDirectory = QFileInfo{paths_.dataDirectory()}.canonicalFilePath();
    const auto canonicalResumesDirectory = QFileInfo{paths_.resumesDirectory()}.canonicalFilePath();
    if (canonicalDataDirectory.isEmpty() || canonicalResumesDirectory.isEmpty()) {
        return {{}, QStringLiteral("JobTracker's managed data directory is unavailable."), false, false};
    }

    const auto candidatePath = QDir(canonicalDataDirectory).absoluteFilePath(QDir::cleanPath(relativePath));
    if (!isPathWithinDirectory(canonicalResumesDirectory, candidatePath)) {
        return {{}, QStringLiteral("The CV has an invalid managed file path."), false, false};
    }

    const auto resumeRelativePath = QDir(canonicalResumesDirectory).relativeFilePath(candidatePath);
    if (resumeRelativePath.contains(QLatin1Char('/'))
        || resumeRelativePath.contains(QLatin1Char('\\'))
        || resumeRelativePath != document.storedFileName_) {
        return {{}, QStringLiteral("The CV has an invalid managed file path."), false, false};
    }

    const QFileInfo candidateInfo{candidatePath};
    if (!candidateInfo.exists()) {
        return {candidatePath, {}, true, false};
    }
    if (!candidateInfo.isFile() || candidateInfo.isSymLink()) {
        return {{}, QStringLiteral("The CV has an invalid managed file path."), false, false};
    }

    const auto canonicalFilePath = candidateInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty()
        || !isPathWithinDirectory(canonicalResumesDirectory, canonicalFilePath)) {
        return {{}, QStringLiteral("The CV has an invalid managed file path."), false, false};
    }

    return {canonicalFilePath, {}, true, true};
}
