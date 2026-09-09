#ifndef JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
#define JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/RelationFilterProxyModel.hpp"
#include "common/BulkIdSelectionTracker.hpp"
#include "common/SerialOperationQueue.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "CvImportService.hpp"
#include "CvListModel.hpp"

#include <QObject>
#include <QList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <memory>
#include <optional>

class QAbstractItemModel;
class CvFileAccessService;
class CvImportWorker;
class CvRepository;
class DataRemovalWorker;
class StorageMutationGate;
class CancellationState;
class JobApplicationListModel;
struct CvImportSaveOutcome;
struct DataRemovalBatchOutcome;
enum class DataRemovalKind;

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
    Q_PROPERTY(LibraryView libraryView READ libraryView WRITE setLibraryView NOTIFY libraryViewChanged)
    Q_PROPERTY(QStringList checkedCvIds READ checkedCvIds NOTIFY checkedCvsChanged)
    Q_PROPERTY(int checkedCvCount READ checkedCvCount NOTIFY checkedCvsChanged)
    Q_PROPERTY(int checkedLinkedCvCount READ checkedLinkedCvCount NOTIFY checkedCvsChanged)
    Q_PROPERTY(int checkedUnlinkedCvCount READ checkedUnlinkedCvCount NOTIFY checkedCvsChanged)
    Q_PROPERTY(bool allVisibleCvsChecked READ allVisibleCvsChecked NOTIFY checkedCvsChanged)
    Q_PROPERTY(bool someVisibleCvsChecked READ someVisibleCvsChecked NOTIFY checkedCvsChanged)
    Q_PROPERTY(bool mutatingCvs READ mutatingCvs NOTIFY mutatingCvsChanged)
    Q_PROPERTY(int pendingDeletionCount READ pendingDeletionCount NOTIFY pendingDeletionCountChanged)
    Q_PROPERTY(bool canMutateCheckedCvs READ canMutateCheckedCvs NOTIFY mutationAvailabilityChanged)

public:
    enum class LibraryView
    {
        Active,
        Archived
    };
    Q_ENUM(LibraryView)

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
    CvLibraryController(
        const JobApplicationListModel& applicationsModel,
        QVector<CvDocument> documents,
        CvRepository& repository,
        CvFileAccessService& fileAccessService,
        CvImportWorker& importWorker,
        DataRemovalWorker& removalWorker,
        StorageMutationGate& mutationGate,
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
    LibraryView libraryView() const;
    QStringList checkedCvIds() const;
    int checkedCvCount() const;
    int checkedLinkedCvCount() const;
    int checkedUnlinkedCvCount() const;
    bool allVisibleCvsChecked() const;
    bool someVisibleCvsChecked() const;
    bool mutatingCvs() const;
    int pendingDeletionCount() const;
    bool canMutateCheckedCvs() const;

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
    Q_INVOKABLE void setLibraryView(LibraryView view);
    Q_INVOKABLE void toggleCvChecked(int index);
    Q_INVOKABLE void setAllVisibleCvsChecked(bool checked);
    Q_INVOKABLE void removeCheckedCvs();
    Q_INVOKABLE void restoreCheckedCvs();
    Q_INVOKABLE void permanentlyDeleteCheckedCvs();
    Q_INVOKABLE void cancelCvMutation();
    void recordCvUse(
        const CvDocument& document,
        const QString& applicationId,
        CvImportDisposition disposition);
    void recordCvReplacement(
        const QString& previousCvId,
        const CvDocument& document,
        const QString& applicationId,
        CvImportDisposition disposition);
    void recordApplicationsDeleted(const QStringList& applicationIds);

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
    void libraryViewChanged();
    void checkedCvsChanged();
    void mutatingCvsChanged();
    void pendingDeletionCountChanged();
    void mutationAvailabilityChanged();
    void cvImportCompleted(
        quint64 operationId,
        const QString& fileName,
        bool success,
        const QString& disposition,
        const QString& message);
    void cvMutationCompleted(
        int deletedCount,
        int archivedCount,
        int restoredCount,
        int skippedCount,
        int failedCount,
        bool cancelled,
        const QString& message);
    void importQueueDrained();
    void operationFailed(QString message);

private:
    struct QueuedCvImport final
    {
        QUrl sourceUrl_;
    };

    QVariantMap cvToMap(int sourceRow) const;
    void publishCvDocument(
        const CvDocument& document,
        CvImportDisposition disposition,
        const QString& applicationId = {},
        const QString& previousCvId = {});
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void updateLinkedApplications();
    void startNextCvImport();
    void publishPendingImportStateChange(int previousCount);
    void handleCvImport(const CvImportSaveOutcome& outcome);
    void releaseActiveCvImport();
    void submitCvMutation(DataRemovalKind kind);
    void handleRemovalCompleted(const DataRemovalBatchOutcome& outcome);
    void releaseCvMutation();

private:
    CvRepository& repository_;
    CvFileAccessService& fileAccessService_;
    CvImportWorker& importWorker_;
    DataRemovalWorker* removalWorker_ = nullptr;
    StorageMutationGate* mutationGate_ = nullptr;
    CvListModel cvModel_;
    RoleFilterProxyModel filteredCvModel_;
    RelationFilterProxyModel linkedApplicationsModel_;
    StableIdSelectionTracker selectionTracker_;
    BulkIdSelectionTracker bulkSelectionTracker_;
    QString categoryFilter_;
    QString languageFilter_;
    QString sortMode_ = QStringLiteral("Last Modified");
    LibraryView libraryView_ = LibraryView::Active;
    SerialOperationQueue<QueuedCvImport> importQueue_;
    std::shared_ptr<CancellationState> activeMutationCancellation_;
    quint64 activeMutationOperationId_ = 0;
    quint64 nextMutationOperationId_ = 0;
    std::optional<DataRemovalKind> activeMutationKind_;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_CVS_CVLIBRARYCONTROLLER_HPP
