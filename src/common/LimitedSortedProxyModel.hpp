#ifndef JOBTRACKER_SRC_COMMON_LIMITEDSORTEDPROXYMODEL_HPP
#define JOBTRACKER_SRC_COMMON_LIMITEDSORTEDPROXYMODEL_HPP

#include <QAbstractProxyModel>
#include <QHash>
#include <QPersistentModelIndex>
#include <QVector>

// Exposes at most rowLimit source rows in typed-role sort order. Source
// mutations rebuild the small deterministic projection and the tie-break role
// keeps equal sort values stable independently of source row order.
class LimitedSortedProxyModel final : public QAbstractProxyModel
{
    Q_OBJECT

public:
    LimitedSortedProxyModel(
        const QAbstractItemModel& sourceModel,
        int sortRole,
        int rowLimit,
        int tieBreakRole,
        Qt::SortOrder sortOrder = Qt::DescendingOrder,
        QObject* parent = nullptr);

    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex index(
        int row,
        int column,
        const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    void setRoleName(int role, QByteArray name);

private:
    void connectSourceModel();
    void rebuild();
    void rebuildRows();
    bool sourceIndexPrecedes(
        const QPersistentModelIndex& left,
        const QPersistentModelIndex& right) const;

    int sortRole_;
    int rowLimit_;
    int tieBreakRole_;
    Qt::SortOrder sortOrder_;
    QVector<QPersistentModelIndex> sourceRows_;
    QHash<int, QByteArray> roleNames_;
};

#endif // JOBTRACKER_SRC_COMMON_LIMITEDSORTEDPROXYMODEL_HPP
