#include "cvs/CvLockWrapper.hpp"

#include <QtTest/QtTest>

#include <chrono>
#include <future>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

class CvLockWrapperTest final : public QObject
{
    Q_OBJECT

private slots:
    void cancellationDoesNotReleaseAnotherOperationsLock();
};

void CvLockWrapperTest::cancellationDoesNotReleaseAnotherOperationsLock()
{
    CvLockWrapper lock;
    std::unique_lock owner{lock.getMutex()};
    const auto cancellation = std::make_shared<CancellationState>();
    std::promise<void> entered;
    auto entering = entered.get_future();
    std::promise<bool> completed;
    auto completion = completed.get_future();
    std::jthread waiter{[&] {
        entered.set_value();
        const std::unique_lock acquired{lock.getMutex()};
        completed.set_value(cancellation->isCancellationRequested());
    }};
    entering.wait();
    cancellation->requestCancellation();
    const auto stillBlocked = completion.wait_for(50ms) == std::future_status::timeout;
    owner.unlock();
    waiter.join();
    QVERIFY(stillBlocked);
    QVERIFY(completion.get());
}

QTEST_GUILESS_MAIN(CvLockWrapperTest)

#include "CvLockWrapperTest.moc"
