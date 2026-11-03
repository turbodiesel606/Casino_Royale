#ifndef JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP
#define JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP

#include <memory>
#include <mutex>

// Shares cooperative cancellation across threads without exposing the
// synchronization primitive to callers.
class CancellationState final
    // replace controlled variable with atomic in future
{
public:
    void requestCancellation();
    bool isCancellationRequested() const;

private:
    mutable std::mutex mutex_;
    bool cancellationRequested_ = false;
};

// Codex Comment: This diagnostic observation must never prevent a transaction from committing.
bool cancellationObservedBeforeCommit(const std::shared_ptr<CancellationState>& cancellation) noexcept;

#endif // JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP
