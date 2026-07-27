#ifndef JOBTRACKER_SRC_COMMON_STABLEIDSELECTIONTRACKER_HPP
#define JOBTRACKER_SRC_COMMON_STABLEIDSELECTIONTRACKER_HPP

#include <QModelIndex>
#include <QObject>
#include <QString>

class QAbstractProxyModel;

/**
 * Keeps a proxy-model selection anchored to a stable domain ID.
 *
 * The tracker owns the selected ID and proxy row, reconciles them after every
 * relevant proxy-model mutation, and reports which part of the selection
 * contract changed. Controllers remain responsible for domain-specific side
 * effects such as refreshing linked models.
 */
class StableIdSelectionTracker final : public QObject
{
    Q_OBJECT

public:
    StableIdSelectionTracker(QAbstractProxyModel& proxyModel, int idRole);

    QString selectedId() const;
    int selectedRow() const;
    QModelIndex selectedSourceIndex() const;

    void selectRow(int proxyRow);

    /** Defers reconciliation until a controller-owned proxy update completes. */
    void beginModelUpdate();
    void endModelUpdate();

    void synchronize();

signals:
    void selectionChanged(bool selectedIdChanged, bool selectedRowChanged, bool selectedDataChanged);

private:
    QString idAt(int proxyRow) const;
    int rowForId(const QString& id) const;
    void requestReconcile(bool selectedDataChanged = false);
    void reconcile(bool selectedDataChanged = false);
    void applySelection(QString selectedId, int selectedRow, bool selectedDataChanged);

    QAbstractProxyModel& proxyModel_;
    int idRole_;
    QString selectedId_;
    int selectedRow_ = -1;
    int modelUpdateDepth_ = 0;
    bool reconcilePending_ = false;
    bool selectedDataChangePending_ = false;
};

#endif // JOBTRACKER_SRC_COMMON_STABLEIDSELECTIONTRACKER_HPP
