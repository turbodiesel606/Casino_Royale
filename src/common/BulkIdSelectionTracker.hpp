#ifndef JOBTRACKER_SRC_COMMON_BULKIDSELECTIONTRACKER_HPP
#define JOBTRACKER_SRC_COMMON_BULKIDSELECTIONTRACKER_HPP

#include <QObject>
#include <QSet>
#include <QStringList>

class QAbstractProxyModel;

// Tracks a set of checked rows by stable domain ID while deriving visible
// select-all and partial-selection state from a proxy model.
class BulkIdSelectionTracker final : public QObject
{
    Q_OBJECT

public:
    BulkIdSelectionTracker(QAbstractProxyModel& proxyModel, int idRole);

    QStringList selectedIds() const;
    int selectedCount() const;
    bool contains(const QString& id) const;
    bool allVisibleSelected() const;
    bool someVisibleSelected() const;

    void toggleRow(int proxyRow);
    void setAllVisibleSelected(bool selected);
    void clear();
    void removeIds(const QStringList& ids);

signals:
    void selectionChanged();

private:
    QString idAt(int proxyRow) const;
    QStringList visibleIds() const;
    void reconcile();
    void publishIfChanged(const QSet<QString>& previousIds);

    QAbstractProxyModel& proxyModel_;
    int idRole_;
    QSet<QString> selectedIds_;
};

#endif // JOBTRACKER_SRC_COMMON_BULKIDSELECTIONTRACKER_HPP
