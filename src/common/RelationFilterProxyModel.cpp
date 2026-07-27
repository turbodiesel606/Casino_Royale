#include "RelationFilterProxyModel.hpp"

RelationFilterProxyModel::RelationFilterProxyModel(
    const QAbstractItemModel& sourceModel,
    int relationRole,
    QObject* parent)
    : QSortFilterProxyModel(parent)
    , relationRole_(relationRole)
{
    setDynamicSortFilter(true);
    setFilterRole(relationRole_);
    // QAbstractProxyModel requires a mutable pointer, but this proxy only
    // observes the source model and never mutates it.
    setSourceModel(const_cast<QAbstractItemModel*>(&sourceModel));
}

QString RelationFilterProxyModel::selectedId() const
{
    return selectedId_;
}

void RelationFilterProxyModel::setSelectedId(const QString& selectedId)
{
    if (selectedId_ == selectedId) {
        return;
    }

    selectedId_ = selectedId;
    invalidateRowsFilter();
}

bool RelationFilterProxyModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex& sourceParent) const
{
    if (selectedId_.isEmpty()) {
        return false;
    }

    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    return sourceModel()->data(sourceIndex, relationRole_).toString() == selectedId_;
}
