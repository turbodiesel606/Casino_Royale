#include "maintenance/StorageMutationGate.hpp"
#include "../support/LogCapture.hpp"
#include "logging/ApplicationLog.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>
#include <optional>

class StorageMutationGateTest final : public QObject
{
    Q_OBJECT

private slots:
    void mixedBatchesBlockRemovalUntilEveryReservationFinishes();
    void everyConflictingAttemptIsReportedWithoutQueueing();
    void observersSeeAdmissionFeedbackInOrder();
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

void StorageMutationGateTest::observersSeeAdmissionFeedbackInOrder()
{
    using Category = StorageMutationGate::OperationCategory;
    using Reason = StorageMutationGate::BlockReason;
    StorageMutationGate gate;
    testsupport::LogCapture capture;
    QVERIFY(capture.isValid());
    std::optional<ApplicationLogSession> session{std::in_place};
    QString events;
    connect(&gate, &StorageMutationGate::stateChanged, this, [&] { events += QLatin1Char('S'); });
    connect(&gate, &StorageMutationGate::feedbackChanged, this, [&] { events += QLatin1Char('F'); });
    connect(&gate, &StorageMutationGate::operationBlocked, this, [&] { events += QLatin1Char('B'); });
    QSignalSpy blocked{&gate, &StorageMutationGate::operationBlocked};

    QVERIFY(gate.reserveImportOrUpdate(0));
    gate.releaseImportOrUpdate(0);
    QVERIFY(events.isEmpty());
    QVERIFY(gate.reserveImportOrUpdate());
    QCOMPARE(events, QStringLiteral("SF"));
    events.clear();
    QVERIFY(!gate.checkJobDeletionAllowed());
    QVERIFY(!gate.checkActiveCvRemovalAllowed());
    QCOMPARE(events, QStringLiteral("FBFB"));
    QCOMPARE(blocked.last().at(0).value<Category>(), Category::Removal);
    QCOMPARE(blocked.last().at(1).value<Reason>(), Reason::ImportsOrUpdatesPending);
    events.clear();
    gate.releaseImportOrUpdate();
    QVERIFY(gate.retryNoticeVisible());
    QVERIFY(gate.beginRemoval());
    QCOMPARE(events, QStringLiteral("SFSF"));
    events.clear();
    QVERIFY(!gate.checkCvImportAllowed());
    QVERIFY(!gate.checkArchivedCvDeletionAllowed());
    QCOMPARE(events, QStringLiteral("FBFB"));
    QCOMPARE(blocked.at(2).at(0).value<Category>(), Category::ImportOrUpdate);
    QCOMPARE(blocked.at(2).at(1).value<Reason>(), Reason::RemovalActive);
    QCOMPARE(blocked.at(3).at(0).value<Category>(), Category::Removal);
    QCOMPARE(blocked.at(3).at(1).value<Reason>(), Reason::RemovalActive);
    events.clear();
    gate.beginShutdown();
    QVERIFY(!gate.checkCvImportAllowed());
    QVERIFY(!gate.beginRemoval());
    QVERIFY(gate.reserveImportOrUpdate(0));
    gate.beginShutdown();
    QCOMPARE(events, QStringLiteral("SF"));
    QCOMPARE(blocked.count(), 4);
    session.reset();
    const auto log = capture.contents();
    QCOMPARE(log.count("Declined "), 4);
    QVERIFY(log.contains("Declined job deletion because imports or updates was pending or active."));
    QVERIFY(log.contains("Declined active CV removal because imports or updates was pending or active."));
    QVERIFY(log.contains("Declined CV import because removal work was active."));
    QVERIFY(log.contains("Declined CV deletion because removal work was pending or active."));
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
