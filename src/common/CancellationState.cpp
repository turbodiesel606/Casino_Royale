#include "CancellationState.hpp"

#include <mutex>
// replace controlled variable with atomic in future
void CancellationState::requestCancellation()
{
    const std::scoped_lock lock{mutex_};
    cancellationRequested_ = true;
}

bool CancellationState::isCancellationRequested() const
{
    const std::scoped_lock lock{mutex_};
    return cancellationRequested_;
}
