#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP

#include "common/BulkIdSelectionTracker.hpp"
#include "common/RoleFilterProxyModel.hpp"
#include "common/SerialOperationQueue.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "JobApplicationDraft.hpp"
#include "JobApplicationListModel.hpp"
#include "cvs/CvDocument.hpp"
#include "cvs/CvImportService.hpp"
#include "directory/Company.hpp"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <memory>
#include <variant>

class QAbstractItemModel;
class JobSaveWorker;
class DataRemovalWorker;
class StorageMutationGate;
class CancellationState;
struct AddJobSaveOutcome;
struct UpdateJobSaveOutcome;

// Exposes job applications, selection, filtering, and validation to the user interface through Qt models and properties.
// Owns the validated create/update FIFO and publishes worker results to
// GUI-thread models and QML-facing signals.
class JobApplicationsController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* applicationsModel READ applicationsModel CONSTANT)
    Q_PROPERTY(int applicationCount READ applicationCount NOTIFY applicationCountChanged)
    Q_PROPERTY(int selectedApplicationIndex READ selectedApplicationIndex NOTIFY selectedApplicationIndexChanged)
    Q_PROPERTY(QString selectedApplicationId READ selectedApplicationId NOTIFY selectedApplicationIdChanged)
    Q_PROPERTY(QVariantMap selectedApplication READ selectedApplication NOTIFY selectedApplicationChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString statusFilter READ statusFilter WRITE setStatusFilter NOTIFY statusFilterChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)
    Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)
    Q_PROPERTY(int pendingSaveCount READ pendingSaveCount NOTIFY pendingSaveCountChanged)
    Q_PROPERTY(bool updatingApplication READ updatingApplication NOTIFY updatingApplicationChanged)
    Q_PROPERTY(QStringList checkedApplicationIds READ checkedApplicationIds NOTIFY checkedApplicationsChanged)
    Q_PROPERTY(int checkedApplicationCount READ checkedApplicationCount NOTIFY checkedApplicationsChanged)
    Q_PROPERTY(bool allVisibleApplicationsChecked READ allVisibleApplicationsChecked NOTIFY checkedApplicationsChanged)
    Q_PROPERTY(bool someVisibleApplicationsChecked READ someVisibleApplicationsChecked NOTIFY checkedApplicationsChanged)
    Q_PROPERTY(bool deletingApplications READ deletingApplications NOTIFY deletingApplicationsChanged)
    Q_PROPERTY(int pendingDeletionCount READ pendingDeletionCount NOTIFY pendingDeletionCountChanged)
    Q_PROPERTY(bool canDeleteApplications READ canDeleteApplications NOTIFY deletionAvailabilityChanged)

public:
    JobApplicationsController(
        QVector<JobApplication> applications,
        JobSaveWorker& jobSaveWorker,
        QObject* parent = nullptr);
    JobApplicationsController(
        QVector<JobApplication> applications,
        JobSaveWorker& jobSaveWorker,
        DataRemovalWorker& removalWorker,
        StorageMutationGate& mutationGate,
        QObject* parent = nullptr);
    ~JobApplicationsController() override;

    QAbstractItemModel* applicationsModel();
    JobApplicationListModel& jobApplicationListModel();
    const JobApplicationListModel& jobApplicationListModel() const;
    int applicationCount() const;
    int selectedApplicationIndex() const;
    QString selectedApplicationId() const;
    QVariantMap selectedApplication() const;
    QString searchText() const;
    QString statusFilter() const;
    QString resultSummary() const;
    bool saving() const;
    int pendingSaveCount() const;
    bool updatingApplication() const;
    QStringList checkedApplicationIds() const;
    int checkedApplicationCount() const;
    bool allVisibleApplicationsChecked() const;
    bool someVisibleApplicationsChecked() const;
    bool deletingApplications() const;
    int pendingDeletionCount() const;
    bool canDeleteApplications() const;

    Q_INVOKABLE void selectApplication(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setStatusFilter(const QString& status);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE QStringList validateSelectedApplication() const;
    Q_INVOKABLE void createApplication(const QVariantMap& formValues, const QUrl& selectedCvUrl);
    Q_INVOKABLE void updateApplication(
        const QString& applicationId,
        const QVariantMap& formValues,
        const QUrl& replacementCvUrl);
    Q_INVOKABLE void cancelCreateApplication();
    Q_INVOKABLE void cancelAllJobSaves();
    Q_INVOKABLE void toggleApplicationChecked(int index);
    Q_INVOKABLE void setAllVisibleApplicationsChecked(bool checked);
    Q_INVOKABLE void deleteCheckedApplications();
    Q_INVOKABLE void cancelApplicationDeletion();

signals:
    void applicationCountChanged();
    void selectedApplicationIndexChanged();
    void selectedApplicationIdChanged();
    void selectedApplicationChanged();
    void searchTextChanged();
    void statusFilterChanged();
    void resultSummaryChanged();
    void savingChanged();
    void pendingSaveCountChanged();
    void updatingApplicationChanged();
    void applicationQueued(quint64 operationId);
    void applicationSaveCompleted(
        quint64 operationId,
        const QString& jobTitle,
        bool success,
        const QString& message);
    void applicationUpdateQueued(quint64 operationId, const QString& applicationId);
    void applicationUpdateRejected(
        quint64 operationId,
        const QString& applicationId,
        const QVariantMap& fieldErrors,
        const QString& message);
    void applicationUpdateCompleted(
        quint64 operationId,
        const QString& applicationId,
        const QString& jobTitle,
        bool success,
        const QVariantMap& fieldErrors,
        const QString& message);
    void saveQueueDrained();
    void checkedApplicationsChanged();
    void deletingApplicationsChanged();
    void pendingDeletionCountChanged();
    void deletionAvailabilityChanged();
    void applicationDeletionCompleted(
        int deletedCount,
        int failedCount,
        bool cancelled,
        const QString& message);
    void applicationsDeleted(const QStringList& applicationIds);
    void applicationCreated(const QString& applicationId);
    void companyResolved(const Company& company);
    // saveFailed is a Qt signal handled by JobFormPage.qml (onSaveFailed).
    void saveFailed(const QVariantMap& fieldErrors, const QString& message);
    void cvUsed(
        const CvDocument& document,
        const QString& applicationId,
        CvImportDisposition disposition);
    void cvReplaced(
        const QString& previousCvId,
        const CvDocument& document,
        const QString& applicationId,
        CvImportDisposition disposition);

private:
    struct QueuedCreateApplication final
    {
        NormalizedJobApplicationDraft draft_;
        QUrl selectedCvUrl_;
    };

    struct QueuedUpdateApplication final
    {
        QString applicationId_;
        NormalizedJobApplicationDraft draft_;
        QUrl replacementCvUrl_;
    };

    using QueuedJobSave = std::variant<QueuedCreateApplication, QueuedUpdateApplication>;

    const JobApplication* selectedSourceApplication() const;
    const JobApplication* sourceApplicationById(const QString& applicationId) const;
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void startNextJobSave();
    void publishPendingSaveStateChange(int previousCount);
    void handleAddJobSave(const AddJobSaveOutcome& outcome);
    void handleUpdateJobSave(const UpdateJobSaveOutcome& outcome);
    bool isActiveSaveOutcome(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation,
        bool expectUpdate) const;
    void releaseActiveJobSave();
    void clearPendingUpdate();
    void handleRemovalCompleted(const struct DataRemovalBatchOutcome& outcome);
    void releaseApplicationDeletion();
    QVariantMap applicationToMap(int sourceRow) const;

    JobApplicationListModel applicationsModel_;
    RoleFilterProxyModel filteredApplicationsModel_;
    StableIdSelectionTracker selectionTracker_;
    BulkIdSelectionTracker bulkSelectionTracker_;
    QString statusFilter_;
    JobSaveWorker& jobSaveWorker_;
    DataRemovalWorker* removalWorker_ = nullptr;
    StorageMutationGate* mutationGate_ = nullptr;
    SerialOperationQueue<QueuedJobSave> saveQueue_;
    quint64 pendingUpdateOperationId_ = 0;
    QString pendingUpdateApplicationId_;
    std::shared_ptr<CancellationState> activeDeletionCancellation_;
    quint64 activeDeletionOperationId_ = 0;
    quint64 nextDeletionOperationId_ = 0;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
