#ifndef JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP
#define JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "ContactInteractionListModel.hpp"
#include "ContactListModel.hpp"

#include <QObject>
#include <QVariantMap>

class QAbstractItemModel;

class ContactDirectoryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* contactModel READ contactModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* interactionHistoryModel READ interactionHistoryModel NOTIFY interactionHistoryModelChanged)
    Q_PROPERTY(int contactCount READ contactCount NOTIFY contactModelChanged)
    Q_PROPERTY(int selectedContactIndex READ selectedContactIndex NOTIFY selectedContactChanged)
    Q_PROPERTY(QString selectedContactId READ selectedContactId NOTIFY selectedContactChanged)
    Q_PROPERTY(QVariantMap selectedContact READ selectedContact NOTIFY selectedContactChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(QString companyFilter READ companyFilter WRITE setCompanyFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString channelFilter READ channelFilter WRITE setChannelFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)

public:
    explicit ContactDirectoryController(ContactListModel& contactModel, QObject* parent = nullptr);

    QAbstractItemModel* contactModel();
    QAbstractItemModel* interactionHistoryModel();
    int contactCount() const;
    int selectedContactIndex() const;
    QString selectedContactId() const;
    QVariantMap selectedContact() const;
    QString searchText() const;
    QString companyFilter() const;
    QString channelFilter() const;
    QString sortMode() const;
    QString resultSummary() const;

    Q_INVOKABLE void selectContact(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setCompanyFilter(const QString& company);
    Q_INVOKABLE void setChannelFilter(const QString& channel);
    Q_INVOKABLE void setSortMode(const QString& sortMode);
    Q_INVOKABLE void clearFilters();

signals:
    void contactModelChanged();
    void interactionHistoryModelChanged();
    void selectedContactChanged();
    void filtersChanged();
    void resultSummaryChanged();

private:
    const Contact* selectedSourceContact() const;
    int selectedSourceRow() const;
    void refreshSelection(bool selectedDataChanged = false);
    QVariantMap contactToMap(const Contact& contact) const;
    void updateInteractionHistory();

    ContactListModel& contactModel_;
    RoleFilterProxyModel filteredContactModel_;
    ContactInteractionListModel interactionHistoryModel_;
    QString searchText_;
    QString companyFilter_;
    QString channelFilter_;
    QString sortMode_ = QStringLiteral("Name");
    QString selectedContactId_;
    int selectedContactIndex_ = -1;
};

#endif // JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP
