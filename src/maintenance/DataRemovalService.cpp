#include "DataRemovalService.hpp"

#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqlTransaction.hpp"

#include <QDateTime>
#include <QSqlDatabase>

#include <exception>
#include <stdexcept>
#include <utility>

DataRemovalService::DataRemovalService(
	QSqlDatabase& database,
	JobRepository& jobRepository,
	CvRepository& cvRepository,
	CvManagedFileStore& managedFileStore)
	: database_{ database }
	, jobRepository_{ jobRepository }
	, cvRepository_{ cvRepository }
	, managedFileStore_{ managedFileStore }
{
}

DataRemovalItemOutcome DataRemovalService::deleteJob(const DataRemovalItemRequest& item) const
{
	/*
	opening new transaction is reduntant for single query...
	we do only one operation(DELETE).
	try without SqlTransaction while do refactor.

	current behavior is correct!
	*/

	SqlTransaction transaction{ database_, QStringLiteral("delete job application") };
	if (!jobRepository_.remove(item.id_))
		return failureOutcome(item, QStringLiteral("The job application was not found."));

	transaction.commit();
	return { item.id_, item.label_, DataRemovalItemStatus::Deleted, {}, {} };
}

DataRemovalItemOutcome DataRemovalService::removeActiveCv(const DataRemovalItemRequest& item) const
{
	SqlTransaction transaction{ database_, QStringLiteral("remove CV from library") };
	auto document = cvRepository_.findById(item.id_);

	if (!document)
		return failureOutcome(item, QStringLiteral("The CV was not found."));

	if (document->isArchived_)
		return failureOutcome(item, QStringLiteral("The CV is already archived."));

	// If CV is linked, archive it
	if (!document->linkedApplicationIds_.isEmpty()) {

		const auto updatedAt = cvRepository_.updateArchived(item.id_, true);

		if (!updatedAt)
			return failureOutcome(item, QStringLiteral("The CV could not be archived."));

		document->applyArchiveState(true, *updatedAt, *updatedAt);
		transaction.commit();
		return { item.id_, item.label_, DataRemovalItemStatus::Archived, *document, {} };
	}

	return deleteUnlinkedCv(item, *document, transaction);
}

DataRemovalItemOutcome DataRemovalService::restoreArchivedCv(const DataRemovalItemRequest& item) const
{
	// if CV is archived -> restore, otherwise return with message.
	SqlTransaction transaction{ database_, QStringLiteral("restore archived CV") };
	auto document = cvRepository_.findById(item.id_);

	if (!document)
		return failureOutcome(item, QStringLiteral("The CV was not found."));

	// Archive membership does not depend on timestamp parsing success.
	if (!document->isArchived_)
		return failureOutcome(item, QStringLiteral("The CV is already active."));

	const auto updatedAt = cvRepository_.updateArchived(item.id_, false);

	if (!updatedAt) 
		return failureOutcome(item, QStringLiteral("The CV could not be restored."));
	
	document->applyArchiveState(false, QDateTime{}, *updatedAt);
	transaction.commit();
	return { item.id_, item.label_, DataRemovalItemStatus::Restored, *document, {} };
}

DataRemovalItemOutcome DataRemovalService::deleteArchivedCv(const DataRemovalItemRequest& item) const
{
	SqlTransaction transaction{ database_, QStringLiteral("permanently delete archived CV") };
	const auto document = cvRepository_.findById(item.id_);

	if (!document)
		return failureOutcome(item, QStringLiteral("The CV was not found."));
	
	if (!document->isArchived_)
		return failureOutcome(item, QStringLiteral("The CV is not archived."));
	
	if (!document->linkedApplicationIds_.isEmpty()) {
		return {
			item.id_,
			item.label_,
			DataRemovalItemStatus::SkippedLinked,
			*document,
			QStringLiteral("The CV is still used by one or more job applications.") };
	}

	return deleteUnlinkedCv(item, *document, transaction);
}

DataRemovalItemOutcome DataRemovalService::deleteUnlinkedCv(
	const DataRemovalItemRequest& item,
	const CvDocument& document,
	SqlTransaction& transaction) const
{
	const auto removal = managedFileStore_.prepareRemoval(document);
	if (!removal.succeeded()) 
		throw std::runtime_error(removal.message_.toStdString());
	

	if (!cvRepository_.removeUnlinked(item.id_)) {
		return failureOutcome(
			item,
			QStringLiteral("The CV record in SQL could not be deleted."));
	}

	transaction.commit();

	const bool cleaned = managedFileStore_.finalizeRemoval(*removal.preparation_);
	QString message;

	if (removal.fileWasMissing_)
		message = QStringLiteral("The CV record was deleted; its managed file was already missing.");
	
	else if (!cleaned) 
		message = QStringLiteral(
			"The CV record was deleted, but final file cleanup will be retried on restart.");
	
	return { item.id_, item.label_, DataRemovalItemStatus::Deleted, document, std::move(message) };
}
