#ifndef JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
#define JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/RelationFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "CvListModel.hpp"

#include <QObject>
#include <QList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <deque>
#include <memory>
#include <optional>

class QAbstractItemModel;
class CvFileAccessService;
class CvImportWorker;
class CvRepository;
class CancellationState;
class JobApplicationListModel;
struct CvImportSaveOutcome;

// Exposes CV Library presentation state and owns the GUI-thread FIFO that
// publishes standalone import outcomes into the shared CV model.
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
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(int pendingImportCount READ pendingImportCount NOTIFY pendingImportCountChanged)

public:
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        CvRepository& repository,
        CvFileAccessService& fileAccessService,
        CvImportWorker& importWorker,
        QObject* parent = nullptr);
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        QVector<CvDocument> documents,
        CvRepository& repository,
        CvFileAccessService& fileAccessService,
        CvImportWorker& importWorker,
        QObject* parent = nullptr);
    ~CvLibraryController() override;

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
    bool importing() const;
    int pendingImportCount() const;

    Q_INVOKABLE void selectCv(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setCategoryFilter(const QString& category);
    Q_INVOKABLE void setLanguageFilter(const QString& language);
    Q_INVOKABLE void setSortMode(const QString& sortMode);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void toggleFavorite(const QString& cvId);
    Q_INVOKABLE void openCv(const QString& cvId);
    Q_INVOKABLE void addCvs(const QList<QUrl>& sourceUrls);
    Q_INVOKABLE void cancelAllCvImports();
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
    void importingChanged();
    void pendingImportCountChanged();
    void cvImportCompleted(
        quint64 operationId,
        const QString& fileName,
        bool success,
        bool wasInserted,
        const QString& message);
    void importQueueDrained();
    void operationFailed(QString message);

private:
    struct QueuedCvImport final
    {
        quint64 operationId_ = 0;
        QUrl sourceUrl_;
    };

    QVariantMap cvToMap(int sourceRow) const;
    const CvDocument* findCv(const QString& cvId) const;
    const CvDocument* selectedSourceCv() const;
    void publishCvDocument(const CvDocument& document, const QString& applicationId = {});
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void handleVisibleCountChanged();
    void updateLinkedApplications();
    void startNextCvImport();
    void publishPendingImportStateChange(int previousCount);
    void handleCvImport(const CvImportSaveOutcome& outcome);
    bool isActiveImportOutcome(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation) const;
    void releaseActiveCvImport();

private:
    CvRepository& repository_;
    CvFileAccessService& fileAccessService_;
    CvImportWorker& importWorker_;
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
    std::deque<QueuedCvImport> importQueue_;
    std::optional<QueuedCvImport> activeImport_;
    std::shared_ptr<CancellationState> activeImportCancellation_;
    quint64 nextImportOperationId_ = 0;
    bool suppressActiveImportNotification_ = false;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
