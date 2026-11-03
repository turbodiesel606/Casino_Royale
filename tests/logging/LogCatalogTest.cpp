#include "logging/LogEvent.hpp"

#include <QRegularExpression>
#include <QtTest/QtTest>

#include <array>

class LogCatalogTest final : public QObject
{
    Q_OBJECT
private slots:
    void rendersEverySpecifiedEvent();
    void omitsUnavailableDetailsAndFormatsCorrelation();
};

void LogCatalogTest::rendersEverySpecifiedEvent()
{
    struct Example { LogEvent event_; const char* expected_; };
    const Example examples[]{
        {{LogEventType::ApplicationStorageSetupFailed, {LogFact::ManagedCvArea, LogFact::TransactionCommit, LogFact::DbBusy}},
         "ERROR | Storage | Message: Could not prepare managed CV storage during transaction commit. Reason: database busy."},
        {{LogEventType::DatabaseConnectionSetupFailed, {LogFact::TransactionCommit, LogFact::JobUpdate, LogFact::DbBusy, 6}},
         "ERROR | Database | Message: Database transaction commit failed during job update. Reason: database busy. Affected items: 6."},
        {{LogEventType::DatabaseSchemaVersionCheckFailed, {LogFact::TransactionCommit, 9, 4, LogFact::DbBusy}},
         "ERROR | Database | Message: Schema version check failed during transaction commit. Detected version: 9. Supported version: 4, reason: database busy."},
        {{LogEventType::DatabaseSchemaMigrationFailed, {1, 4, LogFact::TransactionCommit, LogFact::DbBusy, LogFact::Failed}},
         "ERROR | Database | Message: Schema change from 1 to 4 failed at transaction commit. Reason: database busy. Rollback: failed."},
        {{LogEventType::SavedDatasetLoadFailed, {LogFact::Companies, LogFact::DbBusy}},
         "ERROR | Storage | Message: Could not load companies. Reason: database busy."},
        {{LogEventType::ManagedCvRecoveryFailed, {LogFact::PersistRestoreRetained, LogFact::FileError, LogValue::counts(2, 3, 4)}},
         "ERROR | CvLibrary | Message: Startup CV recovery failed at retained file restoration. Reason: file operation failed. Progress before failure: restored 2, removed 3, quarantined 4."},
        {{LogEventType::MainWindowCreationFailed, {LogFact::InterfaceLoading, LogFact::RootWindowUnavailable}},
         "ERROR | Application | Message: Main-window creation failed at interface loading. Reason: no root window."},
        {{LogEventType::JobApplicationCreationFailed, {LogFact::TransactionCommit, LogFact::DbBusy, LogFact::Failed, LogFact::Succeeded}},
         "ERROR | JobApplications | Message: Job creation failed at transaction commit. Reason: database busy. Rollback: failed, file cleanup: succeeded."},
        {{LogEventType::JobApplicationUpdateFailed, {LogFact::TransactionCommit, LogFact::DbBusy, LogFact::Unknown, LogFact::Failed, LogFact::Failed}},
         "ERROR | JobApplications | Message: Job update failed at transaction commit. Reason: database busy. Previous state: unknown, rollback: failed, file cleanup: failed."},
        {{LogEventType::CvSourceReadFailed, {LogFact::PersistSourceRead, LogFact::CvImport, LogFact::FileError}},
         "ERROR | CvLibrary | Message: CV source read failed during CV import. Reason: file operation failed."},
        {{LogEventType::ManagedCvCopyFailed, {LogFact::PersistFlushStage, LogFact::FileError, LogFact::Succeeded}},
         "ERROR | CvLibrary | Message: Managed CV copy failed at staged copy flush. Reason: file operation failed. Cleanup: succeeded."},
        {{LogEventType::CvImportPersistenceFailed, {LogFact::TransactionCommit, LogFact::DbBusy, LogFact::PersistRemovedFile}},
         "ERROR | CvLibrary | Message: Standalone CV import failed at transaction commit. Reason: database busy. Unreferenced managed file: removed."},
        {{LogEventType::CvFavoriteSaveFailed, {LogFact::DbBusy}},
         "ERROR | CvLibrary | Message: Could not save the CV favorite change. Reason: database busy."},
        {{LogEventType::CommittedChangePublicationFailed, {LogFact::JobUpdate, LogFact::DbBusy}},
         "ERROR | ApplicationModel | Message: job update committed successfully. The displayed record could not be updated because database busy."},
        {{LogEventType::JobApplicationDeletionFailed, {LogFact::TransactionCommit, LogFact::DbBusy, 6, 2}},
         "ERROR | JobApplications | Message: Job deletion failed at transaction commit. Reason: database busy. Committed: 6, failed: 2."},
        {{LogEventType::LinkedCvArchiveFailed, {LogFact::DbBusy}},
         "ERROR | CvLibrary | Message: Could not archive a linked active CV. Reason: database busy."},
        {{LogEventType::ArchivedCvRestoreFailed, {LogFact::TransactionCommit, LogFact::DbBusy}},
         "ERROR | CvLibrary | Message: CV restoration failed at transaction commit. Reason: database busy."},
        {{LogEventType::PermanentCvDeletionFailed, {LogFact::TransactionCommit, LogFact::DbBusy, LogFact::NotAttempted}},
         "ERROR | CvLibrary | Message: Permanent CV deletion failed at transaction commit. Reason: database busy. Original file restoration: not attempted."},
        {{LogEventType::ManagedCvOpenFailed, {LogFact::DesktopHandoff, LogFact::DesktopRejected}},
         "ERROR | CvLibrary | Message: Opening the managed CV failed at operating system handoff. Reason: open request rejected."},
        {{LogEventType::InvalidManagedCvReferenceRejected, {LogFact::OpenCv, LogFact::SymlinkReference}},
         "ERROR | CvLibrary | Message: Refused CV open because the saved file reference was a symbolic link."},
        {{LogEventType::DatabaseRollbackFailed, {LogFact::JobUpdate, LogFact::DbBusy}},
         "ERROR | Database | Message: Rollback failed during job update. Reason: database busy. Database state is uncertain."},
        {{LogEventType::RetainedCvFileRestoreFailed, {LogFact::DbBusy}},
         "ERROR | CvLibrary | Message: CV deletion did not commit and its retained file could not be restored. Reason: database busy. The database and managed files may be inconsistent."},
        {{LogEventType::BackgroundWorkStartFailed, {LogFact::JobUpdate, LogFact::TransactionCommit, LogFact::DbBusy, 13}},
         "ERROR | BackgroundWork | Message: Accepted job update could not start at transaction commit. Reason: database busy. Affected items: 13."},
        {{LogEventType::BackgroundWorkInvariantViolated, {LogFact::JobUpdate, LogFact::UnexpectedBusy, LogFact::TransactionCommit}},
         "ERROR | BackgroundWork | Message: job update encountered an unexpectedly busy worker during transaction commit."},
        {{LogEventType::BackgroundWorkDeliveryFailed, {LogFact::JobUpdate, LogFact::ResultPosting, LogFact::CommittedNotDelivered}},
         "ERROR | BackgroundWork | Message: job update delivery failed at result posting. Persistence and visibility: a committed result did not reach the model."},
        {{LogEventType::BackgroundShutdownCleanupFailed, {LogFact::ShutdownContextDispatch, LogFact::WorkerStorage, LogFact::DbBusy}},
         "ERROR | BackgroundWork | Message: Shutdown could not complete worker context cleanup for worker-owned storage. Reason: database busy."},
        {{LogEventType::MutationBlockedByRemoval, {LogFact::OpenCv, LogFact::CvArchive, 2}},
         "WARNING | Storage | Message: Declined open because CV archiving was active. Repeated attempts: 2."},
        {{LogEventType::RemovalBlockedByPendingWork, {LogFact::CvDeletion, LogFact::ImportOrUpdate}},
         "WARNING | Storage | Message: Declined CV deletion because imports or updates was pending or active."},
        {{LogEventType::JobUpdateAlreadyPending},
         "WARNING | JobApplications | Message: Declined a job update because another update was pending."},
        {{LogEventType::RequestedRecordStateMismatch, {LogFact::OpenCv, LogFact::RemovalAlreadyArchived}},
         "WARNING | Storage | Message: Declined open because the requested record was already archived."},
        {{LogEventType::CvImportSourceUnavailable, {LogFact::Missing}},
         "WARNING | CvLibrary | Message: The accepted CV import found that its source was missing when processed."},
        {{LogEventType::LinkedCvPermanentDeletionSkipped, {LogFact::RemovalSkipped, 7, 7}},
         "WARNING | CvLibrary | Message: Permanent deletion skipped protected items because 7 selected CVs were linked to jobs. Skipped: 7."},
        {{LogEventType::DeletedCvFileAlreadyMissing, {13}},
         "WARNING | CvLibrary | Message: CV record deletion committed for 13 items whose managed files were already missing."},
        {{LogEventType::DeletedCvFileCleanupPending, {13}},
         "WARNING | CvLibrary | Message: CV record deletion committed for 13 items. Final file cleanup failed and remains pending for startup recovery."},
        {{LogEventType::FailedOperationFileCleanupIncomplete, {LogFact::JobUpdate, LogFact::PersistPartialFile, LogFact::DbBusy}},
         "WARNING | CvLibrary | Message: Cleanup after job update failed to remove an unfinished managed copy. Reason: database busy."},
        {{LogEventType::UnreferencedCvFilesQuarantined, {2, 3, 4}},
         "WARNING | CvLibrary | Message: Startup moved 2 unreferenced files into quarantine. Retained CV files restored: 3, temporary files removed: 4."},
        {{LogEventType::SavedFieldFallbacksApplied, {LogValue::counts(1, 2, 3, 4)}},
         "WARNING | Storage | Message: Loading continued with fallback values. Affected records by field category: job status 1, work format 2, required date/time 3, archive timestamp 4."},
        {{LogEventType::UnexpectedBackgroundResultDiscarded, {LogFact::JobUpdate, LogFact::CorrelationMismatch}},
         "WARNING | BackgroundWork | Message: Discarded a result for job update during normal operation. Mismatch: operation or cancellation identity mismatch."},
        {{LogEventType::ApplicationStartupBegan},
         "^INFO \\| Application \\| Message: JobTracker [0-9]+(?:\\.[0-9]+)+ started initialization on (Windows|Linux|macOS|unknown platform)\\.$"},
        {{LogEventType::ApplicationStarted},
         "INFO | Application | Message: Startup completed and the main window is available."},
        {{LogEventType::DatabaseSchemaInitialized, {4}},
         "INFO | Database | Message: Initial database schema version 4 committed successfully."},
        {{LogEventType::DatabaseUpgradeStarted, {1, 4}},
         "INFO | Database | Message: Database upgrade from version 1 to 4 began."},
        {{LogEventType::DatabaseUpgradeCompleted, {1, 4}},
         "INFO | Database | Message: Database upgrade from version 1 to 4 committed after relationship checks passed."},
        {{LogEventType::ManagedCvFilesRecovered, {3, 4}},
         "INFO | CvLibrary | Message: Startup restored 3 retained CV files and removed 4 obsolete temporary files."},
        {{LogEventType::BackgroundSaveOrImportAccepted, {LogFact::JobUpdate, 13}},
         "INFO | BackgroundWork | Message: Accepted job update for background processing. Admitted items: 13."},
        {{LogEventType::JobApplicationCreated, {LogFact::PersistCreatedCv}},
         "INFO | JobApplications | Message: Job creation committed. CV outcome: new managed CV."},
        {{LogEventType::JobApplicationUpdated, {LogFact::Yes, LogFact::No, LogFact::PersistReusedCv}},
         "INFO | JobApplications | Message: Job update committed. CV replacement: yes, relationship changed: no, CV outcome: existing active CV reused."},
        {{LogEventType::StandaloneCvImported},
         "INFO | CvLibrary | Message: Standalone import committed a new managed CV."},
        {{LogEventType::ActiveCvReusedOnImport},
         "INFO | CvLibrary | Message: Standalone import reused an existing active CV without adding another copy."},
        {{LogEventType::ArchivedCvRestoredOnImport},
         "INFO | CvLibrary | Message: Standalone import restored a matching archived CV to the active library."},
        {{LogEventType::RemovalOperationAccepted, {LogFact::JobUpdate, 7}},
         "INFO | DataRemoval | Message: Accepted job update for 7 selected records."},
        {{LogEventType::JobApplicationsDeleted, {13}},
         "INFO | JobApplications | Message: Job deletion committed for 13 applications."},
        {{LogEventType::LinkedActiveCvsArchived, {13}},
         "INFO | CvLibrary | Message: Archived 13 linked active CVs."},
        {{LogEventType::UnlinkedCvsPermanentlyDeleted, {13}},
         "INFO | CvLibrary | Message: Permanently deleted 13 eligible CVs and completed physical cleanup."},
        {{LogEventType::ArchivedCvsRestored, {13}},
         "INFO | CvLibrary | Message: Restored 13 archived CVs to the active library."},
        {{LogEventType::PendingWorkCancellationRequested, {2, 3}},
         "INFO | BackgroundWork | Message: Requested cancellation of 2 active operations and removed 3 queued requests."},
        {{LogEventType::OperationStoppedByCancellation, {LogFact::JobUpdate, LogFact::RemovalBetweenItems, LogValue::counts(2, 4)}},
         "INFO | BackgroundWork | Message: job update stopped at the next selected item because cancellation was requested. Processed: 2, unprocessed: 4."},
        {{LogEventType::OperationCommittedAfterCancellationRequest, {LogFact::JobUpdate, LogValue::jobUpdate(true, false, LogFact::PersistReusedCv)}},
         "INFO | BackgroundWork | Message: job update committed after cancellation had been requested. CV replacement: yes, relationship changed: no, CV outcome: existing active CV reused."},
        {{LogEventType::CloseDeferredByUser},
         "INFO | Application | Message: The user selected Wait while work was pending. The application remains open."},
        {{LogEventType::InterruptAndExitSelected, {3, 2}},
         "INFO | Application | Message: The user selected Interrupt and Exit and new work was blocked. Queued requests removed: 3, active operations asked to stop: 2."},
        {{LogEventType::ControlledShutdownStarted, {LogValue::counts(3, 4)}},
         "INFO | Application | Message: Controlled shutdown began. Pending work: active 3, queued 4."},
        {{LogEventType::ControlledShutdownCompleted},
         "INFO | Application | Message: Controlled shutdown and required background cleanup completed."}
    };
    static_assert(std::size(examples) == static_cast<int>(LogEventType::Count));
    const QByteArray prefix{"1970-01-01 00:00:00.123 | "};
    for (const auto& example : examples) {
        const auto line = formatLogLine(example.event_, 123);
        QVERIFY(line.startsWith(prefix));
        QCOMPARE(line.count('\n'), 1);
        QVERIFY(line.endsWith('\n'));
        QVERIFY(!line.contains('\r'));
        const auto body = line.sliced(prefix.size(), line.size() - prefix.size() - 1);
        if (example.event_.type_ == LogEventType::ApplicationStartupBegan) {
            const QRegularExpression startup{QString::fromLatin1(example.expected_)};
            QVERIFY2(startup.match(QString::fromLatin1(body)).hasMatch(), body.constData());
        } else {
            QCOMPARE(body, QByteArray{example.expected_});
        }
        QVERIFY(!body.contains('{'));
        QVERIFY(body.count(". ") <= 2);
    }
}

void LogCatalogTest::omitsUnavailableDetailsAndFormatsCorrelation()
{
    const auto line = formatLogLine({LogEventType::DatabaseConnectionSetupFailed,
        {LogFact::DbOpen, LogFact::Startup, LogFact::DbBusy, LogFact::None}, 17}, 0);
    QVERIFY(line.endsWith("Database connection open failed during startup. Reason: database busy (operation 17).\n"));
    QVERIFY(!line.contains("Affected items"));
    QVERIFY(formatLogLine({LogEventType::Count}, 0).isEmpty());
}

QTEST_GUILESS_MAIN(LogCatalogTest)
#include "LogCatalogTest.moc"
