#include "common/RoleFilterProxyModel.hpp"
#include "common/BulkIdSelectionTracker.hpp"
#include "common/StableIdSelectionTracker.hpp"

#include "../support/MutableRecordModel.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

class StableIdSelectionTrackerTest final : public QObject
{
    Q_OBJECT

private slots:
    void fallsBackAcrossFiltersAndEmptyResults();
    void reportsSourceMutationsPrecisely();
    void visibleCountChangesAreCoalescedWithSelection();
    void bulkSelectionUsesVisibleStableIdsAndSurvivesSorting();
};

void StableIdSelectionTrackerTest::fallsBackAcrossFiltersAndEmptyResults()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSearchRoles({testsupport::MutableRecordModel::NameRole});
    proxy.setSourceModel(&model);
    StableIdSelectionTracker tracker{proxy, testsupport::MutableRecordModel::IdRole};
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
    proxy.setSearchText({});
    QCOMPARE(tracker.selectedId(), QStringLiteral("a"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(selectionSpy.count(), 1);
}

void StableIdSelectionTrackerTest::reportsSourceMutationsPrecisely()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    StableIdSelectionTracker tracker{proxy, testsupport::MutableRecordModel::IdRole};
    QSignalSpy selectionSpy{&tracker, &StableIdSelectionTracker::selectionChanged};

    proxy.setSort(testsupport::MutableRecordModel::ScoreRole, Qt::DescendingOrder);
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

void StableIdSelectionTrackerTest::visibleCountChangesAreCoalescedWithSelection()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSearchRoles({testsupport::MutableRecordModel::NameRole});
    proxy.setSourceModel(&model);
    StableIdSelectionTracker tracker{proxy, testsupport::MutableRecordModel::IdRole};
    QCOMPARE(tracker.visibleRowCount(), 3);

    QSignalSpy selectionSpy{&tracker, &StableIdSelectionTracker::selectionChanged};
    QSignalSpy countSpy{&tracker, &StableIdSelectionTracker::visibleRowCountChanged};
    tracker.beginModelUpdate();
    tracker.beginModelUpdate();
    proxy.setSearchText(QStringLiteral("Gamma"));

    QCOMPARE(selectionSpy.count(), 0);
    QCOMPARE(countSpy.count(), 0);
    QCOMPARE(tracker.visibleRowCount(), 3);

    tracker.endModelUpdate();
    QCOMPARE(selectionSpy.count(), 0);
    QCOMPARE(countSpy.count(), 0);

    tracker.endModelUpdate();
    QCOMPARE(tracker.selectedId(), QStringLiteral("c"));
    QCOMPARE(tracker.selectedRow(), 0);
    QCOMPARE(tracker.visibleRowCount(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);

    tracker.synchronize();
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
}

void StableIdSelectionTrackerTest::bulkSelectionUsesVisibleStableIdsAndSurvivesSorting()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Design"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Phone"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSearchRoles({testsupport::MutableRecordModel::NameRole});
    proxy.setSourceModel(&model);
    BulkIdSelectionTracker tracker{proxy, testsupport::MutableRecordModel::IdRole};
    QSignalSpy changedSpy{&tracker, &BulkIdSelectionTracker::selectionChanged};

    tracker.toggleRow(0);
    QVERIFY(tracker.contains(QStringLiteral("a")));
    QVERIFY(tracker.someVisibleSelected());
    QVERIFY(!tracker.allVisibleSelected());

    proxy.setSort(testsupport::MutableRecordModel::ScoreRole, Qt::DescendingOrder);
    QVERIFY(tracker.contains(QStringLiteral("a")));
    QCOMPARE(tracker.selectedCount(), 1);

    tracker.setAllVisibleSelected(true);
    QCOMPARE(tracker.selectedCount(), 3);
    QVERIFY(tracker.allVisibleSelected());

    proxy.setSearchText(QStringLiteral("Gamma"));
    QCOMPARE(tracker.selectedIds(), QStringList{QStringLiteral("c")});
    QCOMPARE(tracker.selectedCount(), 1);
    QVERIFY(tracker.allVisibleSelected());

    tracker.removeIds({QStringLiteral("c")});
    QCOMPARE(tracker.selectedCount(), 0);
    QVERIFY(!tracker.someVisibleSelected());
    QVERIFY(changedSpy.count() >= 3);
}

QTEST_GUILESS_MAIN(StableIdSelectionTrackerTest)

#include "StableIdSelectionTrackerTest.moc"
