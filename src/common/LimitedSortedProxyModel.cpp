#include "LimitedSortedProxyModel.hpp"

#include <QDate>
#include <QDateTime>
#include <QMetaType>

#include <algorithm>
#include <utility>

namespace {

int compareVariants(const QVariant& left, const QVariant& right)
{
    if (left.metaType().id() == QMetaType::QDateTime
        && right.metaType().id() == QMetaType::QDateTime) {
        const auto leftValue = left.toDateTime();
        const auto rightValue = right.toDateTime();
        if (leftValue.isValid() != rightValue.isValid()) {
            return leftValue.isValid() ? 1 : -1;
        }
        return leftValue < rightValue ? -1 : leftValue > rightValue ? 1 : 0;
    }

    if (left.metaType().id() == QMetaType::QDate
        && right.metaType().id() == QMetaType::QDate) {
        const auto leftValue = left.toDate();
        const auto rightValue = right.toDate();
        if (leftValue.isValid() != rightValue.isValid()) {
            return leftValue.isValid() ? 1 : -1;
        }
        return leftValue < rightValue ? -1 : leftValue > rightValue ? 1 : 0;
    }

    const auto leftType = left.metaType().id();
    const auto rightType = right.metaType().id();
    const auto isNumericType = [](int type) {
        return type == QMetaType::Int
            || type == QMetaType::UInt
            || type == QMetaType::LongLong
            || type == QMetaType::ULongLong
            || type == QMetaType::Float
            || type == QMetaType::Double;
    };
    if (isNumericType(leftType) && isNumericType(rightType)) {
        const auto leftValue = left.toDouble();
        const auto rightValue = right.toDouble();
        return leftValue < rightValue ? -1 : leftValue > rightValue ? 1 : 0;
    }

    return QString::localeAwareCompare(left.toString(), right.toString());
}

} // namespace

LimitedSortedProxyModel::LimitedSortedProxyModel(
    const QAbstractItemModel& sourceModel,
    int sortRole,
    int rowLimit,
    int tieBreakRole,
    Qt::SortOrder sortOrder,
    QObject* parent)
    : QAbstractProxyModel(parent)
    , sortRole_(sortRole)
    , rowLimit_(std::max(0, rowLimit))
    , tieBreakRole_(tieBreakRole)
    , sortOrder_(sortOrder)
{
    // QAbstractProxyModel requires a mutable pointer, but this proxy only
    // observes the source model and never mutates it.
    setSourceModel(const_cast<QAbstractItemModel*>(&sourceModel));
}

QModelIndex LimitedSortedProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid()
        || proxyIndex.row() < 0
        || proxyIndex.row() >= sourceRows_.size()
        || proxyIndex.column() < 0
        || proxyIndex.column() >= columnCount()) {
        return {};
    }

    const auto& sourceRow = sourceRows_.at(proxyIndex.row());
    return sourceModel()->index(sourceRow.row(), proxyIndex.column(), sourceRow.parent());
}

QModelIndex LimitedSortedProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != sourceModel()) {
        return {};
    }

    for (int row = 0; row < sourceRows_.size(); ++row) {
        const auto& candidate = sourceRows_.at(row);
        if (candidate.row() == sourceIndex.row() && candidate.parent() == sourceIndex.parent()) {
            return index(row, sourceIndex.column());
        }
    }

    return {};
}

QModelIndex LimitedSortedProxyModel::index(
    int row,
    int column,
    const QModelIndex& parent) const
{
    if (parent.isValid()
        || row < 0
        || row >= sourceRows_.size()
        || column < 0
        || column >= columnCount()) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex LimitedSortedProxyModel::parent(const QModelIndex& child) const
{
    Q_UNUSED(child)
    return {};
}

int LimitedSortedProxyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : sourceRows_.size();
}

int LimitedSortedProxyModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() || sourceModel() == nullptr
        ? 0
        : sourceModel()->columnCount();
}

QVariant LimitedSortedProxyModel::data(const QModelIndex& index, int role) const
{
    const auto sourceIndex = mapToSource(index);
    return sourceIndex.isValid() ? sourceModel()->data(sourceIndex, role) : QVariant{};
}

QHash<int, QByteArray> LimitedSortedProxyModel::roleNames() const
{
    return roleNames_;
}

void LimitedSortedProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    if (this->sourceModel() == sourceModel) {
        return;
    }
    if (this->sourceModel() != nullptr) {
        disconnect(this->sourceModel(), nullptr, this, nullptr);
    }
    QAbstractProxyModel::setSourceModel(sourceModel);
    roleNames_ = sourceModel != nullptr
        ? sourceModel->roleNames()
        : QHash<int, QByteArray>{};
    connectSourceModel();
    rebuild();
}

void LimitedSortedProxyModel::setRoleName(int role, QByteArray name)
{
    roleNames_.insert(role, std::move(name));
}

void LimitedSortedProxyModel::connectSourceModel()
{
    if (sourceModel() == nullptr) {
        return;
    }
    const auto rebuildModel = [this]() { rebuild(); };
    connect(sourceModel(), &QAbstractItemModel::rowsInserted, this, rebuildModel);
    connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, rebuildModel);
    connect(sourceModel(), &QAbstractItemModel::rowsMoved, this, rebuildModel);
    connect(sourceModel(), &QAbstractItemModel::modelReset, this, rebuildModel);
    connect(sourceModel(), &QAbstractItemModel::layoutChanged, this, rebuildModel);
    connect(sourceModel(), &QAbstractItemModel::dataChanged, this, rebuildModel);
}

void LimitedSortedProxyModel::rebuild()
{
    beginResetModel();
    rebuildRows();
    endResetModel();
}

void LimitedSortedProxyModel::rebuildRows()
{
    sourceRows_.clear();
    if (sourceModel() == nullptr || rowLimit_ == 0) {
        return;
    }

    QVector<QPersistentModelIndex> sortedRows;
    sortedRows.reserve(sourceModel()->rowCount());
    for (int row = 0; row < sourceModel()->rowCount(); ++row) {
        sortedRows.append(QPersistentModelIndex{sourceModel()->index(row, 0)});
    }

    std::stable_sort(
        sortedRows.begin(),
        sortedRows.end(),
        [this](const auto& left, const auto& right) {
            return sourceIndexPrecedes(left, right);
        });
    if (sortedRows.size() > rowLimit_) {
        sortedRows.resize(rowLimit_);
    }
    sourceRows_ = std::move(sortedRows);
}

bool LimitedSortedProxyModel::sourceIndexPrecedes(
    const QPersistentModelIndex& left,
    const QPersistentModelIndex& right) const
{
    const auto sortComparison = compareVariants(
        sourceModel()->data(left, sortRole_),
        sourceModel()->data(right, sortRole_));
    if (sortComparison != 0) {
        return sortOrder_ == Qt::AscendingOrder
            ? sortComparison < 0
            : sortComparison > 0;
    }

    const auto tieComparison = compareVariants(
        sourceModel()->data(left, tieBreakRole_),
        sourceModel()->data(right, tieBreakRole_));
    return tieComparison != 0 ? tieComparison < 0 : left.row() < right.row();
}
