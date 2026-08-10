#include "CancellationState.hpp"

#include <mutex>

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
