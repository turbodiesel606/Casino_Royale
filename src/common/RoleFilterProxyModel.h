#pragma once

#include <QSortFilterProxyModel>
#include <QHash>
#include <QVector>

class RoleFilterProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit RoleFilterProxyModel(QObject* parent = nullptr);

    QString searchText() const;
    void setSearchText(const QString& searchText);
    void setSearchRoles(QVector<int> roles);
    void setExactFilter(int role, const QString& value);
    void clearExactFilter();
    void setRequiredNonEmptyRole(int role);
    void clearRequiredNonEmptyRole();
    void setSort(int role, Qt::SortOrder order = Qt::AscendingOrder);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    bool rowMatchesSearch(int sourceRow, const QModelIndex& sourceParent) const;
    bool rowMatchesExactFilter(int sourceRow, const QModelIndex& sourceParent) const;
    bool rowMatchesRequiredNonEmptyRole(int sourceRow, const QModelIndex& sourceParent) const;

    QString searchText_;
    QVector<int> searchRoles_;
    QHash<int, QString> exactFilters_;
    int requiredNonEmptyRole_ = -1;
};
