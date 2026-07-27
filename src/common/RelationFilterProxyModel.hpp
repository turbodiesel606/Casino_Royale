#ifndef JOBTRACKER_SRC_COMMON_RELATIONFILTERPROXYMODEL_HPP
#define JOBTRACKER_SRC_COMMON_RELATIONFILTERPROXYMODEL_HPP

#include <QSortFilterProxyModel>

// Exposes source rows whose relation role matches the selected domain ID.
// Source role names and Qt model mutation notifications are preserved.
class RelationFilterProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RelationFilterProxyModel(
        const QAbstractItemModel& sourceModel,
        int relationRole,
        QObject* parent = nullptr);

    QString selectedId() const;
    void setSelectedId(const QString& selectedId);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int relationRole_;
    QString selectedId_;
};

#endif // JOBTRACKER_SRC_COMMON_RELATIONFILTERPROXYMODEL_HPP
