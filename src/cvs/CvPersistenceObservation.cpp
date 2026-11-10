#include "CvPersistenceObservation.hpp"

#include "CvImportService.hpp"
#include "logging/ApplicationLog.hpp"

void CvPersistenceObservation::stopped(LogFact boundary) const noexcept
{
    ApplicationLog::record(operationId_, LogEventType::OperationStoppedByCancellation,
        operation_, boundary, LogFact::None);
}

void CvPersistenceObservation::invariant(LogFact problem) const noexcept
{
    ApplicationLog::record(operationId_, LogEventType::BackgroundWorkInvariantViolated,
        operation_, problem, stage_);
}

void CvPersistenceObservation::recordFailure() const noexcept
{
    if (cause_ == LogFact::PersistMissingPrepared || cause_ == LogFact::PersistAlreadyStaged) {
        invariant(cause_);
    } else if (stage_ == LogFact::PersistLookupJob && cause_ == LogFact::Missing) {
        ApplicationLog::record(operationId_, LogEventType::RequestedRecordStateMismatch,
            operation_, cause_);
    } else if (operation_ == LogFact::JobCreation) {
        ApplicationLog::record(operationId_, LogEventType::JobApplicationCreationFailed,
            stage_, cause_, rollback_, cleanup_);
    } else if (operation_ == LogFact::JobUpdate) {
        const auto preserved = rollback_ == LogFact::Failed ? LogFact::PersistUncertain : LogFact::PersistPreserved;
        ApplicationLog::record(operationId_, LogEventType::JobApplicationUpdateFailed,
            stage_, cause_, preserved, rollback_, cleanup_);
    } else if (stage_ == LogFact::PersistSourceInspection &&
               (cause_ == LogFact::Missing || cause_ == LogFact::PersistNotFile)) {
        ApplicationLog::record(operationId_, LogEventType::CvImportSourceUnavailable, cause_);
    } else if (stage_ == LogFact::PersistSourceInspection || stage_ == LogFact::PersistSourceOpen ||
               stage_ == LogFact::PersistSourceRead || stage_ == LogFact::PersistSourceBuffer) {
        ApplicationLog::record(operationId_, LogEventType::CvSourceReadFailed, stage_, operation_, cause_);
    } else if (stage_ == LogFact::PersistCreateStage || stage_ == LogFact::PersistWriteStage ||
               stage_ == LogFact::PersistFlushStage || stage_ == LogFact::PersistRenameStage) {
        ApplicationLog::record(operationId_, LogEventType::ManagedCvCopyFailed, stage_, cause_, cleanup_);
    } else {
        const auto file = cleanup_ == LogFact::Failed ? LogFact::PersistRetainedFile :
            cleanup_ == LogFact::Succeeded ? LogFact::PersistRemovedFile : LogFact::PersistNoFile;
        ApplicationLog::record(operationId_, LogEventType::CvImportPersistenceFailed, stage_, cause_, file);
    }
    if (cleanup_ == LogFact::Failed) {
        ApplicationLog::record(operationId_, LogEventType::FailedOperationFileCleanupIncomplete,
            operation_, LogFact::PersistOrphanFile, LogFact::FileError);
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

