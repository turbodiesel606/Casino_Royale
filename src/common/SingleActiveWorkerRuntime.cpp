#include "SingleActiveWorkerRuntime.hpp"

#include <QMetaObject>
#include <QThread>

#include <stdexcept>
#include <utility>

SingleActiveWorkerRuntime::SingleActiveWorkerRuntime(
    QString threadName,
    SingleActiveWorkerExecutorLifecycle executorLifecycle)
    : executorLifecycle_{std::move(executorLifecycle)}
    , facadeThread_{QThread::currentThread()}
{
    workerThread_.setObjectName(std::move(threadName));
}

SingleActiveWorkerRuntime::~SingleActiveWorkerRuntime()
{
    shutdown();
}

SingleActiveWorkerAdmission SingleActiveWorkerRuntime::tryBeginOperation(
    quint64 operationId,
    std::shared_ptr<CancellationState> cancellation)
{
    assertFacadeThread();

    if (shuttingDown_) 
        return SingleActiveWorkerAdmission::ShuttingDown;
    
    if (busy_) 
        return SingleActiveWorkerAdmission::Busy;
    
    // Creation and thread startup happen before active correlation is visible.
    // If either factory validation below throws, the runtime remains idle.
    ensureExecutor();
    activeOperationId_ = operationId;
    activeCancellation_ = std::move(cancellation);
    busy_ = true;
    return SingleActiveWorkerAdmission::Accepted;
}

QObject& SingleActiveWorkerRuntime::executor() const
{
    assertFacadeThread();
    Q_ASSERT(executor_ != nullptr);
    return *executor_;
}

bool SingleActiveWorkerRuntime::isActiveOutcome(
    quint64 operationId,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    assertFacadeThread();
    return !shuttingDown_
        && busy_
        && activeOperationId_ == operationId
        && activeCancellation_ == cancellation;
}

bool SingleActiveWorkerRuntime::isBusy() const
{
    assertFacadeThread();
    return busy_;
}

bool SingleActiveWorkerRuntime::isRunning() const
{
    assertFacadeThread();
    return workerThread_.isRunning();
}

bool SingleActiveWorkerRuntime::isShuttingDown() const
{
    assertFacadeThread();
    return shuttingDown_;
}

bool SingleActiveWorkerRuntime::requestActiveCancellation()
{
    assertFacadeThread();
    if (!busy_ || activeCancellation_ == nullptr) {
        return false;
    }

    activeCancellation_->requestCancellation();
    return true;
}

void SingleActiveWorkerRuntime::completeOperation()
{
    assertFacadeThread();
    clearActiveOperation();
}

void SingleActiveWorkerRuntime::shutdown()
{
    assertFacadeThread();
    if (shuttingDown_) {
        return;
    }

    shuttingDown_ = true;
    requestActiveCancellation();

    if (executor_ != nullptr) {
        // A stopped thread must be running to service the blocking context
        // destruction callback on the executor's owning thread.
        if (!workerThread_.isRunning()) {
            workerThread_.start();
        }

        auto destroyContext = executorLifecycle_.destroyContext_;
        const auto invoked = QMetaObject::invokeMethod(
            executor_,
            [executor = executor_, destroyContext = std::move(destroyContext)]() {
                if (destroyContext) {
                    destroyContext(*executor);
                }
            },
            Qt::BlockingQueuedConnection);
        Q_ASSERT(invoked);

        workerThread_.quit();
        workerThread_.wait();
        executor_ = nullptr;
    }

    clearActiveOperation();
}

void SingleActiveWorkerRuntime::assertFacadeThread() const
{
    Q_ASSERT(QThread::currentThread() == facadeThread_);
}

void SingleActiveWorkerRuntime::ensureExecutor()
{
    if (executor_ == nullptr) {
        if (!executorLifecycle_.create_) 
            throw std::runtime_error("The worker executor factory is unavailable.");

        // check for nullptr is not needed. See lambda in SingleActiveWorkerFacade constructor.
        auto* const created = executorLifecycle_.create_(); 
#if 0
        if (created == nullptr) {
            throw std::runtime_error(
                "The worker executor could not be created.");
        }

        // A parented QObject cannot safely move to another thread.
        if (created->parent() != nullptr) {
            delete created;
            throw std::runtime_error("The worker executor must not have a QObject parent.");
        }
#endif // 0
        //Change executor affinity to the dedicated worker thread.
        created->moveToThread(&workerThread_);

        //Arrange deletion after the thread finishes.
        QObject::connect(
            &workerThread_,
            &QThread::finished,
            created,
            &QObject::deleteLater);

        executor_ = created;
    }

    if (!workerThread_.isRunning()) 
        workerThread_.start();
}

void SingleActiveWorkerRuntime::clearActiveOperation()
{
    busy_ = false;
    activeOperationId_ = 0;
    activeCancellation_.reset();
}
