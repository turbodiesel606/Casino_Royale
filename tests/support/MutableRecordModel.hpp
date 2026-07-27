#ifndef JOBTRACKER_TESTS_SUPPORT_MUTABLERECORDMODEL_HPP
#define JOBTRACKER_TESTS_SUPPORT_MUTABLERECORDMODEL_HPP

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QStringList>
#include <QVector>

namespace testsupport {

class MutableRecordModel final : public QAbstractListModel
{
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        ChannelRole,
        ScoreRole,
        TimestampRole,
    };

    explicit MutableRecordModel(QObject* parent = nullptr)
        : QAbstractListModel{parent}
    {
    }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : rows_.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
            return {};
        }

        return rows_.at(index.row()).value(role);
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {IdRole, "id"},
            {NameRole, "name"},
            {CategoryRole, "category"},
            {ChannelRole, "channel"},
            {ScoreRole, "score"},
            {TimestampRole, "timestamp"},
        };
    }

    void addRow(const QString& name, const QString& category, const QString& channel, int score)
    {
        addRow(
            QString{name}.toCaseFolded().replace(QLatin1Char{' '}, QLatin1Char{'-'}),
            name,
            category,
            channel,
            score);
    }

    void addRow(
        const QString& id,
        const QString& name,
        const QString& category,
        const QString& channel,
        int score)
    {
        const auto row = rows_.size();
        beginInsertRows({}, row, row);
        rows_.append(makeRow(id, name, category, channel, score, score));
        endInsertRows();
    }

    void insertRowData(int row, const QString& id, const QString& name, int score)
    {
        beginInsertRows({}, row, row);
        rows_.insert(
            row,
            makeRow(
                id,
                name,
                QStringLiteral("Engineering"),
                QStringLiteral("Email"),
                score,
                score + 1));
        endInsertRows();
    }

    void removeRowData(int row)
    {
        beginRemoveRows({}, row, row);
        rows_.removeAt(row);
        endRemoveRows();
    }

    void moveRowData(int sourceRow, int destinationRow)
    {
        const auto destinationChild = destinationRow > sourceRow ? destinationRow + 1 : destinationRow;
        beginMoveRows({}, sourceRow, sourceRow, {}, destinationChild);
        rows_.move(sourceRow, destinationRow);
        endMoveRows();
    }

    void resetRows(const QStringList& ids)
    {
        beginResetModel();
        rows_.clear();
        for (int row = 0; row < ids.size(); ++row) {
            rows_.append(makeRow(
                ids.at(row),
                ids.at(row).toUpper(),
                QStringLiteral("Engineering"),
                QStringLiteral("Email"),
                row,
                row + 1));
        }
        endResetModel();
    }

    void updateName(int row, const QString& name)
    {
        rows_[row].insert(NameRole, name);
        const auto changedIndex = index(row, 0);
        emit dataChanged(changedIndex, changedIndex, {NameRole});
    }

    void updateCategory(int row, const QString& category)
    {
        rows_[row].insert(CategoryRole, category);
        const auto changedIndex = index(row, 0);
        emit dataChanged(changedIndex, changedIndex, {CategoryRole});
    }

    void updateTimestamp(int row, const QDateTime& timestamp)
    {
        rows_[row].insert(TimestampRole, timestamp);
        const auto changedIndex = index(row, 0);
        emit dataChanged(changedIndex, changedIndex, {TimestampRole});
    }

private:
    static QHash<int, QVariant> makeRow(
        const QString& id,
        const QString& name,
        const QString& category,
        const QString& channel,
        int score,
        int timestampDay)
    {
        return {
            {IdRole, id},
            {NameRole, name},
            {CategoryRole, category},
            {ChannelRole, channel},
            {ScoreRole, score},
            {TimestampRole, QDateTime{QDate{2026, 1, timestampDay}, QTime{0, 0}, Qt::UTC}},
        };
    }

    QVector<QHash<int, QVariant>> rows_;
};

} // namespace testsupport

#endif // JOBTRACKER_TESTS_SUPPORT_MUTABLERECORDMODEL_HPP
