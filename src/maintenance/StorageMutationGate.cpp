#include "StorageMutationGate.hpp"

StorageMutationGate::StorageMutationGate(QObject* parent)
    : QObject{parent}
{
}

bool StorageMutationGate::reserveJobSave()
{
    if (removalActive_) {
        return false;
    }
    ++pendingJobSaves_;
    emit stateChanged();
    return true;
}

void StorageMutationGate::releaseJobSave()
{
    if (pendingJobSaves_ <= 0) {
        return;
    }
    --pendingJobSaves_;
    emit stateChanged();
}

bool StorageMutationGate::reserveCvImports(int count)
{
    if (count <= 0) {
        return true;
    }
    if (removalActive_) {
        return false;
    }
    pendingCvImports_ += count;
    emit stateChanged();
    return true;
}

void StorageMutationGate::releaseCvImports(int count)
{
    if (count <= 0 || pendingCvImports_ <= 0) {
        return;
    }
    pendingCvImports_ = qMax(0, pendingCvImports_ - count);
    emit stateChanged();
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
