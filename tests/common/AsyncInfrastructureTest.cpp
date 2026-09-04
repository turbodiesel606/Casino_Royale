#include "common/CancellationState.hpp"
#include "common/SerialOperationQueue.hpp"
#include "common/SingleActiveWorkerRuntime.hpp"

#include <QObject>
#include <QThread>
#include <QtTest/QtTest>

#include <atomic>
#include <memory>
#include <stdexcept>

namespace {

class TestExecutor final : public QObject
{
public:
    TestExecutor(
        std::atomic_int* destructorCount = nullptr,
        std::atomic_bool* destroyedOnOwnerThread = nullptr)
        : destructorCount_{destructorCount}
        , destroyedOnOwnerThread_{destroyedOnOwnerThread}
    {
    }

    ~TestExecutor() override
    {
        if (destructorCount_ != nullptr) {
            ++*destructorCount_;
        }
        if (destroyedOnOwnerThread_ != nullptr) {
            *destroyedOnOwnerThread_ = QThread::currentThread() == thread();
        }
    }

private:
    std::atomic_int* destructorCount_ = nullptr;
    std::atomic_bool* destroyedOnOwnerThread_ = nullptr;
};

} // namespace

class AsyncInfrastructureTest final : public QObject
{
    Q_OBJECT

private slots:
    void serialQueueOwnsFifoCorrelationAndCancellation();
    void workerRuntimeOwnsLifecycleAndRejectsStaleOutcomes();
    void workerRuntimeLeavesNoBusyStateAfterInitializationFailure();
};

void AsyncInfrastructureTest::serialQueueOwnsFifoCorrelationAndCancellation()
{
    SerialOperationQueue<QString> queue;

    QCOMPARE(queue.enqueue(QStringLiteral("first")), quint64{1});
    QCOMPARE(queue.enqueue(QStringLiteral("second")), quint64{2});
    QCOMPARE(queue.pendingCount(), 2);
    QVERIFY(queue.activateNext());
    QVERIFY(!queue.activateNext());

    const auto* active = queue.active();
    QVERIFY(active != nullptr);
    QCOMPARE(active->operationId_, quint64{1});
    QCOMPARE(active->payload_, QStringLiteral("first"));
    const auto firstCancellation = queue.activeCancellation();
    QVERIFY(queue.matches(1, firstCancellation));
    QVERIFY(!queue.matches(2, firstCancellation));

    QVERIFY(queue.requestActiveCancellation(false));
    QVERIFY(firstCancellation->isCancellationRequested());
    QVERIFY(!queue.completionSuppressed());
    queue.finishActive();

    QVERIFY(queue.activateNext());
    QCOMPARE(queue.active()->operationId_, quint64{2});
    QCOMPARE(queue.active()->payload_, QStringLiteral("second"));
    QVERIFY(queue.requestActiveCancellation());
    QVERIFY(queue.completionSuppressed());

    QCOMPARE(queue.enqueue(QStringLiteral("third")), quint64{3});
    QCOMPARE(queue.enqueue(QStringLiteral("fourth")), quint64{4});
    QCOMPARE(queue.clearWaiting(), 2);
    QCOMPARE(queue.pendingCount(), 1);
    queue.finishActive();
    QVERIFY(queue.isDrained());

    QCOMPARE(queue.enqueue(QStringLiteral("fifth")), quint64{5});
}

void AsyncInfrastructureTest::workerRuntimeOwnsLifecycleAndRejectsStaleOutcomes()
{
    std::atomic_int createCount = 0;
    std::atomic_int destroyCount = 0;
    std::atomic_int destructorCount = 0;
    std::atomic_bool contextDestroyedOnOwnerThread = false;
    std::atomic_bool executorDestroyedOnOwnerThread = false;
    SingleActiveWorkerRuntime runtime{
        QStringLiteral("AsyncInfrastructureTestThread"),
        {
            [&createCount, &destructorCount, &executorDestroyedOnOwnerThread]() -> QObject* {
                ++createCount;
                return new TestExecutor{
                    &destructorCount,
                    &executorDestroyedOnOwnerThread};
            },
            [&destroyCount, &contextDestroyedOnOwnerThread](QObject& executor) {
                ++destroyCount;
                contextDestroyedOnOwnerThread = QThread::currentThread() == executor.thread();
            },
        }};

    const auto firstCancellation = std::make_shared<CancellationState>();
    QCOMPARE(
        runtime.tryBeginOperation(7, firstCancellation),
        SingleActiveWorkerAdmission::Accepted);
    QCOMPARE(createCount.load(), 1);
    QVERIFY(runtime.isBusy());
    QVERIFY(runtime.isRunning());
    QVERIFY(runtime.isActiveOutcome(7, firstCancellation));
    QVERIFY(!runtime.isActiveOutcome(8, firstCancellation));
    QVERIFY(!runtime.isActiveOutcome(7, std::make_shared<CancellationState>()));
    QCOMPARE(
        runtime.tryBeginOperation(8, std::make_shared<CancellationState>()),
        SingleActiveWorkerAdmission::Busy);

    QVERIFY(runtime.requestActiveCancellation());
    QVERIFY(firstCancellation->isCancellationRequested());
    runtime.completeOperation();
    QVERIFY(!runtime.isBusy());
    QVERIFY(!runtime.isActiveOutcome(7, firstCancellation));

    const auto secondCancellation = std::make_shared<CancellationState>();
    QCOMPARE(
        runtime.tryBeginOperation(8, secondCancellation),
        SingleActiveWorkerAdmission::Accepted);
    QCOMPARE(createCount.load(), 1);
    runtime.shutdown();
    QVERIFY(secondCancellation->isCancellationRequested());
    QVERIFY(!runtime.isBusy());
    QVERIFY(!runtime.isRunning());
    QCOMPARE(destroyCount.load(), 1);
    QCOMPARE(destructorCount.load(), 1);
    QVERIFY(contextDestroyedOnOwnerThread.load());
    QVERIFY(executorDestroyedOnOwnerThread.load());
    QCOMPARE(
        runtime.tryBeginOperation(9, std::make_shared<CancellationState>()),
        SingleActiveWorkerAdmission::ShuttingDown);

    runtime.shutdown();
    QCOMPARE(destroyCount.load(), 1);
    QCOMPARE(destructorCount.load(), 1);
}

void AsyncInfrastructureTest::workerRuntimeLeavesNoBusyStateAfterInitializationFailure()
{
    SingleActiveWorkerRuntime runtime{
        QStringLiteral("FailingAsyncInfrastructureTestThread"),
        {
            []() -> QObject* {
                throw std::runtime_error("expected executor initialization failure");
            },
            {},
        }};

    QVERIFY_EXCEPTION_THROWN(
        runtime.tryBeginOperation(1, std::make_shared<CancellationState>()),
        std::runtime_error);
    QVERIFY(!runtime.isBusy());
    QVERIFY(!runtime.isRunning());
    runtime.shutdown();
}

QTEST_GUILESS_MAIN(AsyncInfrastructureTest)

#include "AsyncInfrastructureTest.moc"
