#include "common/RoleFilterProxyModel.hpp"
#include "common/StableIdSelectionTracker.hpp"
#include "common/ValidationService.hpp"

#include <QAbstractListModel>
#include <QSignalSpy>
#include <QtTest/QtTest>

namespace {

class SimpleListModel final : public QAbstractListModel
{
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        ChannelRole,
        ScoreRole,
    };

    explicit SimpleListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
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
        };
    }

    void addRow(const QString& name, const QString& category, const QString& channel, int score)
    {
        addRow(name.toCaseFolded().replace(QLatin1Char(' '), QLatin1Char('-')), name, category, channel, score);
    }

    void addRow(
        const QString& id,
        const QString& name,
        const QString& category,
        const QString& channel,
        int score)
    {
        const auto row = rows_.size();
        beginInsertRows(QModelIndex(), row, row);
        rows_.append({
            {IdRole, id},
            {NameRole, name},
            {CategoryRole, category},
            {ChannelRole, channel},
            {ScoreRole, score},
        });
        endInsertRows();
    }

    void insertRowData(int row, const QString& id, const QString& name, int score)
    {
        beginInsertRows({}, row, row);
        rows_.insert(row, {
            {IdRole, id},
            {NameRole, name},
            {CategoryRole, QStringLiteral("Engineering")},
            {ChannelRole, QStringLiteral("Email")},
            {ScoreRole, score},
        });
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
            rows_.append({
                {IdRole, ids.at(row)},
                {NameRole, ids.at(row).toUpper()},
                {CategoryRole, QStringLiteral("Engineering")},
                {ChannelRole, QStringLiteral("Email")},
                {ScoreRole, row},
            });
        }
        endResetModel();
    }

    void updateName(int row, const QString& name)
    {
        rows_[row].insert(NameRole, name);
        const auto changedIndex = index(row, 0);
        emit dataChanged(changedIndex, changedIndex, {NameRole});
    }

private:
    QVector<QHash<int, QVariant>> rows_;
};

} // namespace

class FilterAndValidationTest final : public QObject
{
    Q_OBJECT

private slots:
    void proxyFiltersSearchTextAcrossConfiguredRoles();
    void proxyCombinesExactAndRequiredRoleFilters();
    void proxySortsStringAndNumericRoles();
    void stableSelectionFallsBackAcrossFiltersAndEmptyResults();
    void stableSelectionReportsEveryMutationPrecisely();
    void validationReportsRequiredAndUrlErrors();
};

void FilterAndValidationTest::proxyFiltersSearchTextAcrossConfiguredRoles()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setSearchRoles({SimpleListModel::NameRole, SimpleListModel::CategoryRole});

    proxy.setSearchText(QStringLiteral("qml engineering"));

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::NameRole).toString(), QStringLiteral("QML Engineer"));
}

void FilterAndValidationTest::proxyCombinesExactAndRequiredRoleFilters()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setExactFilter(SimpleListModel::CategoryRole, QStringLiteral("Engineering"));
    proxy.setRequiredNonEmptyRole(SimpleListModel::ChannelRole);

    QCOMPARE(proxy.rowCount(), 2);

    proxy.setExactFilter(SimpleListModel::CategoryRole, QStringLiteral("Design"));
    QCOMPARE(proxy.rowCount(), 0);
}

void FilterAndValidationTest::proxySortsStringAndNumericRoles()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    proxy.setSort(SimpleListModel::NameRole);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::NameRole).toString(), QStringLiteral("Product Designer"));

    proxy.setSort(SimpleListModel::ScoreRole, Qt::DescendingOrder);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::ScoreRole).toInt(), 3);
}

void FilterAndValidationTest::stableSelectionFallsBackAcrossFiltersAndEmptyResults()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSearchRoles({SimpleListModel::NameRole});
    proxy.setSourceModel(&model);
    StableIdSelectionTracker tracker{proxy, SimpleListModel::IdRole};
    tracker.selectRow(1);
    QSignalSpy selectionSpy{&tracker, &StableIdSelectionTracker::selectionChanged};

    proxy.setSearchText(QStringLiteral("Gamma"));
    QCOMPARE(tracker.selectedId(), QStringLiteral("c"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), true);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), false);

    selectionSpy.clear();
    proxy.setSearchText(QStringLiteral("does-not-match"));
    QVERIFY(tracker.selectedId().isEmpty());
    QCOMPARE(tracker.selectedRow(), -1);
    QCOMPARE(selectionSpy.count(), 1);

    selectionSpy.clear();
    proxy.setSearchText(QString());
    QCOMPARE(tracker.selectedId(), QStringLiteral("a"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(selectionSpy.count(), 1);
}

void FilterAndValidationTest::stableSelectionReportsEveryMutationPrecisely()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    StableIdSelectionTracker tracker{proxy, SimpleListModel::IdRole};
    QSignalSpy selectionSpy{&tracker, &StableIdSelectionTracker::selectionChanged};

    proxy.setSort(SimpleListModel::ScoreRole, Qt::DescendingOrder);
    QCOMPARE(tracker.selectedId(), QStringLiteral("a"));
    QCOMPARE(tracker.selectedRow(), 2);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), false);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), false);

    proxy.sort(-1);
    tracker.selectRow(1);
    selectionSpy.clear();

    model.insertRowData(0, QStringLiteral("x"), QStringLiteral("Aardvark"), 0);
    QCOMPARE(tracker.selectedId(), QStringLiteral("b"));
    QCOMPARE(tracker.selectedRow(), 2);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), false);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), false);

    selectionSpy.clear();
    model.updateName(2, QStringLiteral("Beta Updated"));
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), false);
    QCOMPARE(selectionSpy.first().at(1).toBool(), false);
    QCOMPARE(selectionSpy.first().at(2).toBool(), true);

    selectionSpy.clear();
    model.moveRowData(2, 0);
    QCOMPARE(tracker.selectedId(), QStringLiteral("b"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), false);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), false);

    selectionSpy.clear();
    model.resetRows({QStringLiteral("c"), QStringLiteral("b"), QStringLiteral("a")});
    QCOMPARE(tracker.selectedId(), QStringLiteral("b"));
    QCOMPARE(tracker.selectedRow(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), false);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), true);

    selectionSpy.clear();
    model.removeRowData(1);
    QCOMPARE(tracker.selectedId(), QStringLiteral("c"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(selectionSpy.first().at(0).toBool(), true);
    QCOMPARE(selectionSpy.first().at(1).toBool(), true);
    QCOMPARE(selectionSpy.first().at(2).toBool(), false);
}

void FilterAndValidationTest::validationReportsRequiredAndUrlErrors()
{
    const auto required = ValidationService::validateRequiredFields(
        {{QStringLiteral("title"), QStringLiteral("Qt Developer")}, {QStringLiteral("company"), QString()}},
        {QStringLiteral("title"), QStringLiteral("company")});
    QVERIFY(!required.isValid_);
    QCOMPARE(required.messages_, QStringList({QStringLiteral("company is required.")}));

    const auto validUrl = ValidationService::validateHttpUrl(QStringLiteral("jobUrl"), QStringLiteral("https://example.com/job"), true);
    QVERIFY(validUrl.isValid_);
    QVERIFY(validUrl.messages_.isEmpty());

    const auto invalidUrl = ValidationService::validateHttpUrl(QStringLiteral("jobUrl"), QStringLiteral("ftp://example.com/job"), true);
    QVERIFY(!invalidUrl.isValid_);
    QCOMPARE(invalidUrl.messages_, QStringList({QStringLiteral("jobUrl must be a valid HTTP or HTTPS URL.")}));
}

QTEST_APPLESS_MAIN(FilterAndValidationTest)

#include "FilterAndValidationTest.moc"
