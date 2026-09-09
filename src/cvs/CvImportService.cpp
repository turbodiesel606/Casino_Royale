#include "CvImportService.hpp"

#include "CvRepository.hpp"
#include "common/TimeUtils.hpp"

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
		document.title_ = QFileInfo{ preparation.originalFileName_ }.completeBaseName();
		document.category_ = QStringLiteral("General");
		document.createdAt_ = common::currentUtcSecond();
		document.updatedAt_ = document.createdAt_;
		return document;
	}

} // namespace

QString cvImportDispositionName(CvImportDisposition disposition)
{
	switch (disposition) {
	case CvImportDisposition::Inserted:
		return QStringLiteral("inserted");
	case CvImportDisposition::RestoredArchived:
		return QStringLiteral("restored-archived");
	case CvImportDisposition::ExistingActive:
	default:
		return QStringLiteral("existing-active");
	}
}

QString cvImportSuccessMessage(CvImportDisposition disposition)
{
	switch (disposition) {
	case CvImportDisposition::Inserted:
		return QStringLiteral("CV added successfully.");
	case CvImportDisposition::RestoredArchived:
		return QStringLiteral("The archived CV was restored to the library.");
	case CvImportDisposition::ExistingActive:
	default:
		return QStringLiteral("A CV with the same filename and SHA-256 already exists.");
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
	const std::shared_ptr<CvManagedFilePreparation>& preparation,
	// cancellation is stub, delete later
	const std::shared_ptr<CancellationState>& cancellation) const
{
	/*	Cancellation checks were turned off intentionally in this method!	*/
	
	// check for prepared CV file aviability
	if (preparation == nullptr) {
		return { {}, {}, CvImportDisposition::ExistingActive, false, false,
			QStringLiteral("The prepared CV file is unavailable.") };
	}
#if 0 
	if (cancellation != nullptr && cancellation->isCancellationRequested()) {
		return { {}, {}, CvImportDisposition::ExistingActive, false, true,
			QStringLiteral("CV import was canceled.") };
	}
#endif // 0

	/* check for exact SHA - 256 + 
	exact original filename, including filename case.*/

	auto existing = repository_.findByIdentity(
		preparation->sha256_,
		preparation->originalFileName_);
	
	if (existing) {
		// Every exact archived duplicate is now restored.
		if (existing->archivedAt_.isValid()) {
			const auto updatedAt = repository_.updateArchived(existing->id_, false);

			if (!updatedAt)
				throw std::runtime_error("The archived CV could not be restored.");

			// update archive and updated state after success, then return.
			existing->applyArchiveState(QDateTime{}, *updatedAt);
			return { *existing, {}, CvImportDisposition::RestoredArchived, true };
		}

		return { *existing, {}, CvImportDisposition::ExistingActive, true };
	}
	
	/* if really miss in DB, create new file */
	
	const auto staged = managedFileStore_.stageAndFinalize(*preparation, cancellation);
	if (!staged.succeeded()) {
		return { {}, {}, CvImportDisposition::ExistingActive, false,
			staged.cancelled_, staged.message_ };
	}
	auto document = buildCvDocument(*preparation);
	repository_.insert(document);
	return { std::move(document), preparation->finalFilePath_, CvImportDisposition::Inserted, true };
}

bool CvImportService::removeCompletedFile(const QString& completedFilePath) const
{
	return managedFileStore_.removeCompletedFile(completedFilePath);
}
