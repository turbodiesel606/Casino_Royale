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

Use:

```powershell
cmake --preset windows-debug-tests-local
```

```powershell
cmake --build --preset windows-debug-tests-local
```

```powershell
ctest --preset windows-debug-tests-local
```

If CTest reports that no tests were found, report that state clearly instead of treating it as a test failure.

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
- `JobTrackerMigrationTests` covers schema initialization, supported upgrades,
  rollback, foreign keys, reopen, unsupported newer versions, and named-connection
  cleanup when `SqliteDatabase` construction fails.
- `JobTrackerRepositoryTests` covers repository persistence plus shared SQL error and transaction infrastructure.
- `JobTrackerCvImportTests` covers managed file preparation, exact
  case-sensitive composite identity, cleanup, and recovery.
- `JobTrackerAddJobTests` covers the canonical preflight contract used by Add
  Job, defensive service pre-staging validation, cancellation immediately before
  a transaction, transaction behavior, and durable orchestration.
- `JobTrackerUpdateJobTests` covers in-place metadata and ordered-technology
  updates, company creation and normalized reuse, replacement-CV insertion,
  active and archived duplicate reuse, old-CV preservation, rollback and file
  cleanup, missing targets, defensive invalid-input rejection, and cancellation
  before the update transaction.
- `JobTrackerIntegrationTests` covers synchronous invalid-input rejection
  without queue, filesystem, or SQLite mutation; normalized controller FIFO
  admission; exact queued/final event ordering; worker-thread connection
  initialization and retry through final failure outcomes; FIFO persistence
  order; pending-count and saving transitions; CV and database-lock failure
  isolation; duplicate-CV cleanup; cooperative active and cancel-all behavior;
  GUI event-loop responsiveness; shutdown cleanup; and restart hydration.

Storage-oriented suites reuse the temporary database and filesystem fixtures under `tests/support` so setup and cleanup rules remain consistent.

## Manual Checks

When automated validation is unavailable or not applicable, list the manual checks needed for the user to confirm the change.
