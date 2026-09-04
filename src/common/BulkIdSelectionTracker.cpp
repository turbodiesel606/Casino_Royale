#include "BulkIdSelectionTracker.hpp"

#include "ModelRoleUtils.hpp"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>

#include <algorithm>

BulkIdSelectionTracker::BulkIdSelectionTracker(
    QAbstractProxyModel& proxyModel,
    int idRole)
    : proxyModel_{proxyModel}
    , idRole_{idRole}
{
    const auto reconcileSelection = [this]() { reconcile(); };
    connect(&proxyModel_, &QAbstractItemModel::rowsInserted, this, reconcileSelection);
    connect(&proxyModel_, &QAbstractItemModel::rowsRemoved, this, reconcileSelection);
    connect(&proxyModel_, &QAbstractItemModel::modelReset, this, reconcileSelection);
    connect(&proxyModel_, &QAbstractItemModel::layoutChanged, this, reconcileSelection);
    connect(
        &proxyModel_,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
            if (roles.isEmpty() || roles.contains(idRole_)) {
                reconcile();
            }
        });
}

QStringList BulkIdSelectionTracker::selectedIds() const
{
    QStringList ordered;
    for (const auto& id : visibleIds()) {
        if (selectedIds_.contains(id)) {
            ordered.append(id);
        }
    }
    return ordered;
}

int BulkIdSelectionTracker::selectedCount() const
{
    return selectedIds_.size();
}

bool BulkIdSelectionTracker::contains(const QString& id) const
{
    return selectedIds_.contains(id);
}

bool BulkIdSelectionTracker::allVisibleSelected() const
{
    const auto ids = visibleIds();
    return !ids.isEmpty()
        && std::all_of(ids.cbegin(), ids.cend(), [this](const QString& id) {
            return selectedIds_.contains(id);
        });
}

bool BulkIdSelectionTracker::someVisibleSelected() const
{
    const auto ids = visibleIds();
    const auto selectedVisible = std::count_if(
        ids.cbegin(),
        ids.cend(),
        [this](const QString& id) { return selectedIds_.contains(id); });
    return selectedVisible > 0 && selectedVisible < ids.size();
}

void BulkIdSelectionTracker::toggleRow(int proxyRow)
{
    const auto id = idAt(proxyRow);
    if (id.isEmpty()) {
        return;
    }
    if (selectedIds_.contains(id)) {
        selectedIds_.remove(id);
    } else {
        selectedIds_.insert(id);
    }
    emit selectionChanged();
}

void BulkIdSelectionTracker::setAllVisibleSelected(bool selected)
{
    const auto previousIds = selectedIds_;
    for (const auto& id : visibleIds()) {
        if (selected) {
            selectedIds_.insert(id);
        } else {
            selectedIds_.remove(id);
        }
    }
    publishIfChanged(previousIds);
}

void BulkIdSelectionTracker::clear()
{
    if (selectedIds_.isEmpty()) {
        return;
    }
    selectedIds_.clear();
    emit selectionChanged();
}

void BulkIdSelectionTracker::removeIds(const QStringList& ids)
{
    const auto previousIds = selectedIds_;
    for (const auto& id : ids) {
        selectedIds_.remove(id);
    }
    publishIfChanged(previousIds);
}

QString BulkIdSelectionTracker::idAt(int proxyRow) const
{
    return common::model::stringRoleAt(proxyModel_, proxyRow, idRole_);
}

QStringList BulkIdSelectionTracker::visibleIds() const
{
    QStringList ids;
    ids.reserve(proxyModel_.rowCount());
    for (int row = 0; row < proxyModel_.rowCount(); ++row) {
        const auto id = idAt(row);
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

void BulkIdSelectionTracker::reconcile()
{
    const auto previousIds = selectedIds_;
    const auto visibleIdList = visibleIds();
    const QSet<QString> visible{visibleIdList.cbegin(), visibleIdList.cend()};
    selectedIds_.intersect(visible);
    publishIfChanged(previousIds);
}

void BulkIdSelectionTracker::publishIfChanged(const QSet<QString>& previousIds)
{
    if (previousIds != selectedIds_) {
        emit selectionChanged();
    }
}
