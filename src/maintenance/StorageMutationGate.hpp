#ifndef JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP

#include <QObject>

// Serializes destructive maintenance against queued job-save and Add CV work.
class StorageMutationGate final : public QObject
{
    Q_OBJECT

public:
    explicit StorageMutationGate(QObject* parent = nullptr);

    bool reserveJobSave();
    void releaseJobSave();
    bool reserveCvImports(int count);
    void releaseCvImports(int count);
    bool beginRemoval();
    void endRemoval();

    bool removalActive() const;
    bool canBeginRemoval() const;

signals:
    void stateChanged();

private:
    int pendingJobSaves_ = 0;
    int pendingCvImports_ = 0;
    bool removalActive_ = false;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP
