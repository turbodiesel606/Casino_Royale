#ifndef JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP
#define JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/RelationFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "CompanyListModel.hpp"

#include <QObject>
#include <QVariantMap>

class QAbstractItemModel;
class ContactListModel;
class JobApplicationListModel;

class CompanyDirectoryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* companyModel READ companyModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* linkedJobsModel READ linkedJobsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* linkedContactsModel READ linkedContactsModel CONSTANT)
    Q_PROPERTY(int companyCount READ companyCount NOTIFY companyCountChanged)
    Q_PROPERTY(int selectedCompanyIndex READ selectedCompanyIndex NOTIFY selectedCompanyIndexChanged)
    Q_PROPERTY(QString selectedCompanyId READ selectedCompanyId NOTIFY selectedCompanyIdChanged)
    Q_PROPERTY(QVariantMap selectedCompany READ selectedCompany NOTIFY selectedCompanyChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)

public:
    CompanyDirectoryController(const JobApplicationListModel& applicationsModel, const ContactListModel& contactModel, QObject* parent = nullptr);
    CompanyDirectoryController(
        QVector<Company> companies,
        const JobApplicationListModel& applicationsModel,
        const ContactListModel& contactModel,
        QObject* parent = nullptr);

    QAbstractItemModel* companyModel();
    QAbstractItemModel* linkedJobsModel();
    QAbstractItemModel* linkedContactsModel();
    int companyCount() const;
    int selectedCompanyIndex() const;
    QString selectedCompanyId() const;
    QVariantMap selectedCompany() const;
    QString searchText() const;
    QString sortMode() const;
    QString resultSummary() const;

    Q_INVOKABLE void selectCompany(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setSortMode(const QString& sortMode);
    Q_INVOKABLE void clearFilters();

    void publishCompany(const Company& company);

signals:
    void companyCountChanged();
    void selectedCompanyIndexChanged();
    void selectedCompanyIdChanged();
    void selectedCompanyChanged();
    void searchTextChanged();
    void sortModeChanged();
    void resultSummaryChanged();

private:
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void refreshCompanyJobCounts();
    QVariantMap companyToMap(int sourceRow) const;
    void updateLinkedModels();

    const JobApplicationListModel& applicationsModel_;
    CompanyListModel companyModel_;
    RoleFilterProxyModel filteredCompanyModel_;
    RelationFilterProxyModel linkedJobsModel_;
    RelationFilterProxyModel linkedContactsModel_;
    StableIdSelectionTracker selectionTracker_;
    QString sortMode_ = QStringLiteral("Name");
};

#endif // JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP
