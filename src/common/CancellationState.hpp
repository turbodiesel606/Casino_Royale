#ifndef JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP
#define JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP

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

#endif // JOBTRACKER_SRC_COMMON_CANCELLATIONSTATE_HPP
