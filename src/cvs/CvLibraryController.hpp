#ifndef JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
#define JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/RelationFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "CvListModel.hpp"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QAbstractItemModel;
class CvFileAccessService;
class CvRepository;
class JobApplicationListModel;

class CvLibraryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* cvModel READ cvModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* linkedApplicationsModel READ linkedApplicationsModel CONSTANT)
    Q_PROPERTY(QVariantList categorySummary READ categorySummary NOTIFY categorySummaryChanged)
    Q_PROPERTY(int cvCount READ cvCount NOTIFY cvCountChanged)
    Q_PROPERTY(int selectedCvIndex READ selectedCvIndex NOTIFY selectedCvIndexChanged)
    Q_PROPERTY(QString selectedCvId READ selectedCvId NOTIFY selectedCvIdChanged)
    Q_PROPERTY(QVariantMap selectedCv READ selectedCv NOTIFY selectedCvChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString languageFilter READ languageFilter WRITE setLanguageFilter NOTIFY languageFilterChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)

public:
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        CvRepository& repository,
        CvFileAccessService& fileAccessService,
        QObject* parent = nullptr);
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        QVector<CvDocument> documents,
        CvRepository& repository,
        CvFileAccessService& fileAccessService,
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
    void categorySummaryChanged();
    void cvCountChanged();
    void selectedCvIndexChanged();
    void selectedCvIdChanged();
    void selectedCvChanged();
    void searchTextChanged();
    void categoryFilterChanged();
    void languageFilterChanged();
    void sortModeChanged();
    void resultSummaryChanged();
    void operationFailed(QString message);

private:
    QVariantMap cvToMap(int sourceRow) const;
    const CvDocument* findCv(const QString& cvId) const;
    const CvDocument* selectedSourceCv() const;
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void handleVisibleCountChanged();
    void updateLinkedApplications();

private:
    CvRepository& repository_;
    CvFileAccessService& fileAccessService_;
    CvListModel cvModel_;
    RoleFilterProxyModel filteredCvModel_;
    RelationFilterProxyModel linkedApplicationsModel_;
    StableIdSelectionTracker selectionTracker_;
    QString searchText_;
    QString categoryFilter_;
    QString languageFilter_;
    QString sortMode_ = QStringLiteral("Last Modified");
    int publishedCvCount_ = 0;
    bool visibleCountNotificationsSuppressed_ = false;
};

#endif // JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
