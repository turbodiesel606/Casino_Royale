#include "maintenance/StorageMutationGate.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

class StorageMutationGateTest final : public QObject
{
    Q_OBJECT

private slots:
    void mixedBatchesBlockRemovalUntilEveryReservationFinishes();
    void everyConflictingAttemptIsReportedWithoutQueueing();
    void noticeFollowsAvailabilityUntilDismissedOrAdmitted();
    void shutdownAndUnobservedWorkNeverAnnounceAvailability();
};

void StorageMutationGateTest::mixedBatchesBlockRemovalUntilEveryReservationFinishes()
{
    StorageMutationGate gate;
    QSignalSpy blocked{&gate, &StorageMutationGate::operationBlocked};
    QVERIFY(gate.reserveImportOrUpdate(1000));
    QVERIFY(gate.reserveImportOrUpdate(1000));
    QVERIFY(!gate.beginRemoval());
    QCOMPARE(blocked.count(), 1);
    QCOMPARE(blocked.first().at(0).value<StorageMutationGate::OperationCategory>(),
        StorageMutationGate::OperationCategory::Removal);
    QCOMPARE(blocked.first().at(1).value<StorageMutationGate::BlockReason>(),
        StorageMutationGate::BlockReason::ImportsOrUpdatesPending);
    for (int index = 0; index < 1999; ++index) {
        gate.releaseImportOrUpdate();
        QVERIFY(!gate.canBeginRemoval());
        QVERIFY(!gate.retryNoticeVisible());
    }
    gate.releaseImportOrUpdate();
    QVERIFY(gate.canBeginRemoval());
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.retryNoticeMessage().contains(QStringLiteral("finished")));
    QVERIFY(gate.blockedTitle().contains(QStringLiteral("finished")));
}

void StorageMutationGateTest::everyConflictingAttemptIsReportedWithoutQueueing()
{
    StorageMutationGate gate;
    QSignalSpy blocked{&gate, &StorageMutationGate::operationBlocked};
    QVERIFY(gate.beginRemoval());
    for (int attempt = 0; attempt < 3; ++attempt) {
        QVERIFY(!gate.reserveImportOrUpdate(1000));
        QVERIFY(!gate.beginRemoval());
    }
    QCOMPARE(blocked.count(), 6);
    for (const auto& event : blocked) {
        QCOMPARE(event.at(1).value<StorageMutationGate::BlockReason>(),
            StorageMutationGate::BlockReason::RemovalActive);
    }
    QVERIFY(!gate.retryNoticeVisible());
    gate.endRemoval();
    QVERIFY(gate.canBeginImportOrUpdate());
    QVERIFY(gate.canBeginRemoval());
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.retryNoticeMessage().contains(QStringLiteral("importing")));
    QVERIFY(gate.retryNoticeMessage().contains(QStringLiteral("deleting")));
    QVERIFY(!gate.removalActive());
}

void StorageMutationGateTest::noticeFollowsAvailabilityUntilDismissedOrAdmitted()
{
    StorageMutationGate gate;
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(!gate.checkRemovalAllowed());
    gate.releaseImportOrUpdate();
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.checkRemovalAllowed());
    QVERIFY(gate.retryNoticeVisible()); // A confirmation check reserves nothing.
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.retryNoticeMessage().contains(QStringLiteral("Wait")));
    QVERIFY(!gate.beginRemoval());
    gate.releaseImportOrUpdate();
    QVERIFY(gate.beginRemoval());
    QVERIFY(!gate.retryNoticeVisible());
    QVERIFY(!gate.checkImportOrUpdateAllowed());
    gate.endRemoval();
    QVERIFY(gate.retryNoticeVisible());
    gate.dismissRetryNotice();
    QVERIFY(!gate.retryNoticeVisible());
    QVERIFY(gate.beginRemoval());
    gate.endRemoval();
    QVERIFY(!gate.retryNoticeVisible());
    QVERIFY(gate.beginRemoval());
    QVERIFY(!gate.checkImportOrUpdateAllowed());
    gate.endRemoval();
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(!gate.retryNoticeVisible());
    gate.releaseImportOrUpdate();
}

void StorageMutationGateTest::shutdownAndUnobservedWorkNeverAnnounceAvailability()
{
    StorageMutationGate gate;
    QVERIFY(gate.reserveImportOrUpdate());
    gate.releaseImportOrUpdate();
    QVERIFY(gate.beginRemoval());
    gate.endRemoval();
    QVERIFY(!gate.retryNoticeVisible());
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(!gate.beginRemoval());
    QSignalSpy blocked{&gate, &StorageMutationGate::operationBlocked};
    gate.beginShutdown();
    gate.beginShutdown();
    gate.releaseImportOrUpdate();
    QVERIFY(gate.shuttingDown());
    QVERIFY(!gate.canBeginRemoval());
    QVERIFY(!gate.canBeginImportOrUpdate());
    QVERIFY(!gate.checkRemovalAllowed());
    QVERIFY(!gate.reserveImportOrUpdate());
    QVERIFY(!gate.retryNoticeVisible());
    QCOMPARE(blocked.count(), 0);
}

QTEST_GUILESS_MAIN(StorageMutationGateTest)

#include "StorageMutationGateTest.moc"
