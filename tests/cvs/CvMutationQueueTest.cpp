#include "cvs/CvMutationQueue.hpp"

#include <QtTest/QtTest>

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

class CvMutationQueueTest final : public QObject
{
    Q_OBJECT

private slots:
    void leaseMovesAndReleasesOnDestruction();
    void waitersAcquireInFifoOrder();
    void cancelledWaiterLeavesBeforeActiveLeaseIsReleased();
};

void CvMutationQueueTest::leaseMovesAndReleasesOnDestruction()
{
    CvMutationQueue queue;
    const auto cancellation = std::make_shared<CancellationState>();
    {
        auto first = queue.acquire(cancellation);
        QVERIFY(first);
        auto second = std::move(first);
        QVERIFY(!first);
        QVERIFY(second);
        CvMutationQueue::Lease third;
        third = std::move(second);
        QVERIFY(!second);
        QVERIFY(third);
    }
    auto next = queue.acquire(cancellation);
    QVERIFY(next);
    cancellation->requestCancellation();
    auto cancelled = queue.acquire(cancellation);
    QVERIFY(!cancelled);
}

void CvMutationQueueTest::waitersAcquireInFifoOrder()
{
    CvMutationQueue queue;
    auto owner = queue.acquire({});
    std::vector<int> order;
    std::mutex orderMutex;
    std::vector<std::jthread> waiters;
    bool allBlocked = true;
    for (int index = 0; index < 3; ++index) {
        std::promise<void> entered;
        auto entering = entered.get_future();
        std::promise<void> finished;
        auto finishing = finished.get_future();
        waiters.emplace_back(
            [&, index, entered = std::move(entered), finished = std::move(finished)]
            (std::stop_token stop) mutable {
                const auto cancellation = std::make_shared<CancellationState>();
                const std::stop_callback cancelOnExit{stop, [&] { cancellation->requestCancellation(); }};
                entered.set_value();
                auto lease = queue.acquire(cancellation);
                if (lease) {
                    const std::scoped_lock lock{orderMutex};
                    order.push_back(index);
                }
                finished.set_value();
            });
        // Start each contender separately while the owner holds the queue.
        entering.wait();
        allBlocked = (finishing.wait_for(50ms) == std::future_status::timeout) && allBlocked;
    }
    owner = {};
    for (auto& waiter : waiters) {
        waiter.join();
    }
    QVERIFY(allBlocked);
    QCOMPARE(order, std::vector<int>({0, 1, 2}));
}

void CvMutationQueueTest::cancelledWaiterLeavesBeforeActiveLeaseIsReleased()
{
    CvMutationQueue queue;
    auto owner = queue.acquire({});
    const auto cancellation = std::make_shared<CancellationState>();
    std::promise<void> entered;
    auto entering = entered.get_future();
    std::promise<bool> completed;
    auto completion = completed.get_future();
    std::jthread waiter{[&](std::stop_token stop) {
        const std::stop_callback cancelOnExit{stop, [&] { cancellation->requestCancellation(); }};
        entered.set_value();
        auto lease = queue.acquire(cancellation);
        completed.set_value(static_cast<bool>(lease));
    }};
    entering.wait();
    const auto initiallyBlocked = completion.wait_for(50ms) == std::future_status::timeout;
    cancellation->requestCancellation();
    const auto cancelledWhileHeld = completion.wait_for(2s) == std::future_status::ready;
    owner = {};
    waiter.join();
    QVERIFY(initiallyBlocked);
    QVERIFY(cancelledWhileHeld);
    QVERIFY(!completion.get());
    auto next = queue.acquire({});
    QVERIFY(next);
}

QTEST_GUILESS_MAIN(CvMutationQueueTest)

#include "CvMutationQueueTest.moc"
