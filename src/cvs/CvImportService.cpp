#include "CvImportService.hpp"

#include "CvRepository.hpp"
#include "storage/StoragePaths.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

#include <stdexcept>

CvImportService::CvImportService(const StoragePaths& paths, CvRepository& repository)
    : paths_(paths)
    , repository_(repository)
{
}

CvImportResult CvImportService::importDocument(const QUrl& sourceUrl) const
{
    if (!sourceUrl.isLocalFile()) {
        throw std::runtime_error("Select a local CV file.");
    }

    const QFileInfo sourceInfo(sourceUrl.toLocalFile());
    const auto extension = sourceInfo.suffix().toLower();
    if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable()) {
        throw std::runtime_error("The selected CV file is not readable.");
    }
    if (extension != QStringLiteral("pdf")
        && extension != QStringLiteral("doc")
        && extension != QStringLiteral("docx")) {
        throw std::runtime_error("The CV must be a PDF, DOC, or DOCX file.");
    }

    QFile source(sourceInfo.absoluteFilePath());
    if (!source.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("The selected CV file could not be opened.");
    }
    const auto hash = QString::fromLatin1(QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256).toHex());
    source.close();

    if (const auto existing = repository_.findBySha256(hash)) {
        return {*existing, {}, false};
    }

    auto baseName = sourceInfo.completeBaseName();
    baseName.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("_"));
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("resume");
    }

    const auto storedName = QStringLiteral("%1_%2_%3.%4")
                                .arg(
                                    QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")),
                                    QUuid::createUuid().toString(QUuid::WithoutBraces),
                                    baseName,
                                    extension);
    const auto finalPath = QDir(paths_.resumesDirectory()).filePath(storedName);
    const auto temporaryPath = finalPath + QStringLiteral(".part");
    if (!QFile::copy(sourceInfo.absoluteFilePath(), temporaryPath) || !QFile::rename(temporaryPath, finalPath)) {
        QFile::remove(temporaryPath);
        QFile::remove(finalPath);
        throw std::runtime_error("The CV could not be copied into JobTracker storage.");
    }

    CvDocument document;
    document.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    document.fileName_ = sourceInfo.fileName();
    document.originalFileName_ = sourceInfo.fileName();
    document.storedFileName_ = storedName;
    document.relativePath_ = QDir::fromNativeSeparators(QStringLiteral("Resumes/%1").arg(storedName));
    document.sha256_ = hash;
    document.sizeBytes_ = sourceInfo.size();
    document.title_ = sourceInfo.completeBaseName();
    document.category_ = QStringLiteral("General");
    document.categoryAccent_ = QStringLiteral("#1687ff");
    document.languageAccent_ = QStringLiteral("#65bf4c");
    document.fileSizeLabel_ = QStringLiteral("%1 KB").arg((document.sizeBytes_ + 1023) / 1024);
    document.createdAt_ = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    document.updatedAt_ = document.createdAt_;
    document.lastModifiedLabel_ = QDate::currentDate().toString(QStringLiteral("MMM d, yyyy"));

    try {
        repository_.insert(document);
    } catch (...) {
        QFile::remove(finalPath);
        throw;
    }
    return {document, finalPath, true};
}
