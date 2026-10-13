# Threading And Asynchronous Work

Read this document for worker ownership, thread affinity, queued delivery,
cancellation, shutdown, or cross-worker mutation serialization.

## Worker Topology

`SingleActiveWorkerRuntime` owns the common mechanics for one reusable worker
thread: lazy executor creation, busy and shutdown state, active-operation and
cancellation correlation, stale-result rejection, worker-thread context
destruction, and quit/wait handling. `SingleActiveWorkerFacade<Executor>` builds
on that runtime for typed one-operation facades, standardizing request admission,
queued value-outcome delivery, and infrastructure-failure translation.
`JobSaveWorker` and `CvImportWorker` use its executor-factory constructor to
capture the same non-owning `CvLockWrapper&` supplied by bootstrap and pass it to
their lazily created executors.

`JobSaveWorker`, `CvImportWorker`, and `DataRemovalWorker` derive from that
facade, using `JobSaveExecutor`, `CvImportExecutor`, and `DataRemovalExecutor`
respectively. Executors return value outcomes; the shared facade owns request
admission and queued request/result delivery. The concrete workers retain their
domain requests, outcomes, signals, pipelines, and messages.

`JobSaveWorker` is shared by job creation and job updates. On its first submitted
request its runtime starts the dedicated thread. The GUI controller maps form
values and runs storage-independent canonical preflight synchronously. For a
validated create request, the worker lazily constructs a private `StoragePaths`,
`SqliteDatabase`, repository, managed-file, import-service, `AddJobService`, and
`UpdateJobService` graph inside that thread. Production owns no GUI-thread job
persistence service.

`CvImportWorker` is a separate facade for standalone CV Library imports. Its
runtime reuses one dedicated thread, and its executor lazily constructs a private
`StoragePaths`, `SqliteDatabase`, `CvRepository`, `CvManagedFileStore`, and
`CvImportService` graph on that thread. `CvLibraryController` owns the import FIFO
and receives only value outcomes for GUI-thread model publication.

`DataRemovalWorker` is the shared GUI-thread facade for job deletion and CV
archive, restore, and permanent-deletion batches. Its reusable
`DataRemovalExecutor` owns a private SQLite graph on its worker thread and
returns value-only per-item outcomes. Its header-defined `process<Kind>()`
obtains the lazily constructed service through `ensureService()`; the persistence
context remains private to the executor implementation file.

Removal requests and outcomes retain their `DataRemovalKind` template argument
in their concrete types. The shared facade wraps a returned result in the
executor's four-alternative `Outcome` variant for queued delivery to the GUI
thread. The worker exposes four concrete typed completion signals and registers
their outcome metatypes; its QObject class and signals remain non-templated.
Its outcome handler uses `std::visit` and compile-time dispatch for both accepted
and unavailable outcomes. Request traits and executor exception handling share
the same correlated failed-batch builder. Accepted outcomes first pass runtime
operation/token correlation and complete the active worker operation.
Busy/shutdown rejection and executor-start failure outcomes are queued directly
for publication and must never complete or replace another active operation.
Controller correlation and GUI publication still precede release of removal
admission.

## Cross-Worker Mutation Coordination

The application-owned `CvLockWrapper` contains one `std::mutex`, exposed by
`getMutex()`. `AddJobService`, `UpdateJobService`, and `CvImportExecutor` acquire
it with `std::unique_lock` before their persistence transactions, serializing job
save and standalone CV-import persistence across the two worker threads.

This lock has no condition variable, ticket FIFO, cancellation-aware wait, or
custom lease type. Acquisition order is the platform mutex's unspecified order,
and a worker blocked in `lock()` cannot observe cancellation until it acquires
the mutex. Create, replacement-CV, and standalone import paths then recheck the
token before starting their transaction. All job updates currently take the
lock, including metadata-only updates; the metadata-only path has no second
post-acquisition cancellation check.

The `std::unique_lock` spans identity lookup when applicable, transaction commit
or rollback, and catch-path file cleanup. Bootstrap declares `CvLockWrapper`
before both workers so it outlives them. This is process-local serialization;
the SQL unique constraint still protects against other-process identity races.

`StorageMutationGate` admits imports/updates together and grants exclusive
ownership to one deletion, manual archive, or manual restore batch. Its shared
import/update count includes accepted job-save and Add CV queues plus scoped
synchronous favorite writes. Controllers reserve before enqueueing or writing
and release after persistence, compensation, GUI model/cross-controller
publication, completion signals, and pending-state publication. Queue advancement
cannot open a gap between accepted items. Removing waiting items releases only
their reservations; active cancellation retains admission until its outcome.

Every rejected attempt during normal operation emits a typed `operationBlocked`
event. The gate retains only retry-notice interest, never a rejected payload or
automatic retry. Pure availability queries do not report attempts; explicit
checks before confirmation or the picker report conflicts but reserve nothing.
Final commands check again. Once shutdown begins, admission closes and rejected
attempts are silent. Bootstrap joins all workers before controller teardown
releases outstanding reservations, and destroys QML before the exposed backend
graph.

`DataRemovalWorker` does not acquire `CvLockWrapper`; exclusion with removal is
provided by this GUI-thread admission gate instead. Automatic archived-CV
restoration inside an admitted import remains part of that import.
The domain-specific acquisition and release behavior is documented in
[jobs.md](jobs.md), [cvs.md](cvs.md), and [removal.md](removal.md).

## Coordination Test Coverage

The fixtures and registered `JobTrackerCvLockTests` use `CvLockWrapper` and
`std::unique_lock`; they do not require FIFO lock acquisition or cancellation of
a mutex wait. Gate and controller/integration tests cover repeated rejection,
mixed queues, cancellation, publication ordering, and notice/shutdown state.
The offscreen QML feedback suite covers dialog reopening and live banner state.

## Threading Rules

- Keep QML-facing controllers, Qt models, and their mutations on the GUI thread.
- Return worker results through queued delivery and apply model or property
  changes on the owning thread.
- Define worker ownership, cancellation, shutdown, and late-result handling
  before moving work off the GUI thread.
- Use `SingleActiveWorkerRuntime` for the shared single-executor thread,
  cancellation, correlation, and shutdown mechanics while keeping domain
  requests, outcomes, pipelines, and signals in their typed worker facades.
- Create, use, and close each Qt SQL connection in one thread. Do not share
  `QSqlDatabase` connections or active `QSqlQuery` objects across threads.
- Job create/update form mapping and canonical preflight run synchronously in the
  GUI controller and remain pure and storage-independent. The active worker path
  consumes the resulting normalized draft through `prepareValidated()`; checked
  service entry points retain validation for other callers. Managed CV work,
  duplicate lookup, repositories, and transactions run on the dedicated
  job-save worker and its private SQLite connection. Only value outcomes return
  to the GUI thread.
- Keep job-save model mutation and QML/cross-screen signal publication in the GUI
  controller. Destroy the worker persistence graph on its own thread before
  quitting and waiting for that thread; never terminate it.
- Run deletion/archive batches on the dedicated removal executor with its own
  SQLite connection. Check cooperative cancellation only between items and
  return value outcomes for all GUI-thread model mutations.
