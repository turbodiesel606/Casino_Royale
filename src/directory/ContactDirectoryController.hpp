#ifndef JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP
#define JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP

#include "common/RoleFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "ContactInteractionListModel.hpp"
#include "ContactListModel.hpp"

#include <QObject>
#include <QVariantMap>

class QAbstractItemModel;

class ContactDirectoryController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* contactModel READ contactModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* interactionHistoryModel READ interactionHistoryModel CONSTANT)
    Q_PROPERTY(int contactCount READ contactCount NOTIFY contactCountChanged)
    Q_PROPERTY(int selectedContactIndex READ selectedContactIndex NOTIFY selectedContactIndexChanged)
    Q_PROPERTY(QString selectedContactId READ selectedContactId NOTIFY selectedContactIdChanged)
    Q_PROPERTY(QVariantMap selectedContact READ selectedContact NOTIFY selectedContactChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString companyFilter READ companyFilter WRITE setCompanyFilter NOTIFY companyFilterChanged)
    Q_PROPERTY(QString channelFilter READ channelFilter WRITE setChannelFilter NOTIFY channelFilterChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
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
    void contactCountChanged();
    void selectedContactIndexChanged();
    void selectedContactIdChanged();
    void selectedContactChanged();
    void searchTextChanged();
    void companyFilterChanged();
    void channelFilterChanged();
    void sortModeChanged();
    void resultSummaryChanged();

private:
    const Contact* selectedSourceContact() const;
    void handleSelectionChanged(bool idChanged, bool rowChanged, bool dataChanged);
    void updateInteractionHistory();

    ContactListModel& contactModel_;
    RoleFilterProxyModel filteredContactModel_;
    ContactInteractionListModel interactionHistoryModel_;
    StableIdSelectionTracker selectionTracker_;
    QString companyFilter_;
    QString channelFilter_;
    QString sortMode_ = QStringLiteral("Name");
};

#endif // JOBTRACKER_SRC_DIRECTORY_CONTACTDIRECTORYCONTROLLER_HPP
