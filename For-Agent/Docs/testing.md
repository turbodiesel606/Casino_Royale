# Testing Policy

Run tests when a change affects business logic, storage, parsing, algorithms,
cross-module contracts or high-risk runtime behavior. Do not add formal tests
only for appearance-level QML changes when no suitable infrastructure exists.
Do not claim code works unless verified.

For documentation/instruction-only changes that do not affect build behavior,
check links, command forms, preset/target/script contracts and evidence labels;
do not rebuild unchanged C++ only for prose edits. Use [build.md](build.md) for
shared action selection, then the relevant OS build reference for commands.
Standalone deployment and installer verification use the relevant existing
[installer guides](../../Installer/README.md).

Start target selection with the [compact map](#subsystem-to-test-map) below.
Consult [testing-reference.md](testing-reference.md) only for a specific suite,
fixture, diagnostic log, or manual scenario: search its Suite Responsibilities
section for the exact target name, or follow its feature-specific manual headings.
Ordinary target selection does not require reading the complete suite catalog.

## Production And Test Isolation

Tests are consumers of the production architecture, never production dependencies. Keep the dependency direction strictly one-way: `tests -> production`.

- Production code under `src/`, production targets, and production APIs must not include, link, reference, or conditionally depend on test sources, test targets, Qt Test, fixtures, mocks, test-only compile definitions, or test-only hooks.
- Do not add or widen production APIs, weaken access control, or alter production ownership, lifetime, threading, behavior, or architectural boundaries solely to make tests easier to write.
- Exercise existing production contracts. Keep test fixtures, mocks, fakes, helpers, and other test support under `tests/`.
- With `JOBTRACKER_BUILD_TESTS=OFF`, normal configure, build, and deployment must not require test-only packages, create or register test targets, compile test sources, deploy test dependencies, or change the `JobTrackerApp` artifact.
- Test-enabled builds may depend on and compile production code. Production code and normal builds must remain unchanged by whether tests are enabled.

## Current Test Setup

Tests are enabled through `JOBTRACKER_BUILD_TESTS` in separate test presets.
Use the shared [preset-selection rules](build.md#shared-presets-and-local-qt-selection)
and [routine workflow](build.md#routine-codex-workflow) for the validation sequence.
Read [build-windows.md](build-windows.md#test-runtime) or
[build-linux.md](build-linux.md#build-and-test-commands) for the relevant OS's
test-runtime preparation and commands. Preset inspection on Windows does not
verify Linux execution.

## Test Outcomes

Report the selected preset/configuration/filter, the number of tests actually
executed, and one of these outcomes:

- `passed`: the selected tests ran and all passed.
- `failed`: a CTest run reported failures; distinguish assertion failures from
  crashes, timeouts, or test-start/runtime errors, and count only tests that
  actually executed.
- `not run`: tests were not executed; state why and what remains unverified.
- `no matching tests`: selection found zero tests. This is neither a passing
  suite nor an executed failing test. All shared and local test presets set
  `execution.noTestsAction` to `error`, so an empty selection returns a nonzero
  exit code. An intentional nonexistent-filter check verifies this guard;
  report it as expected empty-selection rejection.

If tests were expected but none matched, verification is incomplete. Inspect the
selected preset, test-enabled configuration, CMake registration, and filter before
claiming success. Report configuration, build or execution errors separately
from test assertion failures; tests blocked by those errors are `not run`.

## Test Scope

Choose test scope from the final diff and affected contracts. For an ordinary
bounded change, start with the directly associated targets or CTest selections
from the map below. Use [build.md](build.md#optional-tests) for action selection
and its relevant OS reference for canonical command forms. Substitute verified
preset names, target names, and filters as appropriate; the examples are not an
exhaustive list of permitted targets.

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

## Manual Checks

When automated validation is unavailable or not applicable, list the manual
checks needed for the user to confirm the change. Consult only the relevant
[manual scenario](testing-reference.md#manual-checks), including its expected
environment-specific limitations.
