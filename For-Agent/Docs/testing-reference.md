# Testing Reference

Use [testing.md](testing.md) for test selection, quality, isolation, and outcomes.
Read only the details needed for a selected target or feature:
search [Suite Responsibilities](#test-suite-responsibilities) for its exact
target name from the [test map](testing.md#subsystem-to-test-map), or use
[fixtures and diagnostic logs](#fixtures-and-diagnostic-logs),
[admission/feedback checks](#admission-and-feedback), or
[CV-import checks](#cv-import).

## Test Suite Responsibilities

- `JobTrackerCommonTests` covers shared filtering, limited sorting, and canonical job validation.
- `JobTrackerRelationProxyTests` covers relation filtering and every relevant source-model mutation.
- `JobTrackerSelectionTrackerTests` covers stable-ID selection, fallback behavior, and precise change reporting.
- `JobTrackerAsyncInfrastructureTests` covers serial FIFO operation IDs,
  activation, cancellation, completion suppression, stale-result correlation,
  drained state, lazy worker-executor creation, initialization failure, busy and
  shutdown state, worker-thread context destruction, and quit/wait cleanup.
- `JobTrackerJobsTests`, `JobTrackerCvTests`, `JobTrackerDashboardTests`, and
  `JobTrackerDirectoryTests` retain QML-facing
  contracts and controller-specific side effects. The jobs controller suite
  verifies synchronous canonical preflight, immediate complete field errors,
  no queue or worker state for invalid input, first-valid operation-ID
  allocation, normalized value capture before form reset, update rejection,
  in-place role publication, failure isolation, replacement-CV publication,
  and create/update FIFO ordering and state. The CV controller
  suite verifies multi-batch FIFO ordering, pending-state transitions,
  duplicate and failure isolation, database-lock recovery, cancel-all behavior,
  idempotent Add Job/CV publication, and worker SQL connection cleanup.
  It also verifies restored archive roles before Add Job/replacement links,
  removal of the old link before publishing existing/new replacements, same-ID
  replacement restoration, standalone restoration, and continuation after a CV
  insert failure with finalized-file cleanup.
- `JobTrackerMigrationTests` covers schema initialization, supported upgrades,
  rollback, foreign keys, reopen, unsupported newer versions, and named-connection
  cleanup when `SqliteDatabase` construction fails.
- `JobTrackerRepositoryTests` covers repository persistence plus shared SQL error and transaction infrastructure.
- `JobTrackerCvImportTests` covers exact buffered bytes/hash/size with no early
  managed names or files, staging the snapshot after source removal, exact
  case-sensitive identity, duplicate no-file behavior, archived restoration,
  structured failures, cancellation before buffering, complete persistence with
  a canceled token after buffering, final bytes without leftover `.part` files,
  and startup recovery.
- `JobTrackerCvLockTests` covers a canceled waiter remaining blocked until the
  active mutex owner releases the lock. It makes no FIFO or cancellable-lock
  guarantee. A test-only timeout bounds synchronization regressions.
- `JobTrackerMutationGateTests` covers mixed counted batches, one exclusive
  removal, every rejected attempt, non-reserving checks, live retry-notice
  state, dismissal, fresh admission, and shutdown suppression.
- `JobTrackerMutationFeedbackTests` runs the shared QML component offscreen at
  the minimum window width, checks dialog reopening and live banner state, and
  rejects QML runtime warnings. Its Qt Quick test dependency stays inside the
  test-enabled configuration; see [Windows test-runtime details](build-windows.md#test-runtime)
  for offscreen platform plugin and QML dependency deployment.
- `JobTrackerRemovalTests` covers job/CV deletion, archive/restore, linked-CV
  protection, best-effort outcomes, managed paths/tombstones, and worker cleanup.
  Compile-time checks cover supported request/outcome kinds, constant operation
  identity, shared-facade request compatibility, typed executor return values,
  and rejection of unsupported template arguments. Worker cases cover
  all four typed signals and Qt metatype extraction, operation/token correlation,
  empty and pre-canceled batches, item/context failures, shutdown rejection,
  and repeated busy rejections without disturbing an accepted database operation.
  They also cover completion-handler submission of the next batch, context
  initialization retry, null cancellation tokens, lazy connection creation and
  reuse, and suppression of pending accepted outcomes after idempotent shutdown.
  The jobs and CV controller suites also verify typed routing, stale same-kind
  and foreign completions, idle rejection, selection preservation, and admission
  held through completion publication.
- `JobTrackerAddJobTests` covers the canonical preflight contract used by Add
  Job, defensive service pre-staging validation, cancellation immediately before
  a transaction, archived restoration and its rollback, cleanup when CV/job
  insertion fails, cancellation after final rename still committing, transaction
  behavior, and durable orchestration.
- `JobTrackerUpdateJobTests` covers in-place metadata and ordered-technology
  updates, company creation and normalized reuse, replacement-CV insertion,
  active duplicate reuse, archived restoration including same-ID replacement,
  metadata-only CV preservation, old-CV preservation, restoration rollback and file
  cleanup, missing targets, defensive invalid-input rejection, and cancellation
  before the update transaction.
- `JobTrackerIntegrationTests` covers synchronous invalid-input rejection
  without queue, filesystem, or SQLite mutation; normalized controller FIFO
  admission; exact queued/final event ordering; worker-thread connection
  initialization and retry through final failure outcomes; FIFO persistence
  order; pending-count and saving transitions; CV and database-lock failure
  isolation; duplicate-CV cleanup; cooperative active and cancel-all behavior;
  GUI event-loop responsiveness; shutdown cleanup; and restart hydration.
  Cross-worker cases concurrently import the same exact identity through Add
  Job and Add CV in both submission orders: one inserts, one reuses, and one CV
  row/file remains without `.part` files. Job rollback cases verify the other
  worker can still import the identity without orphaned files or partial jobs.
  Gate cases cover both controller queues, completion-handler attempts to start
  conflicting work, failures, active cancellation, and metadata updates waiting
  for the shared CV mutex.

## Fixtures And Diagnostic Logs

Storage-oriented suites reuse the temporary database and filesystem fixtures under `tests/support` so setup and cleanup rules remain consistent.

All 18 suites use `jobtracker_register_test` to retain verbose Qt Test diagnostics
in `<build-directory>/test-logs/<configuration>/<test-target>.txt` as well as
requesting console output. The CMake runner forwards captured output and, when
stdout is empty, prints this invocation's text log to CTest. Configure creates
writable log directories; test names
and configurations have separate files. Each invocation replaces that test's
previous text log, so retain a copy when comparing runs. Use this file when a
Windows host supplies an empty CTest failure log. A failure before Qt Test
initialization may produce no new Qt Test log: the runner says so and leaves any
previous log unchanged. Retain CTest/process diagnostics
(`Testing/Temporary/LastTest.log`, console output and exit status) as well.
Captured process output is forwarded as it arrives. The runner prints the unique
invocation log path before starting Qt Test; retain that partial file if CTest
terminates the process on timeout before the normal log replacement.
The QML feedback test uses the ordinary CTest command for automatic incremental
[runtime preparation](build-windows.md#test-runtime).

## Manual Checks

Apply the [manual-check policy](testing.md#manual-checks) and use only the
scenario relevant to the changed feature.

### Admission And Feedback

For admission/feedback changes, verify repeated blocked actions, rejection from
the unsaved-changes Save flow, native multi-file picker selection restoration,
confirmation-time rechecks and focus, long/two-category banner text beside
ordinary notifications at 1180 x 720, and Wait/Interrupt-and-Exit with feedback
visible. Offscreen component tests do not verify native picker behavior.

### CV Import

For CV-import changes, manually exercise overlapping Add Job/Add CV requests,
archived duplicate restoration (including same-ID replacement), and cancel/close
with a large selected file. Real disk-full/permission failures, abrupt process
termination, Linux runtime behavior, and other-process races need separate
environment-specific verification. Full-file buffering intentionally increases
peak memory; no new CV size limit or streaming fallback is introduced.
