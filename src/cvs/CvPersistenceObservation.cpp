#include "CvPersistenceObservation.hpp"

#include "CvImportService.hpp"
#include "logging/ApplicationLog.hpp"

void CvPersistenceObservation::stopped(LogFact boundary) const noexcept
{
    ApplicationLog::record({LogEventType::OperationStoppedByCancellation,
        {operation_, boundary, LogFact::None}, operationId_});
}

void CvPersistenceObservation::invariant(LogFact problem) const noexcept
{
    ApplicationLog::record({LogEventType::BackgroundWorkInvariantViolated,
        {operation_, problem, stage_}, operationId_});
}

void CvPersistenceObservation::recordFailure() const noexcept
{
    if (cause_ == LogFact::PersistMissingPrepared || cause_ == LogFact::PersistAlreadyStaged) {
        invariant(cause_);
    } else if (stage_ == LogFact::PersistLookupJob && cause_ == LogFact::Missing) {
        ApplicationLog::record({LogEventType::RequestedRecordStateMismatch,
            {operation_, cause_}, operationId_});
    } else if (operation_ == LogFact::JobCreation) {
        ApplicationLog::record({LogEventType::JobApplicationCreationFailed,
            {stage_, cause_, rollback_, cleanup_}, operationId_});
    } else if (operation_ == LogFact::JobUpdate) {
        const auto preserved = rollback_ == LogFact::Failed ? LogFact::PersistUncertain : LogFact::PersistPreserved;
        ApplicationLog::record({LogEventType::JobApplicationUpdateFailed,
            {stage_, cause_, preserved, rollback_, cleanup_}, operationId_});
    } else if (stage_ == LogFact::PersistSourceInspection &&
               (cause_ == LogFact::Missing || cause_ == LogFact::PersistNotFile)) {
        ApplicationLog::record({LogEventType::CvImportSourceUnavailable, {cause_}, operationId_});
    } else if (stage_ == LogFact::PersistSourceInspection || stage_ == LogFact::PersistSourceOpen ||
               stage_ == LogFact::PersistSourceRead || stage_ == LogFact::PersistSourceBuffer) {
        ApplicationLog::record({LogEventType::CvSourceReadFailed, {stage_, operation_, cause_}, operationId_});
    } else if (stage_ == LogFact::PersistCreateStage || stage_ == LogFact::PersistWriteStage ||
               stage_ == LogFact::PersistFlushStage || stage_ == LogFact::PersistRenameStage) {
        ApplicationLog::record({LogEventType::ManagedCvCopyFailed, {stage_, cause_, cleanup_}, operationId_});
    } else {
        const auto file = cleanup_ == LogFact::Failed ? LogFact::PersistRetainedFile :
            cleanup_ == LogFact::Succeeded ? LogFact::PersistRemovedFile : LogFact::PersistNoFile;
        ApplicationLog::record({LogEventType::CvImportPersistenceFailed, {stage_, cause_, file}, operationId_});
    }
    if (cleanup_ == LogFact::Failed) {
        ApplicationLog::record({LogEventType::FailedOperationFileCleanupIncomplete,
            {operation_, LogFact::PersistOrphanFile, LogFact::FileError}, operationId_});
    }
}

LogFact cvDispositionFact(CvImportDisposition disposition) noexcept
{
    switch (disposition) {
    case CvImportDisposition::Inserted: return LogFact::PersistCreatedCv;
    case CvImportDisposition::RestoredArchived: return LogFact::PersistRestoredCv;
    case CvImportDisposition::ExistingActive: return LogFact::PersistReusedCv;
    }
    return LogFact::Unknown;
}

