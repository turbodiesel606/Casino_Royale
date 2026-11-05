#ifndef JOBTRACKER_TESTS_STORAGE_DATAREMOVALSERVICETEST_HPP
#define JOBTRACKER_TESTS_STORAGE_DATAREMOVALSERVICETEST_HPP

#include "../support/StorageTestFixtures.hpp"

#include "common/CancellationState.hpp"
#include "maintenance/DataRemovalService.hpp"
#include "maintenance/DataRemovalWorker.hpp"

#include <QMetaType>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QVariant>
#include <QVector>
#include <QtTest/QtTest>

#include <concepts>
#include <memory>

namespace {

template<DataRemovalKind Kind>
concept HasTypedRemovalApi = requires(
    const DataRemovalService& service,
    DataRemovalExecutor& executor,
    DataRemovalWorker& worker,
    const QVector<DataRemovalItemRequest>& items,
    const std::shared_ptr<CancellationState>& cancellation)
{
    typename DataRemovalRequest<Kind>;
    typename DataRemovalBatchOutcome<Kind>;
    requires DataRemovalRequest<Kind>::kind_ == Kind;
    requires DataRemovalBatchOutcome<Kind>::kind_ == Kind;
    requires SingleActiveWorkerRequestFor<DataRemovalRequest<Kind>, DataRemovalExecutor>;
    { service.process<Kind>(items, cancellation) } -> std::same_as<DataRemovalBatchResult>;
    { executor.process(DataRemovalRequest<Kind>{}) } -> std::same_as<DataRemovalBatchOutcome<Kind>>;
    { worker.submit(DataRemovalRequest<Kind>{}) } -> std::same_as<void>;
};

QVector<DataRemovalItemRequest> requestFor(const QString& id, const QString& label = {})
{
    return {{id, label.isEmpty() ? id : label}};
}

template<DataRemovalKind Kind>
void verifyCorrelation(
    const QVariant& value,
    quint64 operationId,
    const std::shared_ptr<CancellationState>& cancellation)
{
    using Outcome = DataRemovalBatchOutcome<Kind>;
    QCOMPARE(value.metaType(), QMetaType::fromType<Outcome>());
    const auto outcome = qvariant_cast<Outcome>(value);
    QCOMPARE(outcome.operationId_, operationId);
    QCOMPARE(outcome.cancellation_, cancellation);
}

template<DataRemovalKind Kind, typename Signal>
void verifyWorkerBoundaryOutcomes(Signal signal)
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto initialConnections = QSqlDatabase::connectionNames().size();
    DataRemovalWorker worker{storage.paths().dataDirectory()};
    QSignalSpy completed{&worker, signal};
    QVERIFY(completed.isValid());
    using Outcome = DataRemovalBatchOutcome<Kind>;

    const auto emptyToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{1, {}, emptyToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    verifyCorrelation<Kind>(completed.at(0).at(0), 1, emptyToken);
    const auto empty = qvariant_cast<Outcome>(completed.at(0).at(0));
    QVERIFY(empty.result_.items_.isEmpty());
    QVERIFY(!empty.result_.cancelled_);

    const auto cancelledToken = std::make_shared<CancellationState>();
    cancelledToken->requestCancellation();
    worker.submit(DataRemovalRequest<Kind>{2, requestFor(QStringLiteral("missing")), cancelledToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    verifyCorrelation<Kind>(completed.at(1).at(0), 2, cancelledToken);
    const auto cancelled = qvariant_cast<Outcome>(completed.at(1).at(0));
    QVERIFY(cancelled.result_.cancelled_);
    QVERIFY(cancelled.result_.items_.isEmpty());

    const auto failedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{3, requestFor(QStringLiteral("missing")), failedToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 3, 10000);
    verifyCorrelation<Kind>(completed.at(2).at(0), 3, failedToken);
    const auto failed = qvariant_cast<Outcome>(completed.at(2).at(0));
    QCOMPARE(failed.result_.items_.size(), 1);
    QCOMPARE(failed.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(!failed.result_.items_.first().message_.isEmpty());

    worker.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnections);
    const auto rejectedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{4, requestFor(QStringLiteral("rejected")), rejectedToken});
    QCOMPARE(completed.count(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 4, 10000);
    verifyCorrelation<Kind>(completed.at(3).at(0), 4, rejectedToken);
    const auto rejected = qvariant_cast<Outcome>(completed.at(3).at(0));
    QCOMPARE(rejected.result_.items_.size(), 1);
    QCOMPARE(rejected.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QCOMPARE(rejected.result_.items_.first().message_, QStringLiteral("The deletion worker is shutting down."));

    // A file cannot serve as the worker's data directory: context creation fails.
    const auto invalidDirectory = storage.createFile(QStringLiteral("not-a-directory.pdf"));
    QVERIFY(!invalidDirectory.isEmpty());
    DataRemovalWorker unavailable{invalidDirectory};
    QSignalSpy unavailableSpy{&unavailable, signal};
    const auto unavailableToken = std::make_shared<CancellationState>();
    unavailable.submit(DataRemovalRequest<Kind>{5, requestFor(QStringLiteral("unavailable")), unavailableToken});
    QTRY_COMPARE_WITH_TIMEOUT(unavailableSpy.count(), 1, 10000);
    verifyCorrelation<Kind>(unavailableSpy.first().at(0), 5, unavailableToken);
    const auto unavailableOutcome = qvariant_cast<Outcome>(unavailableSpy.first().at(0));
    QCOMPARE(unavailableOutcome.result_.items_.size(), 1);
    QCOMPARE(unavailableOutcome.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(!unavailableOutcome.result_.items_.first().message_.isEmpty());
    unavailable.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnections);
}

}

#endif // JOBTRACKER_TESTS_STORAGE_DATAREMOVALSERVICETEST_HPP
