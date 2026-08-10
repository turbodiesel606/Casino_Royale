#include "CvImportService.hpp"

#include "CvRepository.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QUuid>

#include <stdexcept>
#include <utility>

namespace {

CvDocument buildCvDocument(const CvManagedFilePreparation& preparation)
{
    CvDocument document;
    document.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    document.originalFileName_ = preparation.originalFileName_;
    document.storedFileName_ = preparation.storedFileName_;
    document.relativePath_ = preparation.relativePath_;
    document.sha256_ = preparation.sha256_;
    document.sizeBytes_ = preparation.sizeBytes_;
    document.title_ = QFileInfo{preparation.originalFileName_}.completeBaseName();
    document.category_ = QStringLiteral("General");
    document.createdAt_ = QDateTime::fromString(
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
        Qt::ISODate).toUTC();
    document.updatedAt_ = document.createdAt_;
    return document;
}

}

CvImportService::CvImportService(
    const CvManagedFileStore& managedFileStore,
    CvRepository& repository)
    : managedFileStore_(managedFileStore)
    , repository_(repository)
{
}

CvManagedFilePreparationResult CvImportService::prepareDocument(
    const QUrl& sourceUrl,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    return managedFileStore_.prepare(sourceUrl, cancellation);
}

CvImportResult CvImportService::importPreparedDocument(
    const std::shared_ptr<CvManagedFilePreparation>& preparation) const
{
    if (preparation == nullptr) {
        throw std::runtime_error("The prepared CV file is unavailable.");
    }

    if (const auto existing = repository_.findByIdentity(
            preparation->sha256_,
            preparation->originalFileName_)) {
        return {*existing, {}, false};
    }

    auto document = buildCvDocument(*preparation);
    const auto completedFilePath = managedFileStore_.finalize(*preparation);
    try {
        repository_.insert(document);
    } catch (...) {
        managedFileStore_.removeCompletedFile(completedFilePath);
        throw;
    }
    return {std::move(document), completedFilePath, true};
}

bool CvImportService::removeCompletedFile(const QString& completedFilePath) const
{
    return managedFileStore_.removeCompletedFile(completedFilePath);
}
