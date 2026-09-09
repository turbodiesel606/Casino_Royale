# Testing Policy

Run tests when a change affects:

- Business logic.
- Storage.
- Parsing.
- Algorithms.
- Cross-module contracts.
- High-risk runtime behavior.

Do not add formal tests only for appearance-level QML changes when no suitable test infrastructure exists.

Do not claim that code works unless it was verified.

## Production And Test Isolation

Tests are consumers of the production architecture, never production dependencies. Keep the dependency direction strictly one-way: `tests -> production`.

- Production code under `src/`, production targets, and production APIs must not include, link, reference, or conditionally depend on test sources, test targets, Qt Test, fixtures, mocks, test-only compile definitions, or test-only hooks.
- Do not add or widen production APIs, weaken access control, or alter production ownership, lifetime, threading, behavior, or architectural boundaries solely to make tests easier to write.
- Exercise existing production contracts. Keep test fixtures, mocks, fakes, helpers, and other test support under `tests/`.
- With `JOBTRACKER_BUILD_TESTS=OFF`, normal configure, build, and deployment must not require test-only packages, create or register test targets, compile test sources, deploy test dependencies, or change the `JobTrackerApp` artifact.
- Test-enabled builds may depend on and compile production code. Production code and normal builds must remain unchanged by whether tests are enabled.

## Current Test Setup

Tests are enabled through the `JOBTRACKER_BUILD_TESTS` CMake option and the `windows-debug-tests-local` preset.

Use only the configure, build, and CTest commands in
`For-Agent/Docs/build.md`; that document is the canonical command catalog.

If CTest reports that no tests were found, report that state clearly instead of treating it as a test failure.

## Test Scope

Choose test scope from the final diff. For an ordinary bounded change, build and run the test target or CTest suite directly associated with the changed subsystem. Use the targeted commands documented in `For-Agent/Docs/build.md`.

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

## Test Suite Responsibilities

- `JobTrackerCommonTests` covers shared filtering, limited sorting, and canonical job validation.
- `JobTrackerRelationProxyTests` covers relation filtering and every relevant source-model mutation.
- `JobTrackerSelectionTrackerTests` covers stable-ID selection, fallback behavior, and precise change reporting.
- `JobTrackerAsyncInfrastructureTests` covers serial FIFO operation IDs,
  activation, cancellation, completion suppression, stale-result correlation,
  drained state, lazy worker-executor creation, initialization failure, busy and
  shutdown state, worker-thread context destruction, and quit/wait cleanup.
- Jobs, CV, dashboard, and directory controller suites retain QML-facing
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
  structured failures, cancellation before and during staging, immediate `.part`
  cleanup while preparation remains alive, and startup recovery.
- `JobTrackerCvMutationQueueTests` covers FIFO contention, move-only lease
  ownership/RAII release, and cancellation of a waiter while the active lease
  remains held. A test-only timeout bounds synchronization regressions.
- `JobTrackerAddJobTests` covers the canonical preflight contract used by Add
  Job, defensive service pre-staging validation, cancellation immediately before
  a transaction, archived restoration and its rollback, cleanup when CV/job
  insertion fails, cancellation after final rename still committing, transaction
  behavior, and durable orchestration.
- `JobTrackerUpdateJobTests` covers in-place metadata and ordered-technology
  updates, company creation and normalized reuse, replacement-CV insertion,
  active duplicate reuse, archived restoration including same-ID replacement,
  metadata-only queue bypass, old-CV preservation, restoration rollback and file
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

Storage-oriented suites reuse the temporary database and filesystem fixtures under `tests/support` so setup and cleanup rules remain consistent.

## Manual Checks

When automated validation is unavailable or not applicable, list the manual checks needed for the user to confirm the change.

For CV-import changes, manually exercise overlapping Add Job/Add CV requests,
archived duplicate restoration (including same-ID replacement), and cancel/close
with a large selected file. Real disk-full/permission failures, abrupt process
termination, Linux runtime behavior, and other-process races need separate
environment-specific verification. Full-file buffering intentionally increases
peak memory; no new CV size limit or streaming fallback is introduced.
