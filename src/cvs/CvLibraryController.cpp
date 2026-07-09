#include "CvLibraryController.h"

#include <QMap>

#include <algorithm>

namespace {

	QString linkedApplicationCountLabel(int count)
	{
		return count == 1 ? QStringLiteral("1 job") : QStringLiteral("%1 jobs").arg(count);
	}

}

CvLibraryController::CvLibraryController(const JobApplicationListModel& applicationsModel, QObject* parent)
    : CvLibraryController(applicationsModel, QVector<CvDocument>{}, parent)
{
}

CvLibraryController::CvLibraryController(
    const JobApplicationListModel& applicationsModel,
    QVector<CvDocument> documents,
    QObject* parent)
	: QObject(parent)
	, cvModel_(std::move(documents), this)
	, filteredCvModel_(this)
	, linkedApplicationsModel_(applicationsModel, this)
{
	filteredCvModel_.setSourceModel(&cvModel_);
	filteredCvModel_.setSearchRoles({
		CvListModel::FileNameRole,
		CvListModel::TitleRole,
		CvListModel::CategoryRole,
		CvListModel::LanguageRole,
		CvListModel::DescriptionRole,
		});
	filteredCvModel_.setSort(CvListModel::LastModifiedLabelRole, Qt::DescendingOrder);
	updateLinkedApplications();
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
    refreshSelectionAfterFilterChange();
    emit cvModelChanged();
    emit resultSummaryChanged();
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
	return selectedCvIndex_;
}

QString CvLibraryController::selectedCvId() const
{
	const auto* cv = selectedSourceCv();
	return cv != nullptr ? cv->id_ : QString();
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
	if (index == selectedCvIndex_ || index < 0 || index >= filteredCvModel_.rowCount()) {
		return;
	}

	selectedCvIndex_ = index;
	updateLinkedApplications();
	emit selectedCvChanged();
	emit linkedApplicationsModelChanged();
}

void CvLibraryController::setSearchText(const QString& text)
{
	const auto normalized = text.trimmed();
	if (searchText_ == normalized) {
		return;
	}

	searchText_ = normalized;
	filteredCvModel_.setSearchText(searchText_);
	refreshSelectionAfterFilterChange();
	emit filtersChanged();
	emit cvModelChanged();
	emit resultSummaryChanged();
}

void CvLibraryController::setCategoryFilter(const QString& category)
{
	const auto normalized = category.trimmed();
	if (categoryFilter_ == normalized) {
		return;
	}

	categoryFilter_ = normalized;
	filteredCvModel_.setExactFilter(CvListModel::CategoryRole, categoryFilter_ == QStringLiteral("All") ? QString() : categoryFilter_);
	refreshSelectionAfterFilterChange();
	emit filtersChanged();
	emit cvModelChanged();
	emit resultSummaryChanged();
}

void CvLibraryController::setLanguageFilter(const QString& language)
{
	const auto normalized = language.trimmed();
	if (languageFilter_ == normalized) {
		return;
	}

	languageFilter_ = normalized;
	filteredCvModel_.setExactFilter(CvListModel::LanguageRole, languageFilter_ == QStringLiteral("All") ? QString() : languageFilter_);
	refreshSelectionAfterFilterChange();
	emit filtersChanged();
	emit cvModelChanged();
	emit resultSummaryChanged();
}

void CvLibraryController::setSortMode(const QString& sortMode)
{
	const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Last Modified") : sortMode.trimmed();
	if (sortMode_ == normalized) {
		return;
	}

	sortMode_ = normalized;
	if (sortMode_ == QStringLiteral("File Name")) {
		filteredCvModel_.setSort(CvListModel::FileNameRole, Qt::AscendingOrder);
	}
	else if (sortMode_ == QStringLiteral("Linked Jobs")) {
		filteredCvModel_.setSort(CvListModel::LinkedApplicationCountRole, Qt::DescendingOrder);
	}
	else {
		filteredCvModel_.setSort(CvListModel::LastModifiedLabelRole, Qt::DescendingOrder);
	}
	refreshSelectionAfterFilterChange();
	emit filtersChanged();
	emit cvModelChanged();
}

void CvLibraryController::clearFilters()
{
	if (searchText_.isEmpty() && categoryFilter_.isEmpty() && languageFilter_.isEmpty()) {
		return;
	}

	searchText_.clear();
	categoryFilter_.clear();
	languageFilter_.clear();
	filteredCvModel_.setSearchText(QString());
	filteredCvModel_.clearExactFilter();
	refreshSelectionAfterFilterChange();
	emit filtersChanged();
	emit cvModelChanged();
	emit resultSummaryChanged();
}

void CvLibraryController::toggleFavorite(const QString& cvId)
{
	if (!cvModel_.toggleFavorite(cvId)) {
		emit operationFailed(QStringLiteral("CV was not found."));
		return;
	}

	if (selectedCvId() == cvId) {
		emit selectedCvChanged();
	}
}

void CvLibraryController::openCv(const QString& cvId)
{
	for (int row = 0; row < cvModel_.rowCount(); ++row) {
		const auto* cv = cvModel_.cvAt(row);
		if (cv != nullptr && cv->id_ == cvId) {
			emit openCvRequested(cv->fileName_);
			return;
		}
	}

	emit operationFailed(QStringLiteral("CV was not found."));
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

const CvDocument* CvLibraryController::selectedSourceCv() const
{
	const auto sourceRow = selectedSourceRow();
	return sourceRow >= 0 ? cvModel_.cvAt(sourceRow) : nullptr;
}

int CvLibraryController::selectedSourceRow() const
{
	const auto proxyIndex = filteredCvModel_.index(selectedCvIndex_, 0);
	if (!proxyIndex.isValid()) {
		return -1;
	}

	return filteredCvModel_.mapToSource(proxyIndex).row();
}

void CvLibraryController::refreshSelectionAfterFilterChange()
{
	const auto previousIndex = selectedCvIndex_;
	const auto rowCount = filteredCvModel_.rowCount();
	selectedCvIndex_ = rowCount > 0 ? std::clamp(selectedCvIndex_, 0, rowCount - 1) : -1;
	updateLinkedApplications();
	if (selectedCvIndex_ != previousIndex || selectedCvIndex_ >= 0) {
		emit selectedCvChanged();
		emit linkedApplicationsModelChanged();
	}
}

void CvLibraryController::updateLinkedApplications()
{
	linkedApplicationsModel_.setCvId(selectedCvId());
}
