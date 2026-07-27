#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "cvs/CvDocument.hpp"
#include "JobApplicationListModel.hpp"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

class QAbstractItemModel;
class AddJobService;

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

public:
    explicit JobApplicationsController(QObject* parent = nullptr);
    explicit JobApplicationsController(QVector<JobApplication> applications, QObject* parent = nullptr);
    JobApplicationsController(
        QVector<JobApplication> applications,
        AddJobService& addJobService,
        QObject* parent = nullptr);

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

    Q_INVOKABLE void selectApplication(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setStatusFilter(const QString& status);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE QStringList validateSelectedApplication() const;
    Q_INVOKABLE void createApplication(const QVariantMap& formValues, const QUrl& selectedCvUrl);

signals:
    void applicationCountChanged();
    void selectedApplicationIndexChanged();
    void selectedApplicationIdChanged();
    void selectedApplicationChanged();
    void searchTextChanged();
    void statusFilterChanged();
    void resultSummaryChanged();
    void savingChanged();
    void applicationCreated(const QString& applicationId);
    void companyResolved(const QString& companyId, const QString& companyName);
    void saveFailed(const QVariantMap& fieldErrors, const QString& message);
    void cvUsed(const CvDocument& document, const QString& applicationId, bool wasInserted);

private:
    const JobApplication* selectedSourceApplication() const;
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void handleVisibleCountChanged();
    QVariantMap applicationToMap(const JobApplication& application) const;

    JobApplicationListModel applicationsModel_;
    RoleFilterProxyModel filteredApplicationsModel_;
    StableIdSelectionTracker selectionTracker_;
    QString searchText_;
    QString statusFilter_;
    int publishedApplicationCount_ = 0;
    bool visibleCountNotificationsSuppressed_ = false;
    AddJobService* addJobService_ = nullptr;
    bool saving_ = false;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONSCONTROLLER_HPP
