#pragma once

#include "common/RoleFilterProxyModel.h"
#include "CvListModel.h"
#include "LinkedApplicationListModel.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QAbstractItemModel;

class CvLibraryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* cvModel READ cvModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* linkedApplicationsModel READ linkedApplicationsModel NOTIFY linkedApplicationsModelChanged)
    Q_PROPERTY(QVariantList categorySummary READ categorySummary NOTIFY filtersChanged)
    Q_PROPERTY(int cvCount READ cvCount NOTIFY cvModelChanged)
    Q_PROPERTY(int selectedCvIndex READ selectedCvIndex NOTIFY selectedCvChanged)
    Q_PROPERTY(QString selectedCvId READ selectedCvId NOTIFY selectedCvChanged)
    Q_PROPERTY(QVariantMap selectedCv READ selectedCv NOTIFY selectedCvChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString languageFilter READ languageFilter WRITE setLanguageFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)

public:
    explicit CvLibraryController(const JobApplicationListModel& applicationsModel, QObject* parent = nullptr);
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        QVector<CvDocument> documents,
        QObject* parent = nullptr);

    QAbstractItemModel* cvModel();
    CvListModel& cvListModel();
    const CvListModel& cvListModel() const;
    QAbstractItemModel* linkedApplicationsModel();
    QVariantList categorySummary() const;
    int cvCount() const;
    int selectedCvIndex() const;
    QString selectedCvId() const;
    QVariantMap selectedCv() const;
    QString searchText() const;
    QString categoryFilter() const;
    QString languageFilter() const;
    QString sortMode() const;
    QString resultSummary() const;

    Q_INVOKABLE void selectCv(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setCategoryFilter(const QString& category);
    Q_INVOKABLE void setLanguageFilter(const QString& language);
    Q_INVOKABLE void setSortMode(const QString& sortMode);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void toggleFavorite(const QString& cvId);
    Q_INVOKABLE void openCv(const QString& cvId);
    void recordCvUse(const CvDocument& document, const QString& applicationId, bool wasInserted);

signals:
    void cvModelChanged();
    void linkedApplicationsModelChanged();
    void selectedCvChanged();
    void filtersChanged();
    void resultSummaryChanged();
    void openCvRequested(QString filePath);
    void operationFailed(QString message);

private:
    QVariantMap cvToMap(const CvDocument& cv) const;
    const CvDocument* selectedSourceCv() const;
    int selectedSourceRow() const;
    void refreshSelectionAfterFilterChange();
    void updateLinkedApplications();

    CvListModel cvModel_;
    RoleFilterProxyModel filteredCvModel_;
    LinkedApplicationListModel linkedApplicationsModel_;
    QString searchText_;
    QString categoryFilter_;
    QString languageFilter_;
    QString sortMode_ = QStringLiteral("Last Modified");
    int selectedCvIndex_ = 0;
};
