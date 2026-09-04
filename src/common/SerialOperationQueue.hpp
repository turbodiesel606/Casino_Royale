#ifndef JOBTRACKER_SRC_COMMON_SERIALOPERATIONQUEUE_HPP
#define JOBTRACKER_SRC_COMMON_SERIALOPERATIONQUEUE_HPP

#include "CancellationState.hpp"

#include <QtGlobal>

#include <deque>
#include <memory>
#include <optional>
#include <utility>

/**
 * Owns the value-level mechanics of a FIFO with exactly one active operation.
 *
 * The queue assigns stable monotonically increasing operation IDs and creates
 * the active cooperative-cancellation identity. Domain controllers retain
 * admission policy, signals, worker submission, and completion side effects.
 */
template<typename Payload>
class SerialOperationQueue final
{
public:
    struct Entry final
    {
        quint64 operationId_ = 0;
        Payload payload_;
    };

    SerialOperationQueue() = default;
    SerialOperationQueue(const SerialOperationQueue&) = delete;
    SerialOperationQueue& operator=(const SerialOperationQueue&) = delete;
    SerialOperationQueue(SerialOperationQueue&&) = delete;
    SerialOperationQueue& operator=(SerialOperationQueue&&) = delete;

    quint64 enqueue(Payload payload)
    {
        const auto operationId = nextOperationId_ + 1;
        waiting_.emplace_back(operationId, std::move(payload));
        nextOperationId_ = operationId;
        return operationId;
    }

    /** Activates the next entry and creates its cancellation identity. */
    bool activateNext()
    {
        if (active_.has_value() || waiting_.empty())
            return false;

        auto cancellation = std::make_shared<CancellationState>();
        active_ = std::move(waiting_.front());
        waiting_.pop_front();
        activeCancellation_ = std::move(cancellation);
        completionSuppressed_ = false;
        return true;
    }

    int waitingCount() const
    {
        return static_cast<int>(waiting_.size());
    }

    int pendingCount() const
    {
        return waitingCount() + (active_.has_value() ? 1 : 0);
    }

    bool hasActive() const
    {
        return active_.has_value();
    }

    bool isDrained() const
    {
        return waiting_.empty() && !active_.has_value();
    }

    const Entry* active() const
    {
        return active_.has_value() ? &*active_ : nullptr;
    }

    std::shared_ptr<CancellationState> activeCancellation() const
    {
        return activeCancellation_;
    }

    bool matches(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation) const
    {
        return active_.has_value()
            && active_->operationId_ == operationId
            && activeCancellation_ == cancellation;
    }

    /**
     * Requests cancellation of the active entry. Suppression is sticky for
     * that entry and is unchanged when no entry is active.
     */
    bool requestActiveCancellation(bool suppressCompletion = true)
    {
        if (!active_.has_value()) {
            return false;
        }

        completionSuppressed_ = completionSuppressed_ || suppressCompletion;
        activeCancellation_->requestCancellation();
        return true;
    }

    bool completionSuppressed() const
    {
        return active_.has_value() && completionSuppressed_;
    }

    /** Clears only waiting entries and preserves the operation-ID sequence. */
    int clearWaiting()
    {
        const auto removed = waitingCount();
        waiting_.clear();
        return removed;
    }

    /** Releases the active entry without changing the next operation ID. */
    void finishActive()
    {
        active_.reset();
        activeCancellation_.reset();
        completionSuppressed_ = false;
    }

private:
    std::deque<Entry> waiting_;
    std::optional<Entry> active_;
    std::shared_ptr<CancellationState> activeCancellation_;
    quint64 nextOperationId_ = 0;
    bool completionSuppressed_ = false;
};

#endif // JOBTRACKER_SRC_COMMON_SERIALOPERATIONQUEUE_HPP
