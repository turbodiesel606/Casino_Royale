#ifndef JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP
#define JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP

#include "CancellationState.hpp"
#include "ExceptionUtils.hpp"
#include "SingleActiveWorkerRuntime.hpp"
#include "logging/ApplicationLog.hpp"

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtGlobal>

#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

// Specialize this for every request accepted by a SingleActiveWorkerFacade.
template<typename Request>
struct SingleActiveWorkerRequestTraits;

/**
 * Complete request contract for a typed single-active-worker facade.
 *
 * A request carries operation correlation, opts into domain-specific failure
 * handling through SingleActiveWorkerRequestTraits, and must produce a value
 * that can be stored in the executor's outcome variant.
 */
template<typename Request, typename Executor>
concept SingleActiveWorkerRequestFor =
std::move_constructible<Request>
&& requires(
	Executor & executor,
	Request request,
	const Request & requestView,
	QString message)
{
	typename Executor::Outcome;

	{ request.operationId_ } -> std::convertible_to<quint64>;
	{ request.cancellation_ }
	-> std::convertible_to<std::shared_ptr<CancellationState>>;

	{ SingleActiveWorkerRequestTraits<Request>::errorMessage }
	-> std::convertible_to<QString>;
	{ SingleActiveWorkerRequestTraits<Request>::shuttingDownMessage }
	-> std::convertible_to<QString>;
	{ SingleActiveWorkerRequestTraits<Request>::busyMessage }
	-> std::convertible_to<QString>;

		requires std::constructible_from<
			typename Executor::Outcome,
				decltype(executor.process(std::move(request)))>;
				requires std::constructible_from<
					typename Executor::Outcome,
						decltype(SingleActiveWorkerRequestTraits<Request>::unavailableOutcome(
							requestView,
							std::move(message)))>;
};

template<typename Executor>
class SingleActiveWorkerFacade : public QObject
{
public:
	using Outcome = typename Executor::Outcome;
	using OutcomeHandler = std::function<void(Outcome)>;
	using ExecutorFactory = std::function<Executor*()>;

	explicit SingleActiveWorkerFacade(
		QString threadName,
		QString dataDirectory,
		LogFact storageArea,
		QObject* parent = nullptr)
		: SingleActiveWorkerFacade{ // Delegating constructor
			std::move(threadName),
			[dataDirectory = std::move(dataDirectory)]() { return new Executor{dataDirectory}; },
			storageArea,
			parent}
	{
	}

	// The factory retains dependencies until lazy creation and must return a
	// non-null, unparented executor or throw. Dependency owners outlive the facade.
	explicit SingleActiveWorkerFacade(
		QString threadName,
		ExecutorFactory executorFactory,
		LogFact storageArea,
		QObject* parent = nullptr)
		: QObject{ parent }
		, runtime_{
			std::move(threadName),
			SingleActiveWorkerExecutorLifecycle{
				[this, factory = std::move(executorFactory)]() -> QObject* {
					executor_ = factory();
					return executor_;
				},
				[this](QObject&){
				/* stub param is needed for compatibility with other caller code,
				see e.g. DataRemovalWorker::DataRemovalWorker() */
					Q_ASSERT(executor_ != nullptr);
					executor_->destroyContext();
				}, storageArea } }
	{
	}

	~SingleActiveWorkerFacade() override
	{
		shutdown();
	}

	template<typename Request>
		requires SingleActiveWorkerRequestFor<Request, Executor>
	void submit(Request request)
	{
		submitRequest(std::move(request));
	}

	bool shutdown()
	{
		Q_ASSERT(QThread::currentThread() == thread());
		const auto cleanedUp = runtime_.shutdown();
		executor_ = nullptr;
		return cleanedUp;
	}

	[[nodiscard]]
	bool isRunning() const
	{
		return runtime_.isRunning();
	}

protected:
	void setOutcomeHandler(OutcomeHandler handler)
	{
		outcomeHandler_ = std::move(handler);
	}

private:
	template<typename Request>
	void submitRequest(Request&& request)
	{
		using RequestType = std::remove_cvref_t<Request>;
		using Traits = SingleActiveWorkerRequestTraits<RequestType>;
		const auto operationId = request.operationId_;

		// Admission and correlation state belong to the GUI facade thread.
		Q_ASSERT(QThread::currentThread() == thread());

		SingleActiveWorkerAdmission admission;
		try {
			admission = runtime_.tryBeginOperation(
				request.operationId_,
				request.cancellation_);
		}
		catch (...) {
			ApplicationLog::record({LogEventType::BackgroundWorkStartFailed,
				{Traits::operation, LogFact::WorkerInitialization, LogFact::CauseUnavailable, Traits::itemCount(request)}, operationId});
			//Convert executor / runtime initialization exceptions into
			// an asynchronous domain result instead of crossing the Qt boundary.
			queueUnavailableOutcome(
				request,
				common::exceptionMessage(
					std::current_exception(),
					Traits::errorMessage));
			return;
		}

		if (admission == SingleActiveWorkerAdmission::ShuttingDown) {
			ApplicationLog::record({LogEventType::BackgroundWorkStartFailed,
				{Traits::operation, LogFact::WorkerAdmission, LogFact::WorkerShuttingDown, Traits::itemCount(request)}, operationId});
			queueUnavailableOutcome(
				request,
				Traits::shuttingDownMessage);
			return;
		}

		//Only one executor request may run at a time.
		// The controller FIFO should normally prevent this branch.
		if (admission == SingleActiveWorkerAdmission::Busy) {
			ApplicationLog::record({LogEventType::BackgroundWorkInvariantViolated,
				{Traits::operation, LogFact::UnexpectedBusy, LogFact::WorkerAdmission}, operationId});
			queueUnavailableOutcome(
				request,
				Traits::busyMessage);
			return;
		}

		Q_ASSERT(executor_ != nullptr);
		auto* const executor = executor_;

		//Queue the request to the executor's worker thread.
		const bool dispatched = QMetaObject::invokeMethod(
			executor,
			[this, executor, operationId, request = std::forward<Request>(request)]() mutable
			// maybe std::move, cuz we pass request to other thread
			{
				// process() starts operation in other thread.
				auto result = executor->process(std::move(request));
				const auto verified = Traits::hasCommittedChange(result) ? LogFact::CommittedNotDelivered : LogFact::OutcomeNotDelivered;
				Outcome outcome{std::move(result)};
				// Queue the completed value back to this GUI facade.
				const bool posted = QMetaObject::invokeMethod(
					this,
					[this, outcome = std::move(outcome)]() mutable {
						// Note: This lambda executes in GUI thread.
						deliverOutcome(std::move(outcome), Traits::operation);
					},
					Qt::QueuedConnection);
				if (!posted)
					ApplicationLog::record({LogEventType::BackgroundWorkDeliveryFailed,
						{Traits::operation, LogFact::ResultPosting, verified}, operationId});
			},
			Qt::QueuedConnection);
		if (!dispatched)
			ApplicationLog::record({LogEventType::BackgroundWorkDeliveryFailed,
				{Traits::operation, LogFact::RequestDispatch, LogFact::RequestNotDelivered}, operationId});
	}

	template<typename Request> // Maybe Concept needed
	void queueUnavailableOutcome(
		const Request& request,
		QString message)
	{
		using RequestType = std::remove_cvref_t<Request>;
		using Traits = SingleActiveWorkerRequestTraits<RequestType>;

		Outcome outcome{
			Traits::unavailableOutcome(
				request,
				std::move(message))
		};

		const bool posted = QMetaObject::invokeMethod(
			this,
			[
				this,
				outcome = std::move(outcome)
			]() mutable {
				dispatchOutcome(std::move(outcome));
			},
			Qt::QueuedConnection);
		if (!posted)
			ApplicationLog::record({LogEventType::BackgroundWorkDeliveryFailed,
				{Traits::operation, LogFact::ResultPosting, LogFact::RequestNotDelivered}, request.operationId_});
	}

	void deliverOutcome(Outcome outcome, LogFact operation)
	{
		Q_ASSERT(QThread::currentThread() == thread());

		const auto isActive = std::visit(
			[this](const auto& value) {
				return runtime_.isActiveOutcome(
					value.operationId_,
					value.cancellation_);
			},
			outcome);

		if (!isActive) {
			if (!runtime_.isShuttingDown())
				ApplicationLog::record({LogEventType::UnexpectedBackgroundResultDiscarded,
					{operation, runtime_.isBusy() ? LogFact::CorrelationMismatch : LogFact::NoActiveOperation}});
			return;
		}

		runtime_.completeOperation();
		dispatchOutcome(std::move(outcome));
	}

	void dispatchOutcome(Outcome outcome)
	{
		Q_ASSERT(QThread::currentThread() == thread());
		Q_ASSERT(outcomeHandler_);

		if (outcomeHandler_)
			std::invoke(outcomeHandler_, std::move(outcome));
	}

	Executor* executor_ = nullptr;
	SingleActiveWorkerRuntime runtime_;
	OutcomeHandler outcomeHandler_;
};

#endif // JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP
