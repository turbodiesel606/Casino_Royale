#include "CvLibraryController.hpp"

#include "CvFileAccessService.hpp"
#include "CvImportWorker.hpp"
#include "CvRepository.hpp"
#include "common/CancellationState.hpp"
#include "common/ModelRoleUtils.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"

#include <QMap>

#include <exception>
#include <optional>

CvLibraryController::CvLibraryController(
	const JobApplicationListModel& applicationsModel,
	CvRepository& repository,
	CvFileAccessService& fileAccessService,
	CvImportWorker& importWorker,
	QObject* parent)
	: CvLibraryController(
		applicationsModel,
		QVector<CvDocument>{},
		repository,
		fileAccessService,
		importWorker,
		parent)
{
}

CvLibraryController::CvLibraryController(
	const JobApplicationListModel& applicationsModel,
	QVector<CvDocument> documents,
	CvRepository& repository,
	CvFileAccessService& fileAccessService,
	CvImportWorker& importWorker,
	QObject* parent)
	: QObject{ parent }
	, repository_{ repository }
	, fileAccessService_{ fileAccessService }
	, importWorker_{ importWorker }
	, cvModel_{ std::move(documents), this }
	, filteredCvModel_{ this }
	, linkedApplicationsModel_(
		applicationsModel,
		JobApplicationListModel::CvIdRole,
		this)
	, selectionTracker_(filteredCvModel_, CvListModel::IdRole)
	, bulkSelectionTracker_{filteredCvModel_, CvListModel::IdRole}
{
	filteredCvModel_.setSearchRoles({
		CvListModel::FileNameRole,
		CvListModel::TitleRole,
		CvListModel::CategoryRole,
		CvListModel::LanguageRole,
		CvListModel::DescriptionRole,
		});
	filteredCvModel_.setSort(CvListModel::UpdatedAtRole, Qt::DescendingOrder);
	filteredCvModel_.setExactFilter(CvListModel::IsArchivedRole, QStringLiteral("false"));
	connect(
		&selectionTracker_,
		&StableIdSelectionTracker::selectionChanged,
		this,
		&CvLibraryController::handleSelectionChanged);
	connect(
		&selectionTracker_,
		&StableIdSelectionTracker::visibleRowCountChanged,
		this,
		[this]() {
			emit cvCountChanged();
			emit resultSummaryChanged();
		});
	connect(
		&bulkSelectionTracker_,
		&BulkIdSelectionTracker::selectionChanged,
		this,
		[this]() {
			emit checkedCvsChanged();
			emit mutationAvailabilityChanged();
		});
	filteredCvModel_.setSourceModel(&cvModel_);
	selectionTracker_.synchronize();
	connect(
		&cvModel_,
		&QAbstractItemModel::rowsInserted,
		this,
		[this]() { emit categorySummaryChanged(); });
	connect(
		&cvModel_,
		&QAbstractItemModel::rowsRemoved,
		this,
		[this]() { emit categorySummaryChanged(); });
	connect(
		&cvModel_,
		&QAbstractItemModel::modelReset,
		this,
		[this]() { emit categorySummaryChanged(); });
	connect(
		&cvModel_,
		&QAbstractItemModel::dataChanged,
		this,
		[this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
			if (roles.isEmpty()
				|| roles.contains(CvListModel::CategoryRole)
				|| roles.contains(CvListModel::IsArchivedRole)) {
				emit categorySummaryChanged();
			}
		});
	connect(
		&importWorker_,
		&CvImportWorker::importCompleted,
		this,
		&CvLibraryController::handleCvImport);
}

CvLibraryController::CvLibraryController(
	const JobApplicationListModel& applicationsModel,
	QVector<CvDocument> documents,
	CvRepository& repository,
	CvFileAccessService& fileAccessService,
	CvImportWorker& importWorker,
	DataRemovalWorker& removalWorker,
	StorageMutationGate& mutationGate,
	QObject* parent)
	: CvLibraryController(
		applicationsModel,
		std::move(documents),
		repository,
		fileAccessService,
		importWorker,
		parent)
{
	removalWorker_ = &removalWorker;
	mutationGate_ = &mutationGate;
	connect(
		removalWorker_,
		&DataRemovalWorker::removalCompleted,
		this,
		&CvLibraryController::handleRemovalCompleted);
	connect(
		mutationGate_,
		&StorageMutationGate::stateChanged,
		this,
		&CvLibraryController::mutationAvailabilityChanged);
}

CvLibraryController::~CvLibraryController()
{
	shuttingDown_ = true;
	if (mutationGate_ != nullptr) {
		mutationGate_->releaseCvImports(importQueue_.pendingCount());
	}
	importQueue_.clearWaiting();
	importQueue_.requestActiveCancellation();
	if (activeMutationCancellation_ != nullptr) {
		activeMutationCancellation_->requestCancellation();
		if (mutationGate_ != nullptr) {
			mutationGate_->endRemoval();
		}
	}
}

void CvLibraryController::recordCvUse(
	const CvDocument& document,
	const QString& applicationId,
	CvImportDisposition disposition)
{
	publishCvDocument(document, disposition, applicationId);
}

void CvLibraryController::recordCvReplacement(
	const QString& previousCvId,
	const CvDocument& document,
	const QString& applicationId,
	CvImportDisposition disposition)
{
	if (applicationId.isEmpty() || document.id_.isEmpty()) {
		return;
	}
	publishCvDocument(document, disposition, applicationId, previousCvId);
}

void CvLibraryController::recordApplicationsDeleted(const QStringList& applicationIds)
{
	cvModel_.removeLinkedApplications(applicationIds);
	if (checkedCvCount() > 0) {
		emit checkedCvsChanged();
		emit mutationAvailabilityChanged();
	}
}

QAbstractItemModel* CvLibraryController::cvModel()
{
	return &filteredCvModel_;
}

CvListModel& CvLibraryController::cvListModel()
{
	return cvModel_;
}

const CvListModel& CvLibraryController::cvListModel() const
{
	return cvModel_;
}

QAbstractItemModel* CvLibraryController::linkedApplicationsModel()
{
	return &linkedApplicationsModel_;
}

QVariantList CvLibraryController::categorySummary() const
{
	const auto matchesCurrentView = [this](const CvDocument& document) {
		return document.archivedAt_.isValid()
			== (libraryView_ == LibraryView::Archived);
	};
	int viewCount = 0;
	for (int row = 0; row < cvModel_.rowCount(); ++row) {
		const auto* cv = cvModel_.cvAt(row);
		if (cv != nullptr && matchesCurrentView(*cv)) {
			++viewCount;
		}
	}
	QVariantList rows;
	rows.append(QVariantMap{
		{QStringLiteral("title"), QStringLiteral("All CVs")},
		{QStringLiteral("count"), viewCount},
		{QStringLiteral("selected"), categoryFilter_.isEmpty() || categoryFilter_ == QStringLiteral("All")},
		});

	QMap<QString, int> categoryCounts;
	for (int row = 0; row < cvModel_.rowCount(); ++row) {
		const auto* cv = cvModel_.cvAt(row);
		if (cv != nullptr && matchesCurrentView(*cv)) {
			++categoryCounts[cv->category_];
		}
	}

	for (auto it = categoryCounts.cbegin(); it != categoryCounts.cend(); ++it) {
		rows.append(QVariantMap{
			{QStringLiteral("title"), it.key()},
			{QStringLiteral("count"), it.value()},
			{QStringLiteral("selected"), categoryFilter_ == it.key()},
			});
	}

	return rows;
}

int CvLibraryController::cvCount() const
{
	return selectionTracker_.visibleRowCount();
}

int CvLibraryController::selectedCvIndex() const
{
	return selectionTracker_.selectedRow();
}

QString CvLibraryController::selectedCvId() const
{
	return selectionTracker_.selectedId();
}

QVariantMap CvLibraryController::selectedCv() const
{
	const auto sourceIndex = selectionTracker_.selectedSourceIndex();
	return sourceIndex.isValid() ? cvToMap(sourceIndex.row()) : QVariantMap();
}

QString CvLibraryController::searchText() const
{
	return filteredCvModel_.searchText();
}

QString CvLibraryController::categoryFilter() const
{
	return categoryFilter_;
}

QString CvLibraryController::languageFilter() const
{
	return languageFilter_;
}

QString CvLibraryController::sortMode() const
{
	return sortMode_;
}

QString CvLibraryController::resultSummary() const
{
	const auto count = cvCount();
	return count == 1 ? QStringLiteral("1 CV") : QStringLiteral("%1 CVs").arg(count);
}

bool CvLibraryController::importing() const
{
	return pendingImportCount() > 0;
}

int CvLibraryController::pendingImportCount() const
{
	return importQueue_.pendingCount();
}

CvLibraryController::LibraryView CvLibraryController::libraryView() const
{
	return libraryView_;
}

QStringList CvLibraryController::checkedCvIds() const
{
	return bulkSelectionTracker_.selectedIds();
}

int CvLibraryController::checkedCvCount() const
{
	return bulkSelectionTracker_.selectedCount();
}

int CvLibraryController::checkedLinkedCvCount() const
{
	int count = 0;
	for (const auto& id : checkedCvIds()) {
		const auto* document = cvModel_.cvById(id);
		if (document != nullptr && !document->linkedApplicationIds_.isEmpty()) {
			++count;
		}
	}
	return count;
}

int CvLibraryController::checkedUnlinkedCvCount() const
{
	return checkedCvCount() - checkedLinkedCvCount();
}

bool CvLibraryController::allVisibleCvsChecked() const
{
	return bulkSelectionTracker_.allVisibleSelected();
}

bool CvLibraryController::someVisibleCvsChecked() const
{
	return bulkSelectionTracker_.someVisibleSelected();
}

bool CvLibraryController::mutatingCvs() const
{
	return activeMutationCancellation_ != nullptr;
}

int CvLibraryController::pendingDeletionCount() const
{
	return mutatingCvs() ? 1 : 0;
}

bool CvLibraryController::canMutateCheckedCvs() const
{
	return removalWorker_ != nullptr
		&& checkedCvCount() > 0
		&& !mutatingCvs()
		&& mutationGate_ != nullptr
		&& mutationGate_->canBeginRemoval();
}

void CvLibraryController::selectCv(int index)
{
	selectionTracker_.selectRow(index);
}

void CvLibraryController::setSearchText(const QString& text)
{
	const auto normalized = text.trimmed();
	if (searchText() == normalized) {
		return;
	}

	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	filteredCvModel_.setSearchText(normalized);
	selectionTracker_.endModelUpdate();
	emit searchTextChanged();
}

void CvLibraryController::setCategoryFilter(const QString& category)
{
	const auto normalized = category.trimmed();
	if (categoryFilter_ == normalized) {
		return;
	}

	const auto previousSummarySelection = categoryFilter_ == QStringLiteral("All")
		? QString()
		: categoryFilter_;
	const auto nextSummarySelection = normalized == QStringLiteral("All")
		? QString()
		: normalized;
	categoryFilter_ = normalized;
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	filteredCvModel_.setExactFilter(CvListModel::CategoryRole, categoryFilter_ == QStringLiteral("All") ? QString() : categoryFilter_);
	selectionTracker_.endModelUpdate();
	emit categoryFilterChanged();
	if (previousSummarySelection != nextSummarySelection) {
		emit categorySummaryChanged();
	}
}

void CvLibraryController::setLanguageFilter(const QString& language)
{
	const auto normalized = language.trimmed();
	if (languageFilter_ == normalized) {
		return;
	}

	languageFilter_ = normalized;
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	filteredCvModel_.setExactFilter(CvListModel::LanguageRole, languageFilter_ == QStringLiteral("All") ? QString() : languageFilter_);
	selectionTracker_.endModelUpdate();
	emit languageFilterChanged();
}

void CvLibraryController::setSortMode(const QString& sortMode)
{
	const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Last Modified") : sortMode.trimmed();
	if (sortMode_ == normalized) {
		return;
	}

	sortMode_ = normalized;
	selectionTracker_.beginModelUpdate();
	if (sortMode_ == QStringLiteral("File Name")) {
		filteredCvModel_.setSort(CvListModel::FileNameRole, Qt::AscendingOrder);
	}
	else if (sortMode_ == QStringLiteral("Linked Jobs")) {
		filteredCvModel_.setSort(CvListModel::LinkedApplicationCountRole, Qt::DescendingOrder);
	}
	else {
		filteredCvModel_.setSort(CvListModel::UpdatedAtRole, Qt::DescendingOrder);
	}
	selectionTracker_.endModelUpdate();
	emit sortModeChanged();
}

void CvLibraryController::clearFilters()
{
	if (searchText().isEmpty() && categoryFilter_.isEmpty() && languageFilter_.isEmpty()) {
		return;
	}

	const bool didSearchTextChange = !searchText().isEmpty();
	const bool didCategoryFilterChange = !categoryFilter_.isEmpty();
	const bool didCategorySummaryChange = didCategoryFilterChange
		&& categoryFilter_ != QStringLiteral("All");
	const bool didLanguageFilterChange = !languageFilter_.isEmpty();
	categoryFilter_.clear();
	languageFilter_.clear();
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	filteredCvModel_.setSearchText(QString());
	filteredCvModel_.setExactFilter(CvListModel::CategoryRole, QString());
	filteredCvModel_.setExactFilter(CvListModel::LanguageRole, QString());
	filteredCvModel_.setExactFilter(
		CvListModel::IsArchivedRole,
		libraryView_ == LibraryView::Archived
			? QStringLiteral("true")
			: QStringLiteral("false"));
	selectionTracker_.endModelUpdate();
	if (didSearchTextChange) {
		emit searchTextChanged();
	}
	if (didCategoryFilterChange) {
		emit categoryFilterChanged();
	}
	if (didCategorySummaryChange) {
		emit categorySummaryChanged();
	}
	if (didLanguageFilterChange) {
		emit languageFilterChanged();
	}
}

void CvLibraryController::toggleFavorite(const QString& cvId)
{
	if (mutationGate_ != nullptr && mutationGate_->removalActive()) {
		emit operationFailed(QStringLiteral("Wait for the active deletion to finish."));
		return;
	}
	const auto* cv = cvModel_.cvById(cvId);
	if (cv == nullptr) {
		emit operationFailed(QStringLiteral("CV was not found."));
		return;
	}

	const bool isFavorite = !cv->isFavorite_;
	std::optional<QDateTime> updatedAt;
	try {
		updatedAt = repository_.updateFavorite(cvId, isFavorite);
		if (!updatedAt) {
			emit operationFailed(QStringLiteral("CV was not found."));
			return;
		}
	}
	catch (const std::exception& error) {
		emit operationFailed(QStringLiteral("The favorite state could not be saved: %1")
			.arg(QString::fromUtf8(error.what())));
		return;
	}
	catch (...) {
		emit operationFailed(QStringLiteral("The favorite state could not be saved."));
		return;
	}

	if (!cvModel_.setFavorite(cvId, isFavorite, *updatedAt)) {
		emit operationFailed(QStringLiteral("The favorite state was saved but the CV model could not be updated."));
		return;
	}
}

void CvLibraryController::openCv(const QString& cvId)
{
	const auto* cv = cvModel_.cvById(cvId);
	if (cv == nullptr) {
		emit operationFailed(QStringLiteral("CV was not found."));
		return;
	}

	const auto result = fileAccessService_.openDocument(*cv);
	if (!result.opened_) {
		emit operationFailed(result.message_);
	}
}

void CvLibraryController::addCvs(const QList<QUrl>& sourceUrls)
{
	if (shuttingDown_ || sourceUrls.isEmpty())
		return;
	int validUrlCount = 0;
	for (const auto& sourceUrl : sourceUrls) {
		if (!sourceUrl.isEmpty()) {
			++validUrlCount;
		}
	}
	if (validUrlCount == 0) {
		return;
	}
	if (mutationGate_ != nullptr && !mutationGate_->reserveCvImports(validUrlCount)) {
		emit operationFailed(
			QStringLiteral("Wait for the active deletion to finish before adding CVs."));
		return;
	}

	const auto previousCount = pendingImportCount();
	for (const auto& sourceUrl : sourceUrls) {
		if (!sourceUrl.isEmpty()) {
			importQueue_.enqueue({sourceUrl});
		}
	}

	publishPendingImportStateChange(previousCount);
	startNextCvImport();
}

void CvLibraryController::cancelAllCvImports()
{
	const auto previousCount = pendingImportCount();
	const auto clearedCount = importQueue_.clearWaiting();
	if (mutationGate_ != nullptr) {
		mutationGate_->releaseCvImports(clearedCount);
	}
	importQueue_.requestActiveCancellation();
	publishPendingImportStateChange(previousCount);
}

void CvLibraryController::setLibraryView(LibraryView view)
{
	if (libraryView_ == view) {
		return;
	}
	libraryView_ = view;
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	filteredCvModel_.setExactFilter(
		CvListModel::IsArchivedRole,
		libraryView_ == LibraryView::Archived
			? QStringLiteral("true")
			: QStringLiteral("false"));
	selectionTracker_.endModelUpdate();
	emit libraryViewChanged();
	emit categorySummaryChanged();
}

void CvLibraryController::toggleCvChecked(int index)
{
	bulkSelectionTracker_.toggleRow(index);
}

void CvLibraryController::setAllVisibleCvsChecked(bool checked)
{
	bulkSelectionTracker_.setAllVisibleSelected(checked);
}

void CvLibraryController::removeCheckedCvs()
{
	if (libraryView_ != LibraryView::Active) {
		emit operationFailed(QStringLiteral("Only active CVs can be removed from the library."));
		return;
	}
	submitCvMutation(DataRemovalKind::RemoveActiveCvs);
}

void CvLibraryController::restoreCheckedCvs()
{
	if (libraryView_ != LibraryView::Archived) {
		emit operationFailed(QStringLiteral("Only archived CVs can be restored."));
		return;
	}
	submitCvMutation(DataRemovalKind::RestoreArchivedCvs);
}

void CvLibraryController::permanentlyDeleteCheckedCvs()
{
	if (libraryView_ != LibraryView::Archived) {
		emit operationFailed(QStringLiteral("Only archived CVs can be permanently deleted."));
		return;
	}
	if (checkedUnlinkedCvCount() == 0) {
		emit operationFailed(
			QStringLiteral("Every selected CV is still used by a job application."));
		return;
	}
	submitCvMutation(DataRemovalKind::DeleteArchivedCvs);
}

void CvLibraryController::cancelCvMutation()
{
	if (activeMutationCancellation_ != nullptr) {
		activeMutationCancellation_->requestCancellation();
	}
}

void CvLibraryController::submitCvMutation(DataRemovalKind kind)
{
	if (!canMutateCheckedCvs() || !mutationGate_->beginRemoval()) {
		emit cvMutationCompleted(
			0,
			0,
			0,
			0,
			checkedCvCount(),
			false,
			QStringLiteral("Wait for pending job-save or Add CV work before changing CVs."));
		return;
	}

	QVector<DataRemovalItemRequest> items;
	for (const auto& id : checkedCvIds()) {
		const auto* document = cvModel_.cvById(id);
		items.append({id, document != nullptr ? document->originalFileName_ : id});
	}

	activeMutationOperationId_ = ++nextMutationOperationId_;
	activeMutationKind_ = kind;
	activeMutationCancellation_ = std::make_shared<CancellationState>();
	emit mutatingCvsChanged();
	emit pendingDeletionCountChanged();
	emit mutationAvailabilityChanged();
	removalWorker_->submit({
		activeMutationOperationId_,
		kind,
		std::move(items),
		activeMutationCancellation_});
}

QVariantMap CvLibraryController::cvToMap(int sourceRow) const
{
	return common::model::rowToVariantMap(
		cvModel_,
		sourceRow,
		{
			CvListModel::IdRole,
			CvListModel::FileNameRole,
			CvListModel::TitleRole,
			CvListModel::CategoryRole,
			CvListModel::CategoryAccentRole,
			CvListModel::LanguageRole,
			CvListModel::LanguageAccentRole,
			CvListModel::LastModifiedLabelRole,
			CvListModel::FileSizeLabelRole,
			CvListModel::DescriptionRole,
			CvListModel::LinkedApplicationCountRole,
			CvListModel::LinkedApplicationCountLabelRole,
			CvListModel::IsFavoriteRole,
			CvListModel::IsArchivedRole,
			CvListModel::ArchivedAtRole,
		});
}

void CvLibraryController::publishCvDocument(
	const CvDocument& document,
	CvImportDisposition disposition,
	const QString& applicationId,
	const QString& previousCvId)
{
	if (document.id_.isEmpty()) {
		return;
	}

	if (disposition == CvImportDisposition::RestoredArchived) {
		cvModel_.setArchiveState(document.id_, {}, document.updatedAt_);
	}

	// Publish restored archive roles before changing either side of a job link.
	if (!previousCvId.isEmpty() && previousCvId != document.id_) {
		cvModel_.removeLinkedApplication(previousCvId, applicationId);
	}

	if (cvModel_.cvById(document.id_) == nullptr) {
		auto newDocument = document;
		if (!applicationId.isEmpty()
			&& !newDocument.linkedApplicationIds_.contains(applicationId)) {
			newDocument.linkedApplicationIds_.append(applicationId);
		}
		cvModel_.appendDocument(std::move(newDocument));
		return;
	}

	if (!applicationId.isEmpty()) {
		if (cvModel_.addLinkedApplication(document.id_, applicationId)
			&& bulkSelectionTracker_.contains(document.id_)) {
			emit checkedCvsChanged();
			emit mutationAvailabilityChanged();
		}
	}
}

void CvLibraryController::handleSelectionChanged(
	bool idChanged,
	bool rowChanged,
	bool dataChanged)
{
	if (idChanged) {
		updateLinkedApplications();
		emit selectedCvIdChanged();
	}
	if (rowChanged) {
		emit selectedCvIndexChanged();
	}
	if (idChanged || dataChanged) {
		emit selectedCvChanged();
	}
}

void CvLibraryController::updateLinkedApplications()
{
	linkedApplicationsModel_.setSelectedId(selectedCvId());
}

void CvLibraryController::startNextCvImport()
{
	if (shuttingDown_ || !importQueue_.activateNext()) {
		return;
	}

	const auto* const active = importQueue_.active();

	importWorker_.submit(
		CvImportRequest{
		active->operationId_,
		active->payload_.sourceUrl_,
		importQueue_.activeCancellation()
		});
}

void CvLibraryController::publishPendingImportStateChange(int previousCount)
{
	const auto currentCount = pendingImportCount();
	if (previousCount == currentCount) {
		return;
	}

	emit pendingImportCountChanged();
	if ((previousCount == 0) != (currentCount == 0)) {
		emit importingChanged();
	}
	if (previousCount > 0 && currentCount == 0 && !shuttingDown_) {
		emit importQueueDrained();
	}
}

void CvLibraryController::handleCvImport(const CvImportSaveOutcome& outcome)
{
	if (shuttingDown_
		|| !importQueue_.matches(outcome.operationId_, outcome.cancellation_)) {
		return;
	}

	if (outcome.success_) {
		publishCvDocument(outcome.document_, outcome.disposition_);
	}

	if (!importQueue_.completionSuppressed()) {
		const auto message = outcome.message_.isEmpty()
			? (outcome.success_
				? cvImportSuccessMessage(outcome.disposition_)
				: QStringLiteral("The CV could not be added."))
			: outcome.message_;
			emit cvImportCompleted(
			outcome.operationId_,
			outcome.fileName_,
			outcome.success_,
			cvImportDispositionName(outcome.disposition_),
			message);
	}

	releaseActiveCvImport();
}

void CvLibraryController::handleRemovalCompleted(
	const DataRemovalBatchOutcome& outcome)
{
	if (shuttingDown_
		|| !activeMutationKind_.has_value()
		|| outcome.kind_ != *activeMutationKind_
		|| outcome.operationId_ != activeMutationOperationId_
		|| outcome.cancellation_ != activeMutationCancellation_) {
		return;
	}

	QStringList deletedIds;
	QStringList completedIds;
	QStringList details;
	int archivedCount = 0;
	int restoredCount = 0;
	int skippedCount = 0;
	int failedCount = 0;
	for (const auto& item : outcome.result_.items_) {
		switch (item.status_) {
		case DataRemovalItemStatus::Deleted:
			deletedIds.append(item.id_);
			completedIds.append(item.id_);
			if (!item.message_.isEmpty()) {
				details.append(QStringLiteral("%1: %2").arg(item.label_, item.message_));
			}
			break;
		case DataRemovalItemStatus::Archived:
			++archivedCount;
			completedIds.append(item.id_);
			cvModel_.setArchiveState(
				item.id_,
				item.document_.archivedAt_,
				item.document_.updatedAt_);
			break;
		case DataRemovalItemStatus::Restored:
			++restoredCount;
			completedIds.append(item.id_);
			cvModel_.setArchiveState(item.id_, {}, item.document_.updatedAt_);
			break;
		case DataRemovalItemStatus::SkippedLinked:
			++skippedCount;
			details.append(QStringLiteral("%1: %2").arg(item.label_, item.message_));
			break;
		case DataRemovalItemStatus::Failed:
			++failedCount;
			details.append(QStringLiteral("%1: %2").arg(item.label_, item.message_));
			break;
		}
	}

	cvModel_.removeDocuments(deletedIds);
	bulkSelectionTracker_.removeIds(completedIds);
	QString message = QStringLiteral("Deleted %1, archived %2, restored %3, skipped %4, failed %5 CV(s).")
		.arg(deletedIds.size())
		.arg(archivedCount)
		.arg(restoredCount)
		.arg(skippedCount)
		.arg(failedCount);
	if (!details.isEmpty()) {
		message.append(QStringLiteral(" %1").arg(details.join(QStringLiteral("; "))));
	}
	if (outcome.result_.cancelled_) {
		message.append(QStringLiteral(" Remaining items were canceled."));
	}

	releaseCvMutation();
	emit cvMutationCompleted(
		deletedIds.size(),
		archivedCount,
		restoredCount,
		skippedCount,
		failedCount,
		outcome.result_.cancelled_,
		message);
}

void CvLibraryController::releaseCvMutation()
{
	activeMutationCancellation_.reset();
	activeMutationOperationId_ = 0;
	activeMutationKind_.reset();
	if (mutationGate_ != nullptr) {
		mutationGate_->endRemoval();
	}
	emit mutatingCvsChanged();
	emit pendingDeletionCountChanged();
	emit mutationAvailabilityChanged();
}

void CvLibraryController::releaseActiveCvImport()
{
	const auto previousCount = pendingImportCount();
	importQueue_.finishActive();
	if (mutationGate_ != nullptr) {
		mutationGate_->releaseCvImports(1);
	}
	publishPendingImportStateChange(previousCount);
	startNextCvImport();
}
