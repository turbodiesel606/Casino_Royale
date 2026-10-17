# Testing Policy

Run tests when a change affects business logic, storage, parsing, algorithms,
cross-module contracts or high-risk runtime behavior. Do not add formal tests
only for appearance-level QML changes when no suitable infrastructure exists.
Do not claim code works unless verified.

For documentation/instruction-only changes that do not affect build behavior,
check links, command forms, preset/target/script contracts and evidence labels;
do not rebuild unchanged C++ or rerun installer mutations only for prose edits.
[build.md](build.md) owns commands, [windows-release.md](windows-release.md)
describes installer behavior, and
[offline STATUS](../../Tasks/Offline-Installer/STATUS.md) links dated results.

## Windows Release Evidence Boundaries

Treat static source/artifact validation, native harnesses, real owned scoped
installation, actual app/data, interactive UI, disconnected clean-host startup
and redistribution approval as separate evidence. Use the
[checklist](../../tests/deployment/Windows-Release-Checklist.md) and
[acceptance matrix](../../Tasks/Offline-Installer/00_ACCEPTANCE_MATRIX.md).

The active standalone consumers use a direct stage and one offline Setup:

| Consumer | What it establishes / limitation |
|---|---|
| `Test-OfflinePayload.ps1` | Private-copy rejection cases: exact inventory, source/test/dev extras, Release/Qt/PE identity, paths/reparse/ADS; preserves original inputs. |
| `Test-OfflineInstaller.ps1` | Static compiled EXE, snapshot, generated identity/source hashes and exact embedded local file entries; no disconnected-machine execution. |
| `Test-MsvcRuntime.ps1` | Native x64 extraction/integrity, imports, result/reboot policy and read-only detection; never executes the system prerequisite. |
| `Test-InstallationPolicy.ps1` | Same-scope identity/maintenance, downgrade/relocation/opposite-scope rejection and isolated registry/profile fixtures. |
| `Test-ManualClose.ps1` | Synthetic exact-path processes, manual-close gate and late boundary checks; no real application launch. |
| `Test-PersonalData.ps1` | Native owned-path/link/ACL/handle safety and private simulated account/scope/dialog callbacks. |
| `Test-LocalInstaller.ps1` | Guarded opt-in owned lifecycle with synthetic sentinels; retained helper name, not a Local release mode. |
| Standalone CMake/`Verify-Staging.ps1` | Private-copy Qt/QML/platform/SQLite/images and optional App-Local loading; never constructs the production App. |

Native harnesses require Inno Setup 7/native x64. Lifecycle suites share
test-only `OfflineInstallerEvidence.ps1`; the legacy `ManifestPath` argument
accepts private offline `installer-build.json`, not an archive manifest. The
reader checks Setup filename/size/version/hash and recorded source hashes before
creating fixtures. Metadata stays outside `Installer/output/`.

Use [offline suite commands](build.md#offline-deployment-tests). The three
lifecycle/data suites can opt into real CurrentUser integration with a fresh
owned fixture, absent existing registration and a non-elevated Windows token.
Registry/shell-folder access requires host-tool permission but never implies OS
elevation. Do not start another real-install suite until the prior fixture has
been safely uninstalled. Only fresh synthetic trees below `dist/` are eligible
for test deletion; do not run against actual personal data.

[Stage 08](../Task-Report/offline-installer/08.md), 2026-10-16, records 34 payload,
14 artifact, 12 runtime, 50 policy, 16 manual-close and 62 data checks, owned
CurrentUser install/repair/uninstall, standalone CTest 1/1 and App-Local smoke.
These are prior runs, not fresh results from reading/updating documentation.
Earlier assertion/setup failures remain in the reports. The old online/archive
test consumers were removed; they are not alternate active suites.

The earlier sharing error 32 recurred in offline stage 06 and remains unresolved
despite a successful guarded recovery and fresh retry. Preserve failed evidence
and manual-close safeguards; a retry does not identify the lock owner or prove a
fix. Synthetic version/data fixtures do not establish real migrations or
SQLite/CV/settings preservation.

Actual app startup/graphics/data, interactive EN/RU/cleanup, AllUsers/UAC,
prerequisite skip/run/failure/cancel/reboot, disconnected clean Windows 10/11 and
Linux execution remain NOT RUN. Use disposable accounts/VMs for stage-11
acceptance and record exact environment, commands, exits and artifacts.

The [redistribution hold](../../packaging/windows/REDISTRIBUTION.md) and
[current license issues](../Task-Report/offline-installer/LICENSE_ISSUES.md)
remain independent of tests. A changed payload/runtime/engine needs new matching
evidence; no passing suite or DONE stage grants legal or publication approval.
Embedded QML resources are allowed. Private logs/JSON/includes/resource evidence
must not enter customer output.

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
matching single-config build types. Linux execution remains NOT RUN on the
Windows stage-02 host; the concrete Linux verification recipe is in `build.md`.

The Windows test runtime helper copies the configuration-matched Qt libraries
and SQLite driver into each test output directory (`sqldrivers/qsqlite[d].dll`).
QtSql loads SQLite dynamically; linking/copying QtSql alone does not supply the
driver. This preparation remains inside `JOBTRACKER_BUILD_TESTS` and does not
deploy the production application or create a Release staging directory.

Windows Release staging has an additional standalone verification project in
`tests/deployment/`. It is not included by the production CMake project or the
ordinary backend test presets. Its CTest check inspects a clean installed
payload, then runs a helper from a retained private copy with system-only PATH
and no inherited Qt/QML overrides. The helper loads the Windows platform, the
current application's Qt QML import families, SVG/JPEG plugins and in-memory
SQLite; it never constructs the production App or accesses JobTracker data.
The helper and its private copy are test artifacts, not release payload files.
This check does not establish full application startup, rendering, native dialog
interaction or clean-machine/MSVC runtime acceptance. Its canonical commands
are in the Windows Release staging section of `build.md`.

`Test-OfflinePayload.ps1` exercises missing stage/plugins, Debug DLLs renamed to
Release names, extra source/test/dev material, PE architecture/product/version,
test-enabled/foreign build trees, stale identity, reparse/ADS and unsafe paths.
It retains private copies under a fresh `build/` directory and verifies original
input hashes afterward. It and the compiled-artifact suite are independent of
production CMake and ordinary backend CTest.

Use only the configure, build, and CTest commands in
`For-Agent/Docs/build.md`; that document is the canonical command catalog.

If CTest reports that no tests were found, report that state clearly instead of treating it as a test failure.

## Test Scope

The standalone `tests/deployment/Test-LocalInstaller.ps1` consumes the real offline
installer; production CMake and packaging never depend on it. It uses fresh
`dist/` fixtures, refuses existing scoped registrations/shortcuts, verifies
installed hashes, registration, shortcuts, previous-directory reuse and a scope
conflict, then checks that uninstall preserves synthetic outside/unknown files.
It never launches JobTracker or inspects personal application data. CurrentUser
must run without OS elevation, AllUsers with it. Both create real shell/registry
entries, so use a disposable account/VM and uninstall only the recorded fixture.
Exact commands and current Windows verification limits are in
[Offline Deployment Tests](build.md#offline-deployment-tests). CurrentUser results are dated
in the offline reports; actual AllUsers/UAC/GUI acceptance remains pending.

No active deployment suite starts an HTTP server, queries a release asset or
constructs/downloads/extracts an application archive. The old transport-only
cases were retired; local traversal, junction, integrity and runtime safeguards
remain in the direct payload and native suites. Static absence-of-network
checks do not prove disconnected installation; that requires the clean-host
acceptance matrix. The official URL in the runtime lock is provenance and the
localized Microsoft license URL is explanatory text, not a download operation.

`Test-MsvcRuntime.ps1` and `MsvcRuntimeHarness.iss` consume the offline runtime
includes and lock-derived local input. They inspect payload/CRT import closure and
exercise result/reboot policy, real host runtime detection, current-user DLL
copies, destination guards, bad build inputs and native official Microsoft
embedded extraction/integrity. The harness never executes the redistributable:
system detection does not call the all-users prerequisite orchestration. These
checks cannot prove installation on a missing/old-runtime host. The separate
historical Windows-Release stage-09 counts include retired download cases;
current offline counts belong in the offline reports. No fixture is a release artifact.

`Verify-Staging.ps1 -MsvcRuntimeDirectory` adds only the verified CRT files to its
private test copy and asks the deployment probe to verify the actual loaded paths
of `msvcp140.dll`, `vcruntime140.dll` and `vcruntime140_1.dll`. The common staging
input must still contain no compiler runtime. This distinguishes App-Local loading
from a successful run masked by an installed system CRT. The owned offline installer
test additionally verifies CRT hashes/absence by scope, its current-user policy
log, the runtime notice and uninstall removal. Actual clean-machine application
startup and all-users prerequisite execution remain separate acceptance checks.

`Test-InstallationPolicy.ps1` and `InstallationPolicyHarness.iss` consume the
current offline version, registry, path and concurrency policy. They use
GUID-qualified HKCU test keys and retained `dist/` fixtures, test both scope rules,
malformed/mismatched identities, unsigned numeric ordering, equal/nested/aliased
paths, unavailable profiles and the shared installer/uninstaller lock. The
optional non-elevated CurrentUser integration refuses existing registration,
installs the real offline installer, supplies coherent synthetic older/newer
PE/registry/marker identities in its own fixture, checks repair/path reuse and
English/Russian shortcut cleanup, then uninstalls only that verified fixture.
The test waits for each installer process tree before restoring fixture state.
Real application data and migrations are never accessed. Read-only HKU and
all-users audits exercise host registry discovery, not an actual system install.
Actual AllUsers scope-conflict/installation, multiple sessions/ACLs, actual old
releases and GUI/clean-machine behavior still need separate acceptance evidence.
Production CMake and normal builds remain independent. Commands are in the
Windows Offline Installation Lifecycle section of `build.md`.

`Test-ManualClose.ps1`, `ManualCloseHarness.iss` and
`RunningApplicationFixture.cs` consume the production manual-close policy. The
fixture is a real x64 mapped executable with the release version; it waits for
a test-owned stop file and exits normally. It does not load Qt/JobTracker or
read personal data. Tests check the exact file while running/closed, an unrelated
same-name copy, unavailable write access, EN/RU silent cancellation with
`/SUPPRESSMSGBOXES`, post-preflight starts, and rejection of automatic-close
options. The per-file cancellation case requires exit 5, native acknowledgement,
rollback logs, surviving process and unchanged protected files. Optional real
CurrentUser integration verifies install/reinstall/uninstall and preserved
sentinels, then removes its owned registration using the existing fixture helper.
Silent native uninstall and late-copy cancellation also run without message-box
suppression. Setup preparation failures require `/SUPPRESSMSGBOXES` to suppress
Inno's own error summary. The installed executable is temporarily replaced by the coherent synthetic
process fixture and restored before successful reinstall/uninstall checks.

Rerunning after cooperative closure is not an interactive Retry-button test.
The late-uninstall case directly invokes the production guard in a test Setup;
real uninstall covers the early running/closed gates, not a delayed native
uninstaller event. Cross-session/AllUsers, GUI/VM and concurrent-start races are
unverified. Native rollback does not establish restoration of all previously
overwritten upgrade binaries. Do not claim transactional upgrade rollback.
Revalidate the cancellation sequence when upgrading Inno Setup: Check and
BeforeInstall exceptions alone cannot abort its native file loop. Commands are
in [Windows Offline Installation Lifecycle](build.md#windows-offline-installation-lifecycle).

`Test-PersonalData.ps1`, `PersonalDataHarness.iss`,
`PersonalDataStateHarness.iss` and `PersonalDataPolicyHarness.iss` verify the
retained production cleanup policy for the offline installer. All three native
harnesses require Inno 7, request x64 and are checked for AMD64/PE32+ output.
Only fresh synthetic `dist/` trees are eligible for test deletion; production
has no test path/consent override. Tests check native handle deletion, Unicode
CV names, all-tree preflight on locks/read-only files, SID comparison, hardlinks,
ancestor/root/child junctions, unsafe roots, access denial, and real helper-
uninstaller complete and partial removal. The ACL case denies a directly
requested attribute-read right as well as deletion and restores the original
ACL in finally; parent deletion rights cannot invalidate the refusal test.
The owner case compares a real fixture handle against an
alternate valid SID; it does not change the file owner or emulate a real UAC
account switch. The lifecycle cases exercise silent preservation and separately
assert the residual-program gate; they do not redirect the production data path.

The callback harness compiles a private policy copy with exact-count substitutions
only for process scope/elevation, silence, known-folder resolution, registration
reads and dialog responses. Native path/ownership/handle deletion code is kept.
All folder results are compiled beneath the fresh fixture, and arbitrary scenario
names fail closed. Both languages check default No/window-close/Yes, silent and
AllUsers/elevated refusal, invalid/missing account identity, reboot/incomplete
removal, both registry views, access-denied state, and renewed guards after
consent. AllUsers/elevated cases assert zero account/registry/data-folder queries.
These are native policy simulations, not actual interactive dialogs or UAC.

Optional non-elevated CurrentUser integration uses the real installer and the
existing owned fixture helper. Private offline build JSON is validated through
`OfflineInstallerEvidence.ps1` before fixture creation. It checks actual registration identity, another
installation path being refused, presence-based shared-data protection, default
uninstall and registration removal, plus repair and unchanged synthetic sibling
data. It never resolves/reads the account's real
JobTracker data. All real test installations must be uninstalled afterward.
Test helper installations without registrations/shortcuts remain as retained
fixtures. Commands are in the Windows Safe Uninstall And Personal Data section
of `build.md`; production CMake/packaging do not depend on these tests.

Stage 07's final suite passed 62 checks, including real owned CurrentUser
install/repair/uninstall. Its earlier harness setup/log assertion failures are
retained in the stage report. If a real maintenance failure leaves an incomplete
fixture, the helper refuses blind uninstall; retain its state/logs and recover
only after verifying its exact ownership and contents before another real suite.

Interactive EN/RU consent/default No/cancellation, true AllUsers/UAC account
mismatch, dual-scope uninstall, registry ACL denial, concurrent mutation and
clean-VM behavior remain manual acceptance checks. Direct EN/RU calls of the
cleanup core do not verify the prompt. AllUsers/elevated uninstall intentionally
preserves data; optional deletion is limited to verified non-elevated CurrentUser.

The reusable `tests/deployment/Windows-Release-Checklist.md` combines the
standalone suites with explicit real-installer, UI and clean-machine matrices.
Use the current offline commands in `build.md` with direct staging and matching
private build JSON. Record evidence levels separately: synthetic version identities test
installer policy, not historical-binary migrations; a Qt staging probe does not
start JobTracker; static embedding/network checks do not prove disconnected
installation. Application startup and actual SQLite/CV preservation require a
disposable account/VM. Keep failed attempts and intermittent sharing failures in
the report even when a later unchanged installer passes. No production target
depends on the checklist or standalone suites.

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
