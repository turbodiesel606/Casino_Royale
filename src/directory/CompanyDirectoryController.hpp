#ifndef JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP
#define JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "CompanyListModel.hpp"
#include "LinkedCompanyContactsModel.hpp"
#include "LinkedCompanyJobsModel.hpp"

#include <QObject>
#include <QVariantMap>

class QAbstractItemModel;

class CompanyDirectoryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* companyModel READ companyModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* linkedJobsModel READ linkedJobsModel NOTIFY linkedModelsChanged)
    Q_PROPERTY(QAbstractItemModel* linkedContactsModel READ linkedContactsModel NOTIFY linkedModelsChanged)
    Q_PROPERTY(int companyCount READ companyCount NOTIFY companyModelChanged)
    Q_PROPERTY(int selectedCompanyIndex READ selectedCompanyIndex NOTIFY selectedCompanyChanged)
    Q_PROPERTY(QString selectedCompanyId READ selectedCompanyId NOTIFY selectedCompanyChanged)
    Q_PROPERTY(QVariantMap selectedCompany READ selectedCompany NOTIFY selectedCompanyChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)
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

    void publishCompany(const QString& companyId, const QString& companyName);

signals:
    void companyModelChanged();
    void linkedModelsChanged();
    void selectedCompanyChanged();
    void filtersChanged();
    void resultSummaryChanged();

private:
    const Company* selectedSourceCompany() const;
    int selectedSourceRow() const;
    void refreshSelection(bool selectedDataChanged = false);
    void refreshCompanyJobCounts();
    QVariantMap companyToMap(const Company& company) const;
    void updateLinkedModels();

    const JobApplicationListModel& applicationsModel_;
    CompanyListModel companyModel_;
    RoleFilterProxyModel filteredCompanyModel_;
    LinkedCompanyJobsModel linkedJobsModel_;
    LinkedCompanyContactsModel linkedContactsModel_;
    QString searchText_;
    QString sortMode_ = QStringLiteral("Name");
    QString selectedCompanyId_;
    int selectedCompanyIndex_ = -1;
};

#endif // JOBTRACKER_SRC_DIRECTORY_COMPANYDIRECTORYCONTROLLER_HPP
