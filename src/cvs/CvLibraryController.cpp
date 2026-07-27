#include "CvLibraryController.hpp"

#include "CvFileAccessService.hpp"
#include "CvRepository.hpp"

#include <QMap>

#include <exception>

namespace {

	QString linkedApplicationCountLabel(int count)
	{
		return count == 1 ? QStringLiteral("1 job") : QStringLiteral("%1 jobs").arg(count);
	}

}

CvLibraryController::CvLibraryController(
    const JobApplicationListModel& applicationsModel,
    CvRepository& repository,
    CvFileAccessService& fileAccessService,
    QObject* parent)
    : CvLibraryController(
        applicationsModel,
        QVector<CvDocument>{},
        repository,
        fileAccessService,
        parent)
{
}

CvLibraryController::CvLibraryController(
    const JobApplicationListModel& applicationsModel,
    QVector<CvDocument> documents,
    CvRepository& repository,
    CvFileAccessService& fileAccessService,
    QObject* parent)
	: QObject(parent)
	, repository_(repository)
	, fileAccessService_(fileAccessService)
	, cvModel_(std::move(documents), this)
	, filteredCvModel_(this)
	, linkedApplicationsModel_(applicationsModel, this)
	, selectionTracker_(filteredCvModel_, CvListModel::IdRole)
{
	filteredCvModel_.setSearchRoles({
		CvListModel::FileNameRole,
		CvListModel::TitleRole,
		CvListModel::CategoryRole,
		CvListModel::LanguageRole,
		CvListModel::DescriptionRole,
		});
	filteredCvModel_.setSort(CvListModel::LastModifiedLabelRole, Qt::DescendingOrder);
	connect(
		&selectionTracker_,
		&StableIdSelectionTracker::selectionChanged,
		this,
		&CvLibraryController::handleSelectionChanged);
	filteredCvModel_.setSourceModel(&cvModel_);
	selectionTracker_.synchronize();
	publishedCvCount_ = cvCount();
	connect(
		&filteredCvModel_,
		&QAbstractItemModel::rowsInserted,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&filteredCvModel_,
		&QAbstractItemModel::rowsRemoved,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&filteredCvModel_,
		&QAbstractItemModel::modelReset,
		this,
		[this]() { handleVisibleCountChanged(); });
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
			if (roles.isEmpty() || roles.contains(CvListModel::CategoryRole)) {
				emit categorySummaryChanged();
			}
		});
}

void CvLibraryController::recordCvUse(
    const CvDocument& document,
    const QString& applicationId,
    bool wasInserted)
{
    if (wasInserted) {
        auto newDocument = document;
        newDocument.linkedApplicationIds_.append(applicationId);
        cvModel_.appendDocument(std::move(newDocument));
    } else {
        cvModel_.addLinkedApplication(document.id_, applicationId);
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
	QVariantList rows;
	rows.append(QVariantMap{
		{QStringLiteral("title"), QStringLiteral("All CVs")},
		{QStringLiteral("count"), cvModel_.rowCount()},
		{QStringLiteral("selected"), categoryFilter_.isEmpty() || categoryFilter_ == QStringLiteral("All")},
		});

	QMap<QString, int> categoryCounts;
	for (int row = 0; row < cvModel_.rowCount(); ++row) {
		const auto* cv = cvModel_.cvAt(row);
		if (cv != nullptr) {
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
	return filteredCvModel_.rowCount();
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
	const auto* cv = selectedSourceCv();
	return cv != nullptr ? cvToMap(*cv) : QVariantMap();
}

QString CvLibraryController::searchText() const
{
	return searchText_;
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

void CvLibraryController::selectCv(int index)
{
	selectionTracker_.selectRow(index);
}

void CvLibraryController::setSearchText(const QString& text)
{
	const auto normalized = text.trimmed();
	if (searchText_ == normalized) {
		return;
	}

	searchText_ = normalized;
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredCvModel_.setSearchText(searchText_);
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
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
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredCvModel_.setExactFilter(CvListModel::CategoryRole, categoryFilter_ == QStringLiteral("All") ? QString() : categoryFilter_);
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
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
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredCvModel_.setExactFilter(CvListModel::LanguageRole, languageFilter_ == QStringLiteral("All") ? QString() : languageFilter_);
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
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
		filteredCvModel_.setSort(CvListModel::LastModifiedLabelRole, Qt::DescendingOrder);
	}
	selectionTracker_.endModelUpdate();
	emit sortModeChanged();
}

void CvLibraryController::clearFilters()
{
	if (searchText_.isEmpty() && categoryFilter_.isEmpty() && languageFilter_.isEmpty()) {
		return;
	}

	const bool didSearchTextChange = !searchText_.isEmpty();
	const bool didCategoryFilterChange = !categoryFilter_.isEmpty();
	const bool didCategorySummaryChange = didCategoryFilterChange
		&& categoryFilter_ != QStringLiteral("All");
	const bool didLanguageFilterChange = !languageFilter_.isEmpty();
	searchText_.clear();
	categoryFilter_.clear();
	languageFilter_.clear();
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredCvModel_.setSearchText(QString());
	filteredCvModel_.clearExactFilter();
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
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
	const auto* cv = findCv(cvId);
	if (cv == nullptr) {
		emit operationFailed(QStringLiteral("CV was not found."));
		return;
	}

	const bool isFavorite = !cv->isFavorite_;
	try {
		if (!repository_.updateFavorite(cvId, isFavorite)) {
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

	if (!cvModel_.setFavorite(cvId, isFavorite)) {
		emit operationFailed(QStringLiteral("The favorite state was saved but the CV model could not be updated."));
		return;
	}
}

void CvLibraryController::openCv(const QString& cvId)
{
	const auto* cv = findCv(cvId);
	if (cv == nullptr) {
		emit operationFailed(QStringLiteral("CV was not found."));
		return;
	}

	const auto result = fileAccessService_.openDocument(*cv);
	if (!result.opened_) {
		emit operationFailed(result.message_);
	}
}

QVariantMap CvLibraryController::cvToMap(const CvDocument& cv) const
{
	const auto linkedCount = cv.linkedApplicationIds_.size();
	return {
		{QStringLiteral("id"), cv.id_},
		{QStringLiteral("fileName"), cv.fileName_},
		{QStringLiteral("title"), cv.title_},
		{QStringLiteral("category"), cv.category_},
		{QStringLiteral("categoryAccent"), cv.categoryAccent_},
		{QStringLiteral("language"), cv.language_},
		{QStringLiteral("languageAccent"), cv.languageAccent_},
		{QStringLiteral("lastModifiedLabel"), cv.lastModifiedLabel_},
		{QStringLiteral("fileSizeLabel"), cv.fileSizeLabel_},
		{QStringLiteral("description"), cv.description_},
		{QStringLiteral("linkedApplicationCount"), linkedCount},
		{QStringLiteral("linkedApplicationCountLabel"), linkedApplicationCountLabel(linkedCount)},
		{QStringLiteral("isFavorite"), cv.isFavorite_},
	};
}

const CvDocument* CvLibraryController::findCv(const QString& cvId) const
{
	for (int row = 0; row < cvModel_.rowCount(); ++row) {
		const auto* cv = cvModel_.cvAt(row);
		if (cv != nullptr && cv->id_ == cvId) {
			return cv;
		}
	}

	return nullptr;
}

const CvDocument* CvLibraryController::selectedSourceCv() const
{
	const auto sourceIndex = selectionTracker_.selectedSourceIndex();
	return sourceIndex.isValid() ? cvModel_.cvAt(sourceIndex.row()) : nullptr;
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

void CvLibraryController::handleVisibleCountChanged()
{
	if (visibleCountNotificationsSuppressed_) {
		return;
	}

	const auto count = cvCount();
	if (publishedCvCount_ == count) {
		return;
	}

	publishedCvCount_ = count;
	emit cvCountChanged();
	emit resultSummaryChanged();
}

void CvLibraryController::updateLinkedApplications()
{
	linkedApplicationsModel_.setCvId(selectedCvId());
}
