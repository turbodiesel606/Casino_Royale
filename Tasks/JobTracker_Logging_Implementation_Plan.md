# JobTracker implementation plan for application logging

Implement a process-local asynchronous logger for all 62 events in the existing specification. **Every event will use a descriptive `enum class LogEventType` value with one predefined severity, component, and message template.** Producers will supply only the typed facts needed by that template. [Logging event specification](../Requirements/JobTracker_Logging_Events_Updated.md)

The primary acceptance criterion is behavioral equivalence. A job save, CV import, deletion, cancellation, or shutdown must produce the same business outcome whether logging succeeds, the queue is full, the directory is unwritable, or the logger never starts.

## 1. Event model and catalog

Define `enum class LogEventType` and a compile-time descriptor table in `src/logging`. Each descriptor contains the event type, `LogSeverity`, `LogComponent`, and message template. The producer API accepts `LogEvent` and returns `void`. Callers cannot override severity, component, or message text.

Use bounded, owned payloads made from enums, counts, flags, and existing operation numbers where correlation is necessary. The formatter converts them to allowlisted English phrases. For example, `{cause}` can render as `permission denied`, `database busy`, or `cause unavailable`. It cannot contain `exception.what()`, driver text, a filename, or a UI notification. Capture application version from build metadata and platform from a fixed platform value.

The catalog below is in specification order so a reviewer can check it row by row. The descriptive names are the C++ enumerators. Placeholders represent typed fields. Optional detail phrases such as `{cvDetail}` also come from predefined templates.

### ERROR events · 26

| `LogEventType` | Fixed component | Message template |
|---|---|---|
| `ApplicationStorageSetupFailed` | Storage | Could not prepare `{area}` during `{stage}`. Reason: `{cause}`. |
| `DatabaseConnectionSetupFailed` | Database | Database `{stage}` failed during `{operation}`. Reason: `{cause}`. `{affectedCountDetail}` |
| `DatabaseSchemaVersionCheckFailed` | Database | Schema version check failed during `{stage}`. Detected version: `{detected}`. Supported version: `{supported}`, reason: `{cause}`. |
| `DatabaseSchemaMigrationFailed` | Database | Schema change from `{source}` to `{target}` failed at `{stage}`. Reason: `{cause}`. Rollback: `{rollback}`. |
| `SavedDatasetLoadFailed` | Storage | Could not load `{dataset}`. Reason: `{cause}`. |
| `ManagedCvRecoveryFailed` | CvLibrary | Startup CV recovery failed at `{stage}`. Reason: `{cause}`. Progress before failure: `{recoveryCounts}`. |
| `MainWindowCreationFailed` | Application | Main-window creation failed at `{stage}`. Reason: `{cause}`. |
| `JobApplicationCreationFailed` | JobApplications | Job creation failed at `{stage}`. Reason: `{cause}`. Rollback: `{rollback}`, file cleanup: `{cleanup}`. |
| `JobApplicationUpdateFailed` | JobApplications | Job update failed at `{stage}`. Reason: `{cause}`. Previous state: `{preservation}`, rollback: `{rollback}{cleanupDetail}`. |
| `CvSourceReadFailed` | CvLibrary | CV source `{stage}` failed during `{operation}`. Reason: `{cause}`. |
| `ManagedCvCopyFailed` | CvLibrary | Managed CV copy failed at `{stage}`. Reason: `{cause}`. Cleanup: `{cleanup}`. |
| `CvImportPersistenceFailed` | CvLibrary | Standalone CV import failed at `{stage}`. Reason: `{cause}`. Unreferenced managed file: `{fileOutcome}`. |
| `CvFavoriteSaveFailed` | CvLibrary | Could not save the CV favorite change. Reason: `{cause}`. |
| `CommittedChangePublicationFailed` | ApplicationModel | `{operation}` committed successfully. The displayed record could not be updated because `{cause}`. |
| `JobApplicationDeletionFailed` | JobApplications | Job deletion failed at `{stage}`. Reason: `{cause}`. Committed: `{committed}`, failed: `{failed}`. |
| `LinkedCvArchiveFailed` | CvLibrary | Could not archive a linked active CV. Reason: `{cause}`. |
| `ArchivedCvRestoreFailed` | CvLibrary | CV restoration failed at `{stage}`. Reason: `{cause}`. |
| `PermanentCvDeletionFailed` | CvLibrary | Permanent CV deletion failed at `{stage}`. Reason: `{cause}`. Original file restoration: `{restoration}`. |
| `ManagedCvOpenFailed` | CvLibrary | Opening the managed CV failed at `{stage}`. Reason: `{cause}`. |
| `InvalidManagedCvReferenceRejected` | CvLibrary | Refused CV `{action}` because the saved file reference was `{invalidReferenceKind}`. |
| `DatabaseRollbackFailed` | Database | Rollback failed during `{operation}`. Reason: `{cause}`. Database state is uncertain. |
| `RetainedCvFileRestoreFailed` | CvLibrary | CV deletion did not commit and its retained file could not be restored. Reason: `{cause}`. The database and managed files may be inconsistent. |
| `BackgroundWorkStartFailed` | BackgroundWork | Accepted `{operation}` could not start at `{stage}`. Reason: `{cause}`. Affected items: `{count}`. |
| `BackgroundWorkInvariantViolated` | BackgroundWork | `{operation}` encountered `{invariant}` during `{stage}`. |
| `BackgroundWorkDeliveryFailed` | BackgroundWork | `{operation}` delivery failed at `{deliveryStage}`. Persistence and visibility: `{verifiedOutcome}`. |
| `BackgroundShutdownCleanupFailed` | BackgroundWork | Shutdown could not complete `{cleanupStep}` for `{storageArea}`. Reason: `{cause}`. |

### WARNING events · 12

| `LogEventType` | Fixed component | Message template |
|---|---|---|
| `MutationBlockedByRemoval` | Storage | Declined `{action}` because `{blockingRemoval}` was active. `{repeatDetail}` |
| `RemovalBlockedByPendingWork` | Storage | Declined `{removalAction}` because `{blockingWork}` was pending or active. |
| `JobUpdateAlreadyPending` | JobApplications | Declined a job update because another update was pending. |
| `RequestedRecordStateMismatch` | Storage | Declined `{action}` because the requested record was `{observedState}`. |
| `CvImportSourceUnavailable` | CvLibrary | The accepted CV import found that its source was `{sourceCondition}` when processed. |
| `LinkedCvPermanentDeletionSkipped` | CvLibrary | Permanent deletion `{selectionOutcome}` because `{protectedCount}` selected CVs were linked to jobs. Skipped: `{skippedCount}`. |
| `DeletedCvFileAlreadyMissing` | CvLibrary | CV record deletion committed for `{count}` items whose managed files were already missing. |
| `DeletedCvFileCleanupPending` | CvLibrary | CV record deletion committed for `{count}` items. Final file cleanup failed and remains pending for startup recovery. |
| `FailedOperationFileCleanupIncomplete` | CvLibrary | Cleanup after `{operation}` failed to remove `{fileKind}`. Reason: `{cause}`. |
| `UnreferencedCvFilesQuarantined` | CvLibrary | Startup moved `{quarantined}` unreferenced files into quarantine. Retained CV files restored: `{restored}`, temporary files removed: `{removed}`. |
| `SavedFieldFallbacksApplied` | Storage | Loading continued with fallback values. Affected records by field category: `{fieldCounts}`. |
| `UnexpectedBackgroundResultDiscarded` | BackgroundWork | Discarded a result for `{operation}` during normal operation. Mismatch: `{mismatchKind}`. |

### INFO events · 24

| `LogEventType` | Fixed component | Message template |
|---|---|---|
| `ApplicationStartupBegan` | Application | JobTracker `{version}` started initialization on `{platform}`. |
| `ApplicationStarted` | Application | Startup completed and the main window is available. |
| `DatabaseSchemaInitialized` | Database | Initial database schema version `{version}` committed successfully. |
| `DatabaseUpgradeStarted` | Database | Database upgrade from version `{source}` to `{target}` began. |
| `DatabaseUpgradeCompleted` | Database | Database upgrade from version `{source}` to `{target}` committed after relationship checks passed. |
| `ManagedCvFilesRecovered` | CvLibrary | Startup restored `{restored}` retained CV files and removed `{removed}` obsolete temporary files. |
| `BackgroundSaveOrImportAccepted` | BackgroundWork | Accepted `{operation}` for background processing. Admitted items: `{count}`. |
| `JobApplicationCreated` | JobApplications | Job creation committed. CV outcome: `{cvDisposition}`. |
| `JobApplicationUpdated` | JobApplications | Job update committed. CV replacement: `{replacement}`, relationship changed: `{relationshipChanged}{cvDetail}`. |
| `StandaloneCvImported` | CvLibrary | Standalone import committed a new managed CV. |
| `ActiveCvReusedOnImport` | CvLibrary | Standalone import reused an existing active CV without adding another copy. |
| `ArchivedCvRestoredOnImport` | CvLibrary | Standalone import restored a matching archived CV to the active library. |
| `RemovalOperationAccepted` | DataRemoval | Accepted `{operation}` for `{selected}` selected records. |
| `JobApplicationsDeleted` | JobApplications | Job deletion committed for `{count}` applications. |
| `LinkedActiveCvsArchived` | CvLibrary | Archived `{count}` linked active CVs. |
| `UnlinkedCvsPermanentlyDeleted` | CvLibrary | Permanently deleted `{count}` eligible CVs and completed physical cleanup. |
| `ArchivedCvsRestored` | CvLibrary | Restored `{count}` archived CVs to the active library. |
| `PendingWorkCancellationRequested` | BackgroundWork | Requested cancellation of `{activeCount}` active operations and removed `{queuedCount}` queued requests. |
| `OperationStoppedByCancellation` | BackgroundWork | `{operation}` stopped at `{boundary}` because cancellation was requested. `{completionCounts}` |
| `OperationCommittedAfterCancellationRequest` | BackgroundWork | `{operation}` committed after cancellation had been requested. `{committedOutcomeDetail}` |
| `CloseDeferredByUser` | Application | The user selected Wait while work was pending. The application remains open. |
| `InterruptAndExitSelected` | Application | The user selected Interrupt and Exit and new work was blocked. Queued requests removed: `{queuedCount}`, active operations asked to stop: `{activeCount}`. |
| `ControlledShutdownStarted` | Application | Controlled shutdown began. Pending work: `{pendingCounts}`. |
| `ControlledShutdownCompleted` | Application | Controlled shutdown and required background cleanup completed. |

## 2. Queue, consumer, and file writer

- **Producer boundary.** Expose `ApplicationLog::record(LogEvent) noexcept`. Capture UTC event time before acquiring the queue mutex. Build records from bounded values so diagnostic preparation cannot throw into a business operation before `record()` is entered. Submission before initialization or after admission closes is a silent no-op.
- **Queue.** Use `std::deque<LogEntry>`, `std::mutex`, and `std::condition_variable`. The limit is **4,096 entries**. As selected, a producer briefly waits for an ordinary queue critical section. At capacity, discard the incoming entry at every severity. Preserve accepted entries in FIFO enqueue order. No overflow buffer or severity reservations.
- **Consumer.** One `std::jthread` waits on `closing || !queue.empty()`. Under the mutex it removes one entry, then unlocks before formatting, writing, flushing, or rotating. It immediately processes the next available entry. It sleeps when idle and exits only when admission is closed and the queue is empty. Set the closing flag and notify explicitly.
- **File format.** The consumer writes one UTF-8 line per event using `yyyy-MM-dd HH:mm:ss.zzz | SEVERITY | Component | Message: ...`. The message and reason together contain at most three sentences. Timestamps can appear out of order across producers. Event names are internal and the line contains no event code, PID, session identifier, or sequence number.
- **Location and retention.** Resolve `QStandardPaths::writableLocation(QStandardPaths::StateLocation)/Logs` after setting the application name. Use numbered segments such as `jobtracker-000001.log`. Resume the latest usable segment across normal restarts. Rotate before the next encoded line, including its newline, would exceed **5 MiB**. Prune the oldest eligible closed segments only when required to keep recognized log files within **50 MiB**. Ignore unrelated files and symlinks. There is no age-based deletion or interprocess coordination. Simultaneous JobTracker instances have undefined logging behavior.
- **Write and flush failures.** Continue a positive short write from its returned offset. On zero progress or error, discard that record and let the next record make an ordinary attempt. If an incomplete final line remains, close that segment and continue in a new one. Flush after ERROR, before sleeping with dirty output, before rotation, and at shutdown. During sustained traffic, check a one-second flush deadline between writes. This policy introduces no deliberate write delay.
- **Isolation.** Only the consumer owns the `QFile`. The logger never acquires the application’s CV mutex, uses a SQL connection, calls back into QML, or waits on the GUI. Directory, open, write, flush, prune, and rotation failures are silent. Consumer startup failure leaves logging disabled. Use typed submission directly, without a global Qt message handler or Qt warning macros for operational events.

## 3. Integrate logging where JobTracker knows the outcome

| Integration point | Implementation rule |
|---|---|
| **Startup, storage, and recovery** | Start logging before constructing `AppBootstrap`, which already opens storage, loads records, and reconciles managed CVs. Record `ApplicationStarted` only after the root window exists. Capture migration versions before they change and record success after commit. Capture recovery counts while the report still exists, including partial progress on failure. Use `UnreferencedCvFilesQuarantined` for combined successful recovery with quarantine. [Bootstrap initialization](../src/app/AppBootstrap.cpp#L43) · [Root-window check](../src/app/AppBootstrap.cpp#L149) · [Schema commit](../src/storage/SchemaMigrator.cpp#L95) |
| **Job saves and CV imports** | The owner of the transaction emits its outcome after commit or after failure handling finishes. A CV failure inside a job save belongs to the job incident, with the specific CV stage and cause. Standalone imports use their own source, copy, persistence, and disposition types. Separate an unavailable source from a file that failed during reading. A selected replacement can resolve to the same CV, so record replacement and relationship change separately. [Job creation](../src/jobs/AddJobService.cpp#L126) · [Job update](../src/jobs/UpdateJobService.cpp#L120) · [Standalone import](../src/cvs/CvImportExecutor.cpp#L124) |
| **Rollback, restoration, and cleanup** | Capture the results of the **existing** rollback, remove, and rename calls. Preserve their execution order. Job-save cleanup can run after rollback, while a deletion guard can attempt file restoration before its caller rolls back. Distinguish `not attempted`, `succeeded`, `failed`, and `unknown`. Emit a correlated consequence record for a separately verified rollback or restoration failure. A shared preparation object must never retain a pointer to an expired stack observation. [Transaction guard](../src/storage/SqlTransaction.cpp#L22) · [Managed-file guards](../src/cvs/CvManagedFileStore.cpp#L73) |
| **Removal and managed-file access** | Aggregate verified outcomes for the selected batch. Count protected CVs separately from job links. Keep clean permanent deletion, committed deletion with a missing file, and committed deletion with pending cleanup distinct. Obtain invalid-reference categories from the resolver. An OS handoff result says only whether the open request was accepted. [Removal outcomes](../src/maintenance/DataRemovalService.cpp#L120) · [Path validation](../src/cvs/CvManagedPathResolver.cpp#L49) · [OS handoff](../src/cvs/CvFileAccessService.cpp#L15) |
| **Worker delivery and GUI publication** | Record committed persistence on the worker before posting to the GUI. Inspect existing request/result posting returns for `BackgroundWorkDeliveryFailed`, without retrying or changing the business result. Emit `CommittedChangePublicationFailed` only when a confirmed save cannot be reflected in the model. An expected callback discard during shutdown generates no stale-result warning. [Queued result delivery](../src/common/SingleActiveWorkerFacade.hpp#L185) · [Shutdown filtering](../src/common/SingleActiveWorkerRuntime.cpp#L51) · [Favorite publication](../src/cvs/CvLibraryController.cpp#L521) |
| **Admission, cancellation, and close choices** | Observe explicit attempts and actual admissions. Exclude button availability checks and dialog openings. Use the existing cancellation boundaries and counters. A flag first read after commit cannot establish that cancellation preceded commit. When that ordering is verified, use `OperationCommittedAfterCancellationRequest` with the ordinary success details in one record. Observe Wait and Interrupt and Exit at their actual QML actions through two semantic signals. Combine the interrupt, cancellation-request, and shutdown-start facts for that transition. [Close-choice handlers](../qml/Main.qml#L162) |

A specific failure takes precedence over a generic parent report. For example, a worker database-open failure produces one `DatabaseConnectionSetupFailed` record with the affected operation and item count. A later failed rollback or failed GUI publication is a separate verified outcome and remains independently reportable.

## 4. Lifecycle and implementation order

1. **Implement the catalog, formatter, inbox, and writer.** Keep one process-local inbox alive throughout all producer calls. `ApplicationLogSession` owns the consumer lifetime. Open admission only after the consumer starts successfully.
2. **Wire the session into `App::start`.** Create it after application naming and before bootstrap construction. Keep startup exception classification inside its lifetime. Preserve the original exit paths. Logger directory setup must be independent of `StoragePaths`, whose construction can itself fail. [Application entry](../src/app/App.cpp#L29) · [Business-data paths](../src/storage/StoragePaths.cpp#L8)
3. **Instrument in dependency order.** Start with storage, migration, and recovery. Follow with job/CV persistence and removal, then worker delivery, controller publication, and close choices. Replace overlapping recovery and rollback diagnostic calls at their owners. Keep diagnostic metadata private to the affected boundaries.
4. **Drain on normal shutdown.** Keep admission open while bootstrap stops and joins the job-save, CV-import, and removal workers. Emit `ControlledShutdownCompleted` only after verified cleanup. Close admission under the queue mutex, notify, drain accepted entries individually, attempt the final flush, close the file, and join without holding the mutex. Processing an accepted entry guarantees a write attempt. Forced termination may lose pending or unflushed records. [Bootstrap teardown](../src/app/AppBootstrap.cpp#L110)
5. **Register and document.** Register the production logging module and link the targets that compile instrumented sources. Keep Qt Test dependencies in the tests-only configuration. After implementation stabilizes, document the event catalog contract, queue policy, file policy, and lifecycle in a focused architecture document and link it from the architecture overview. Keep the supplied event specification unchanged.

## 5. Verification and acceptance

| Area | Acceptance check |
|---|---|
| **Catalog completeness** | Check that all 62 descriptive `LogEventType` values have exactly one descriptor, with 26 ERROR, 12 WARNING, and 24 INFO entries. Reject missing or duplicate descriptors and empty components or templates. Test each rendered event for required diagnostic facts, approved substitutions, one physical line, and the three-sentence limit. |
| **Concurrency and shutdown** | Fill the inbox deterministically, submit another event of each severity, and verify FIFO contents are intact. With concurrent producers below capacity, verify complete lines and each producer’s known order. Exercise idle shutdown, shutdown with pending entries, and a producer racing admission closure. |
| **Files and privacy** | Use temporary storage to test the exact byte boundary, restart append, rotation, 50 MiB pruning, incomplete tails, and preservation of unrelated files. Seed private strings in domain inputs and verify they never appear in output. |
| **Failure isolation** | Run representative job, CV, removal, and cancellation scenarios with logging unavailable, writable, and unwritable. Compare database state, managed files, return values, model signals, notifications, and exit behavior. Restore a previously invalid log-directory path and verify a subsequent record can be written. |
| **Verified outcomes** | Exercise failed rollback, failed file restoration, recovery failure after partial progress, committed deletion with cleanup pending, and a worker commit whose GUI callback is withheld. Verify the domain outcome alongside the exact diagnostic. Exercise the real close choices, including automatic dialog dismissal, which must emit neither user-choice event. |
| **Project checks** | On Windows Debug, build `JobTrackerApp` with tests OFF, build `JobTrackerTests` in the separate tests-ON tree, run CTest, then build `JobTrackerApp_dev_runtime`. Review synchronization, guard lifetime, and transaction behavior independently. Record unexecuted native Linux or fault scenarios explicitly. [Project verification workflow](../For-Agent/Docs/build.md) |