#ifndef JOBTRACKER_SRC_CVS_CVPERSISTENCEOBSERVATION_HPP
#define JOBTRACKER_SRC_CVS_CVPERSISTENCEOBSERVATION_HPP

#include "logging/LogEvent.hpp"

enum class CvImportDisposition;

// Codex Comment: Owned by one save or import call. Nested file/SQL work updates the
// observed stage; the transaction owner publishes the result after rollback and cleanup.
struct CvPersistenceObservation final
{
    LogFact operation_;
    quint64 operationId_ = 0;
    LogFact stage_ = LogFact::PersistPrepare;
    LogFact cause_ = LogFact::CauseUnavailable;
    LogFact rollback_ = LogFact::NotAttempted;
    LogFact cleanup_ = LogFact::NotAttempted;
    bool reportFailure_ = false;

    void recordFailure() const noexcept;
    void stopped(LogFact boundary) const noexcept;
    void invariant(LogFact problem) const noexcept;
};

LogFact cvDispositionFact(CvImportDisposition disposition) noexcept;

#endif
