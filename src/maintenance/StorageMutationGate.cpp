#include "StorageMutationGate.hpp"

StorageMutationGate::StorageMutationGate(QObject* parent)
    : QObject{parent}
{
}

bool StorageMutationGate::reserveJobSave()
{
    return reserveOperations(pendingJobSaves_, 1);
}

void StorageMutationGate::releaseJobSave()
{
    releaseOperations(pendingJobSaves_, 1);
}

bool StorageMutationGate::reserveCvImports(int count)
{
    return reserveOperations(pendingCvImports_, count);
}

void StorageMutationGate::releaseCvImports(int count)
{
    releaseOperations(pendingCvImports_, count);
}

bool StorageMutationGate::beginRemoval()
{
    if (!canBeginRemoval()) {
        return false;
    }
    removalActive_ = true;
    emit stateChanged();
    return true;
}

void StorageMutationGate::endRemoval()
{
    if (!removalActive_) {
        return;
    }
    removalActive_ = false;
    emit stateChanged();
}

bool StorageMutationGate::removalActive() const
{
    return removalActive_;
}

bool StorageMutationGate::canBeginRemoval() const
{
    return !removalActive_ && pendingJobSaves_ == 0 && pendingCvImports_ == 0;
}

bool StorageMutationGate::reserveOperations(int& pendingCount, int count)
{
    // Reserving zero operations is a successful no - op.
    if (count <= 0)
        return true;

    // Removal has exclusive mutation ownership.
    if (removalActive_)
        return false;

    // Track admitted work and notify dependent UI/controller state.
    pendingCount += count;
    emit stateChanged();
    return true;
}

void StorageMutationGate::releaseOperations(int& pendingCount, int count)
{
    if (count <= 0 || pendingCount <= 0) {
        return;
    }
    pendingCount = qMax(0, pendingCount - count);
    emit stateChanged();
}
