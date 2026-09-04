#include "StableIdSelectionTracker.hpp"

#include "ModelRoleUtils.hpp"

#include <QAbstractProxyModel>

#include <utility>

StableIdSelectionTracker::StableIdSelectionTracker(QAbstractProxyModel& proxyModel, int idRole)
    : proxyModel_(proxyModel)
    , idRole_(idRole)
    , visibleRowCount_(proxyModel.rowCount())
{
    connect(
        &proxyModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { requestReconcile(); });
    connect(
        &proxyModel_,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { requestReconcile(); });
    connect(
        &proxyModel_,
        &QAbstractItemModel::rowsMoved,
        this,
        [this]() { requestReconcile(); });
    connect(
        &proxyModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { requestReconcile(!selectedId_.isEmpty()); });
    connect(
        &proxyModel_,
        &QAbstractItemModel::layoutChanged,
        this,
        [this]() { requestReconcile(); });
    connect(
        &proxyModel_,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            const bool selectedDataChanged = selectedRow_ >= topLeft.row()
                && selectedRow_ <= bottomRight.row();
            requestReconcile(selectedDataChanged);
        });

    synchronize();
}

QString StableIdSelectionTracker::selectedId() const
{
    return selectedId_;
}

int StableIdSelectionTracker::selectedRow() const
{
    return selectedRow_;
}

int StableIdSelectionTracker::visibleRowCount() const
{
    return visibleRowCount_;
}

QModelIndex StableIdSelectionTracker::selectedSourceIndex() const
{
    if (selectedRow_ < 0 || selectedRow_ >= proxyModel_.rowCount()) {
        return {};
    }

    return proxyModel_.mapToSource(proxyModel_.index(selectedRow_, 0));
}

void StableIdSelectionTracker::selectRow(int proxyRow)
{
    if (proxyRow < 0 || proxyRow >= proxyModel_.rowCount()) {
        return;
    }

    applySelection(
        common::model::stringRoleAt(proxyModel_, proxyRow, idRole_),
        proxyRow,
        false);
}

void StableIdSelectionTracker::beginModelUpdate()
{
    ++modelUpdateDepth_;
}

void StableIdSelectionTracker::endModelUpdate()
{
    if (modelUpdateDepth_ <= 0) {
        return;
    }

    --modelUpdateDepth_;
    if (modelUpdateDepth_ == 0 && reconcilePending_) {
        const bool selectedDataChanged = selectedDataChangePending_;
        reconcilePending_ = false;
        selectedDataChangePending_ = false;
        reconcile(selectedDataChanged);
    }
}

void StableIdSelectionTracker::synchronize()
{
    requestReconcile();
}

void StableIdSelectionTracker::requestReconcile(bool selectedDataChanged)
{
    if (modelUpdateDepth_ > 0) {
        reconcilePending_ = true;
        selectedDataChangePending_ = selectedDataChangePending_ || selectedDataChanged;
        return;
    }

    reconcile(selectedDataChanged);
}

void StableIdSelectionTracker::reconcile(bool selectedDataChanged)
{
    const auto nextVisibleRowCount = proxyModel_.rowCount();
    const bool didVisibleRowCountChange = visibleRowCount_ != nextVisibleRowCount;
    visibleRowCount_ = nextVisibleRowCount;

    auto nextId = selectedId_;
    auto nextRow = common::model::rowForStringRoleValue(
        proxyModel_,
        idRole_,
        selectedId_);

    if (nextRow < 0) {
        if (proxyModel_.rowCount() > 0) {
            nextRow = 0;
            nextId = common::model::stringRoleAt(proxyModel_, 0, idRole_);
        } else {
            nextId.clear();
        }
    }

    applySelection(std::move(nextId), nextRow, selectedDataChanged);
    if (didVisibleRowCountChange) {
        emit visibleRowCountChanged();
    }
}

void StableIdSelectionTracker::applySelection(
    QString selectedId,
    int selectedRow,
    bool selectedDataChanged)
{
    const bool idChanged = selectedId_ != selectedId;
    const bool rowChanged = selectedRow_ != selectedRow;
    if (!idChanged && !rowChanged && !selectedDataChanged) {
        return;
    }

    selectedId_ = std::move(selectedId);
    selectedRow_ = selectedRow;
    emit selectionChanged(idChanged, rowChanged, selectedDataChanged);
}
