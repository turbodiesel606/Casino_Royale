#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "JobApplicationDraft.hpp"
#include "JobApplicationListModel.hpp"
#include "cvs/CvDocument.hpp"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <deque>
#include <memory>
#include <optional>

class QAbstractItemModel;
class AddJobWorker;
class CancellationState;
struct AddJobSaveOutcome;

// Exposes job applications, selection, filtering, and validation to the user interface through Qt models and properties.
// Owns the validated Add Job FIFO and publishes worker results to GUI-thread models and QML-facing signals.
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

public:
    JobApplicationsController(
        QVector<JobApplication> applications,
        AddJobWorker& addJobWorker,
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

    Q_INVOKABLE void selectApplication(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setStatusFilter(const QString& status);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE QStringList validateSelectedApplication() const;
    Q_INVOKABLE void createApplication(const QVariantMap& formValues, const QUrl& selectedCvUrl);
    Q_INVOKABLE void cancelCreateApplication();
    Q_INVOKABLE void cancelAllCreateApplications();

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
    void applicationQueued(quint64 operationId);
    void applicationSaveCompleted(
        quint64 operationId,
        const QString& jobTitle,
        bool success,
        const QString& message);
    void saveQueueDrained();
    void applicationCreated(const QString& applicationId);
    void companyResolved(const QString& companyId, const QString& companyName);
    void saveFailed(const QVariantMap& fieldErrors, const QString& message);
    void cvUsed(const CvDocument& document, const QString& applicationId, bool wasInserted);

private:
    struct QueuedCreateApplication final
    {
        quint64 operationId_ = 0;
        NormalizedJobApplicationDraft draft_;
        QUrl selectedCvUrl_;
    };

    const JobApplication* selectedSourceApplication() const;
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void handleVisibleCountChanged();
    void startNextCreateApplication();
    void publishPendingSaveStateChange(int previousCount);
    void handleAddJobSave(const AddJobSaveOutcome& outcome);
    bool isActiveCreateOutcome(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation) const;
    void releaseActiveCreateApplication();
    QVariantMap applicationToMap(int sourceRow) const;

    JobApplicationListModel applicationsModel_;
    RoleFilterProxyModel filteredApplicationsModel_;
    StableIdSelectionTracker selectionTracker_;
    QString searchText_;
    QString statusFilter_;
    int publishedApplicationCount_ = 0;
    bool visibleCountNotificationsSuppressed_ = false;
    AddJobWorker& addJobWorker_;
    std::deque<QueuedCreateApplication> createQueue_;
    std::optional<QueuedCreateApplication> activeCreateApplication_;
    std::shared_ptr<CancellationState> activeCreateCancellation_;
    quint64 nextCreateOperationId_ = 0;
    bool suppressActiveCompletionNotification_ = false;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
