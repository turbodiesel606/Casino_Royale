# CV Import And Library Behavior

Read this document for Add CV, CV identity/reuse/restore, import publication, or
CV import cancellation. Read [storage.md](storage.md) for managed-file and
repository primitives, [threading.md](threading.md) for the shared mutation lock
and worker lifecycle, and [removal.md](removal.md) for archive and permanent
deletion behavior.

## Shared CV Import Service

`CvImportService` connects a worker's managed-file store to its private
`CvRepository`. It prepares the file, then resolves the existing composite CV
identity on the same worker thread. Active rows return unchanged; archived rows
always restore, reporting `RestoredArchived`. Neither duplicate path creates a
managed name or file. Only a missing identity stages and inserts a CV.

Preparation and stage failures are structured results; SQL failures propagate to
the workflow transaction boundary, which rolls back before file cleanup. Add
Job, Add CV, and replacement updates all hold the shared mutex through that
cleanup. Cancellation is observed while preparing the source and at explicit
pre-persistence checks, but the import/staging methods do not consult it after
persistence begins. A process crash after finalization but before commit can
leave a completed orphan that startup recovery quarantines.

## Add CV Flow

The sidebar Add CV action opens one `FileDialog` in multi-file mode. QML sends
the ordered local URL list to `CvLibraryController::addCvs()` and stays on the
current page. The dialog filters for PDF, DOC, and DOCX, while the managed-file
store remains the defensive validation boundary for local, readable, supported
files.

`CvLibraryController` composes `SerialOperationQueue<QueuedCvImport>` for its
import FIFO. The queue assigns monotonically increasing operation IDs and owns
one active request, waiting URLs, the active cancellation identity, completion
suppression, correlation checks, and drained transitions. The controller keeps
domain admission, worker submission, mutation-gate coordination, signals, and
model publication. `pendingImportCount` is the active-plus-waiting total,
`importing` is true exactly while it is non-zero, and later picker batches append
behind existing work. Duplicate or failed requests release only their own active
slot and never block later imports.

For each active request, the serial queue creates a shared `CancellationState`,
and the controller submits it to `CvImportWorker`. The worker performs validation
and buffering, blocks on the shared `CvLockWrapper` mutex, rechecks cancellation,
begins its CV-import transaction, and resolves the exact case-sensitive
`(sha256, original_file_name)` identity on its private thread and connection.
Only missing identities stage, finalize, and insert a new file/row. Archived
duplicates restore. Same filename with different content and same content with a
different filename remain distinct CVs. Commit or rollback/file cleanup
completes before lock release, GUI publication, and controller FIFO advancement.

The controller publishes successful outcomes idempotently by stable CV ID and
mutates `CvListModel` only on the GUI thread. This keeps CV Library and Dashboard
projections live and tolerates overlapping Add Job `cvUsed` publication without
duplicate rows or missing links. Per-file QML outcomes distinguish inserted,
restored, duplicate, and failed states for global notifications.

`cancelAllCvImports()` drops waiting URLs, requests cancellation of the active
operation, suppresses its exit-time notification, and emits
`importQueueDrained` after cleanup. Buffering observes cancellation between read
chunks. A worker already blocked on `CvLockWrapper` cannot be awakened by the
token, but checks it after acquiring the mutex and returns before the
transaction. Staging and import persistence currently ignore later cancellation
and must complete or roll back and compensate. Worker shutdown destroys its SQL
graph on the worker thread before quitting and waiting.
