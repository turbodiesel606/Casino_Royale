#ifndef JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP
#define JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP

#include "CancellationState.hpp"
#include "ExceptionUtils.hpp"
#include "SingleActiveWorkerRuntime.hpp"

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

	explicit SingleActiveWorkerFacade(
		QString threadName,
		QString dataDirectory,
		QObject* parent = nullptr)
		: QObject{ parent }
		, runtime_{
			std::move(threadName),
			SingleActiveWorkerExecutorLifecycle{
				[this, dataDirectory = std::move(dataDirectory)]() -> QObject* {
					executor_ = new Executor{ dataDirectory };
					return executor_;
				},
				[this](QObject&){
				/* stub param is needed for compatibility with other caller code,
				see e.g. DataRemovalWorker::DataRemovalWorker() */
					Q_ASSERT(executor_ != nullptr);
					executor_->destroyContext();
				} } }
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

	void shutdown()
	{
		Q_ASSERT(QThread::currentThread() == thread());
		runtime_.shutdown();
		executor_ = nullptr;
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

		// COMMENT: Admission and correlation state belong to the GUI facade thread.
		Q_ASSERT(QThread::currentThread() == thread());

		SingleActiveWorkerAdmission admission;
		try {
			admission = runtime_.tryBeginOperation(
				request.operationId_,
				request.cancellation_);
		}
		catch (...) {
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
			queueUnavailableOutcome(
				request,
				Traits::shuttingDownMessage);
			return;
		}

		//Only one executor request may run at a time.
		// The controller FIFO should normally prevent this branch.
		if (admission == SingleActiveWorkerAdmission::Busy) {
			queueUnavailableOutcome(
				request,
				Traits::busyMessage);
			return;
		}

		Q_ASSERT(executor_ != nullptr);
		auto* const executor = executor_;

		//Queue the request to the executor's worker thread.
		QMetaObject::invokeMethod(
			executor,
			[this, executor, request = std::forward<Request>(request)]() mutable
			// maybe std::move, cuz we pass request to other thread
			{
				// process() starts operation in other thread.
				Outcome outcome{ executor->process(std::move(request)) };
				// Queue the completed value back to this GUI facade.
				QMetaObject::invokeMethod(
					this,
					[this, outcome = std::move(outcome)]() mutable {
						// Note: This lambda executes in GUI thread.
						deliverOutcome(std::move(outcome));
					},
					Qt::QueuedConnection);
			},
			Qt::QueuedConnection);
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

		QMetaObject::invokeMethod(
			this,
			[
				this,
				outcome = std::move(outcome)
			]() mutable {
				dispatchOutcome(std::move(outcome));
			},
			Qt::QueuedConnection);
	}

	void deliverOutcome(Outcome outcome)
	{
		Q_ASSERT(QThread::currentThread() == thread());

		const auto isActive = std::visit(
			[this](const auto& value) {
				return runtime_.isActiveOutcome(
					value.operationId_,
					value.cancellation_);
			},
			outcome);

		if (!isActive)
			return;

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

	//QString dataDirectory_; // maybe this field is not needed, cuz we std::move it to Executor
	Executor* executor_ = nullptr;
	SingleActiveWorkerRuntime runtime_;
	OutcomeHandler outcomeHandler_;
};

#endif // JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERFACADE_HPP
