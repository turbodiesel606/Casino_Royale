# Testing Policy

Run tests when a change affects business logic, storage, parsing, algorithms,
cross-module contracts or high-risk runtime behavior. Do not add formal tests
only for appearance-level QML changes when no suitable infrastructure exists.
Do not claim code works unless verified.

For documentation/instruction-only changes that do not affect build behavior,
check links, command forms, preset/target/script contracts and evidence labels;
do not rebuild unchanged C++ only for prose edits. For normal application
configuration, builds, development runtime preparation, and backend tests, use
[build.md](build.md). Standalone deployment and installer verification are
documented separately in [installer-testing.md](installer-testing.md).

## Production And Test Isolation

Tests are consumers of the production architecture, never production dependencies. Keep the dependency direction strictly one-way: `tests -> production`.

- Production code under `src/`, production targets, and production APIs must not include, link, reference, or conditionally depend on test sources, test targets, Qt Test, fixtures, mocks, test-only compile definitions, or test-only hooks.
- Do not add or widen production APIs, weaken access control, or alter production ownership, lifetime, threading, behavior, or architectural boundaries solely to make tests easier to write.
- Exercise existing production contracts. Keep test fixtures, mocks, fakes, helpers, and other test support under `tests/`.
- With `JOBTRACKER_BUILD_TESTS=OFF`, normal configure, build, and deployment must not require test-only packages, create or register test targets, compile test sources, deploy test dependencies, or change the `JobTrackerApp` artifact.
- Test-enabled builds may depend on and compile production code. Production code and normal builds must remain unchanged by whether tests are enabled.

## Current Test Setup

Tests are enabled through the `JOBTRACKER_BUILD_TESTS` CMake option and the
Windows Debug/Release test presets (`windows-debug-tests-local` and
`windows-release-tests-local`). Both require Qt 6.10.3 and use separate build
directories. Linux Debug and Release test presets (`linux-debug-tests-local`
and `linux-release-tests-local`) use the same SDK pin, distinct directories and
matching single-config build types. Use the Linux commands in `build.md` on a
Linux host; preset inspection on Windows does not verify Linux execution.

The Windows test runtime helper copies the configuration-matched Qt libraries
and SQLite driver into each test output directory (`sqldrivers/qsqlite[d].dll`).
QtSql loads SQLite dynamically; linking/copying QtSql alone does not supply the
driver. This preparation remains inside `JOBTRACKER_BUILD_TESTS` and does not
deploy the production application or create a Release staging directory.

For normal application configuration, builds, development runtime preparation,
and backend tests, use the commands in `For-Agent/Docs/build.md`.

## Test Outcomes

Report the selected preset/configuration/filter, the number of tests actually
executed, and one of these outcomes:

- `passed`: the selected tests ran and all passed.
- `failed`: a CTest run reported failures; distinguish assertion failures from
  crashes, timeouts, or test-start/runtime errors, and count only tests that
  actually executed.
- `not run`: tests were not executed; state why and what remains unverified.
- `no matching tests`: selection found zero tests. This is neither a passing
  suite nor an executed failing test, even if CTest exits successfully.

If tests were expected but none matched, verification is incomplete. Inspect the
selected preset, test-enabled configuration, CMake registration, and filter before
claiming success. Report configuration or build failures separately from test
assertion failures; tests blocked by those failures are `not run`.

## Test Scope

Choose test scope from the final diff and affected contracts. For an ordinary
bounded change, start with the directly associated targets or CTest selections
from the map below, using the canonical command forms in `build.md`. Substitute
verified preset names, target names, and filters as appropriate; the examples
are not an exhaustive list of permitted targets.

Use the full relevant test preset when schema or migrations, storage semantics,
worker/runtime or concurrency behavior, shared infrastructure, several
subsystems, cross-module contracts, unexpected targeted-test failures, or
merge/release readiness requires broader verification.

Do not rerun the same successful test scope without a concrete reason. A later source change, a newly visible risk, a broader verification question, or diagnosis of an unexpected result is a concrete reason. Do not leave risky behavior unverified merely to reduce execution time or usage.

## Backend Test Patterns

Use Qt Test for QObject, model, and signal behavior.

Use `QSignalSpy` when verifying property notify signals, command result signals, or model update signals.

For `QAbstractListModel` subclasses, test:

- Role names.
- Row counts.
- Data returned for each role.
- Insert, remove, reset, and update behavior when applicable.

For services and domain logic, test validation rules, parsing, filtering, sorting, grouping, search, and error handling.

For storage or file-system behavior, use temporary test data and avoid depending on machine-specific paths.

Name tests by the behavior they cover, and keep them registered through the project CMake test setup.

## Regression-Test Quality

- Verify an observable contract or meaningful invariant with a test capable of
  detecting the relevant defect. Avoid tests that mirror implementation steps,
  assert incidental internals, or pass without exercising the changed behavior.
- For a bug fix, demonstrate failure before the fix and success afterward when
  feasible and proportionate. If that demonstration is unavailable, state the
  limitation; do not manufacture ceremonial tests for every small task.
- Cover important failure and boundary cases when the changed contract and risk
  justify them. Reuse suitable existing coverage instead of adding redundant tests.
- For concurrency, lifetime, cancellation, and shutdown, verify ordering,
  completion, ownership, and termination through observable synchronization and
  outcomes. Bounded timeouts may prevent hangs; arbitrary waits alone do not
  establish the contract.
- Tests consume production contracts. Test quality never justifies changing
  production APIs, access control, ownership, threading, or architecture solely
  to accommodate a test.

## Subsystem-To-Test Map

These target and CTest names match the root `CMakeLists.txt`; sources are relative
to `tests/`. This is an initial selection aid, not a claim that one suite covers
every change in a subsystem. Inspect relevant test cases and expand under Test
Scope above. Static test-source inspection does not establish a passing run.

| Subsystem or contract | Build target / CTest name | Test source |
| --- | --- | --- |
| Shared filtering, sorting, job validation | `JobTrackerCommonTests` | `common/FilterAndValidationTest.cpp` |
| Relation proxy and source-model mutations | `JobTrackerRelationProxyTests` | `common/RelationFilterProxyModelTest.cpp` |
| Stable-ID and bulk selection | `JobTrackerSelectionTrackerTests` | `common/StableIdSelectionTrackerTest.cpp` |
| FIFO, cancellation, worker lifecycle and shutdown | `JobTrackerAsyncInfrastructureTests` | `common/AsyncInfrastructureTest.cpp` |
| Jobs controller, roles, selection, preflight, updates and removal publication | `JobTrackerJobsTests` | `jobs/JobApplicationsControllerTest.cpp` |
| CV controller, selection, import queue and archive/removal publication | `JobTrackerCvTests` | `cvs/CvLibraryControllerTest.cpp` |
| Dashboard metrics, funnel, recent rows and refresh | `JobTrackerDashboardTests` | `dashboard/DashboardControllerTest.cpp` |
| Company/contact models, selection, linked rows and updates | `JobTrackerDirectoryTests` | `directory/DirectoryControllerTest.cpp` |
| Schema initialization, upgrades, rollback and connection cleanup | `JobTrackerMigrationTests` | `storage/MigrationTest.cpp` |
| Repository persistence, SQL errors and transactions | `JobTrackerRepositoryTests` | `storage/RepositoryTest.cpp` |
| CV buffered import, identity, files, cancellation and recovery | `JobTrackerCvImportTests` | `storage/CvFileStoreImportTest.cpp` |
| CV mutex waiter/cancellation contract | `JobTrackerCvLockTests` | `cvs/CvLockWrapperTest.cpp` |
| Add Job validation and transactional orchestration | `JobTrackerAddJobTests` | `storage/AddJobServiceTest.cpp` |
| Update Job metadata, company and replacement-CV transactions | `JobTrackerUpdateJobTests` | `storage/UpdateJobServiceTest.cpp` |
| Typed job/CV removal, archive/restore and worker cleanup | `JobTrackerRemovalTests` | `storage/DataRemovalServiceTest.cpp` |
| Mutation admission, reservations and retry notices | `JobTrackerMutationGateTests` | `maintenance/StorageMutationGateTest.cpp` |
| Offscreen QML mutation feedback and runtime warnings | `JobTrackerMutationFeedbackTests` | `maintenance/StorageMutationFeedbackTest.cpp` |
| Controller/worker/storage integration, recovery and shutdown | `JobTrackerIntegrationTests` | `storage/EndToEndIntegrationTest.cpp` |

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
  test-enabled configuration, including Windows deployment of the offscreen
  platform plugin and QML dependencies.
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

Storage-oriented suites reuse the temporary database and filesystem fixtures under `tests/support` so setup and cleanup rules remain consistent.

Suites registered through `jobtracker_add_backend_test` retain verbose Qt Test
diagnostics in `<build-directory>/<test-target>.txt` as well as requesting console
output. Use that file when a Windows host supplies an empty CTest failure log.

## Manual Checks

When automated validation is unavailable or not applicable, list the manual checks needed for the user to confirm the change.

For admission/feedback changes, verify repeated blocked actions, rejection from
the unsaved-changes Save flow, native multi-file picker selection restoration,
confirmation-time rechecks and focus, long/two-category banner text beside
ordinary notifications at 1180 x 720, and Wait/Interrupt-and-Exit with feedback
visible. Offscreen component tests do not verify native picker behavior.

For CV-import changes, manually exercise overlapping Add Job/Add CV requests,
archived duplicate restoration (including same-ID replacement), and cancel/close
with a large selected file. Real disk-full/permission failures, abrupt process
termination, Linux runtime behavior, and other-process races need separate
environment-specific verification. Full-file buffering intentionally increases
peak memory; no new CV size limit or streaming fallback is introduced.
