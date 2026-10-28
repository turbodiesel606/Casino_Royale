# Installer Testing

Use this reference when verifying the Windows installer or its staged payload.
The suites under `tests/deployment/` are independent consumers: production
CMake and ordinary backend test presets do not include them, and packaging does
not depend on tests. Application/backend verification remains in
[build.md](build.md) and [testing.md](testing.md).

Use the [installer reference](installer.md) for construction and lifecycle
contracts, and the [acceptance checklist](../../tests/deployment/Windows-Release-Checklist.md)
for artifact-specific real installation, UI, and disconnected clean-host cases.
These procedures do not assert results for the current Setup.

## Evidence And Environment

Keep static source/artifact checks, native harnesses, real scoped installation,
actual application/data checks, interactive UI, clean-host startup, and
redistribution review distinct. Record the exact commands, exit codes, artifact
hashes, Windows edition/build, x64 architecture, account/elevation, runtime
state, language, destination, logs, and fixture cleanup. Use `PASS`, `FAIL`,
or `NOT_RUN` per applicable case. Retain failed attempts alongside retries.

Artifact-bound lifecycle runs (`Test-LocalInstaller.ps1`,
`Test-InstallationPolicy.ps1`, and `Test-InstallOperations.ps1`) use the test-only
`OfflineInstallerEvidence.ps1` reader. Their legacy `ManifestPath` parameter
accepts private offline `installer-build.json`, not an application archive
manifest. Before fixtures are created, the reader checks Setup filename, size,
version, hash, and recorded installer-source hashes against `Installer/Windows/`.
The compiled-artifact suite additionally checks snapshot/generated identity and
embedding. Use the Setup, build, stage, source state, and private evidence from
the same invocation; never choose arbitrary latest metadata.

`Test-PersonalData.ps1` is source-based by default. It compiles the current
production cleanup/consent source into native fixture consumers and records
`installerSha256=null`; no built Setup or private build JSON is required.
Only its `-RunCurrentUserIntegration` mode invokes the evidence reader and binds
the real installation/removal checks to the supplied Setup. Supplying
`InstallerPath` and `ManifestPath` without that switch does not validate those
files or establish correspondence to a particular EXE.

`Test-ShortcutLifecycle.ps1` also consumes current production source through
native fixture Setups rather than validating a supplied distribution EXE. Its
default mode substitutes scope and four disposable shell folders in a private
test copy; report those scope transitions as simulated. Its opt-in Windows
scope mode exercises the actual user/common shell-folder lifecycle and requires
an already elevated token. Neither mode establishes a particular production
Setup's artifact identity.

The builder records an exact approved `installerFileName`: either the manual
`JobTracker-Setup-<version>.exe` or the launcher's
`JobTracker-Setup-<version>-<UTC-time>-<id>.exe`. The reader validates the versioned
filename grammar and exact approved name, as well as size/hash/PE identity;
renaming to another otherwise valid timestamped name still fails. Older evidence
without this field uses the canonical manual name. Source-hash freshness remains
required. `Test-OfflineInstaller.ps1` retains its incorrectly renamed EXE case
and checks altered approved-filename metadata too.

For artifact-bound commands below, use the exact fields from
`Create-Installer.ps1 -PassThru` or its printed
`build/installer-create/windows-<run-id>/result.json`.
Launcher creation does not run any of these separate suites or acceptance cases.

Signing changes Setup bytes: the unsigned build JSON no longer matches that
signed EXE, so the existing reader rejects it. Retain the unsigned candidate for
repeatable suite checks and the separate signed-artifact receipt for verification
of the final signed candidate; do not rewrite the original evidence to imply a
matching build. See [optional signing](installer.md#optional-installer-signing).

Scripts require Windows and PowerShell 7.2+. Native harness compilation requires
Inno Setup 7+ and verifies native AMD64/PE32+ output. Run commands sequentially
from the repository root and wait for each process tree/final exit code.
CMake execution follows
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md).
The standalone staging-check CMake project selects `pwsh` and validates its
version on every configure, including cached `JOBTRACKER_POWERSHELL` values.
It rejects missing interpreters and versions below 7.2; Windows PowerShell
5.1 is unsupported. To replace a stale selection, configure with
`-UJOBTRACKER_POWERSHELL` or `-DJOBTRACKER_POWERSHELL=<path-to-pwsh>`.
Direct `Verify-Staging.ps1` invocation enforces the same minimum.

If `RemoteSigned` blocks trusted transferred scripts, use the dedicated process
described in the [build sequence](installer.md#build-sequence). Continue these
checks inside it so child PowerShell processes, including the CTest probe, inherit
the process setting without changing persistent policy or source zone metadata.

CurrentUser artifact-integration helpers use a non-elevated Windows token and
user shell-folder access. The focused Windows scope-transition mode described
below instead requires an already elevated token to exercise both installation
scopes and their actual shell folders. AllUsers lifecycle checks require
administrator elevation in a disposable environment. Codex host-tool access does not change
the Windows account/token requirement. Never start another real-install suite
until the prior owned fixtures are accounted for. Own only fresh test
directories and per-copy shortcuts; uninstall only the recorded test copies.
Existing personal installations must remain unchanged, and test copies must
create no JobTracker uninstall registration.

Only fresh owned synthetic trees under repository `build/installer-tests/`
are eligible for fixture cleanup. Use
`build/installer-tests/<run-id>/fixtures/<scenario>/`, including nested
`currentuser/` directories for optional integration. The subtree root itself,
other build trees, staging, installer outputs, and personal data are outside
this cleanup boundary. Keep private payload/probe copies under fresh `build/` paths.
Do not run against personal data or delete failed evidence. Actual app startup
and SQLite/CV/settings checks need disposable accounts/VMs; overriding environment
variables does not prove that Windows known-folder storage has been redirected.

## Paths For The Commands

For artifact-bound commands, continue with the exact `$installerBuild`, `$installerStage`,
`$installerSetup`, `$installerEvidence`, `$installerMetadata`, and
`$installerIscc` values from the [build sequence](installer.md#build-sequence).
When testing an existing artifact, explicitly restore those matching paths;
the private build JSON records its stage and input identity. Do not rebuild
solely to read this guide.

For the standalone personal-data suite, only the installed Inno compiler path
and a fresh fixture directory are needed; obtain `$installerIscc` from the local
`ISCC.exe` installation. The fixture-path definitions below do not require a
build or Setup artifact.

When Inno Setup 7+ is on PATH:

~~~powershell
$installerIscc = (Get-Command ISCC.exe -CommandType Application -ErrorAction Stop).Source
~~~

Otherwise set `$installerIscc` to the actual installed compiler's absolute path.

Choose fresh test directories for each run, leaving the stage/evidence unchanged:

~~~powershell
$installerTestRun = [Guid]::NewGuid().ToString('N')
~~~

~~~powershell
$installerTestRoot = Join-Path (Get-Location).Path "build/qt-6.10.3/installer-tests-$installerTestRun"
~~~

~~~powershell
$installerFixtureRoot = Join-Path (Get-Location).Path "build/installer-tests/$installerTestRun/fixtures"
~~~

## Direct Payload And Compiled Artifact

For focused launcher regressions, the separate
`tests/deployment/Test-CreateInstaller.ps1` accepts `-QtRoot <installed-kit>` and
`-WorkDirectory <fresh-path-under-build>`. Optional `-IncompatibleQtRoot` checks
automatic fallback from another installed Qt version. Its isolated fixtures
cover typed/inherited presets, exact Unicode build paths, cache preservation,
compiler conflicts, configuration precedence, native failure propagation and
overlapping-launcher refusal. It never builds an app or compiles/runs Setup;
it is not part of production packaging. Use the trusted process policy above
when required by transferred scripts.

`Test-OfflinePayload.ps1` exercises missing stages/plugins, Debug DLLs renamed
to Release names, extra source/test/dev material, PE architecture/product/version,
test-enabled/foreign build trees, stale identity, reparse points/ADS, and unsafe
paths. It retains private copies and verifies original input hashes afterward.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-OfflinePayload.ps1 -BuildDirectory $installerBuild -StageDirectory $installerStage -WorkDirectory "$installerTestRoot/payload-tests"
~~~

`Test-OfflineInstaller.ps1` checks the compiled EXE, source/snapshot/generated
identity hashes, exact embedded local file entries, and absence of transport
paths. Static embedding or absence of network code does not establish
disconnected-machine installation.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-OfflineInstaller.ps1 -BuildDirectory $installerBuild -InstallerPath $installerSetup -MetadataPath $installerMetadata -WorkDirectory "$installerTestRoot/artifact-tests"
~~~

## Standalone Staging Smoke

The standalone CMake project inspects the clean stage and runs
`JobTrackerDeploymentSmoke` from a retained private copy with system-only PATH
and cleared Qt/QML overrides. It checks Windows platform/QML imports, confined
plugin paths, SVG/JPEG, and in-memory SQLite. It never constructs the production
App or accesses JobTracker data. The helper/private copy must not enter a
customer payload. A developer host can still mask system dependencies; this
does not certify application rendering, native dialogs, or a clean machine.

Select the Qt 6.10.3 MSVC x64 kit used by the production build:

~~~powershell
$installerQt = (Resolve-Path -LiteralPath (Read-Host 'Qt 6.10.3 MSVC x64 kit root containing bin and lib/cmake')).Path
~~~

~~~powershell
cmake -S tests/deployment -B "$installerTestRoot/deployment-tests" -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$installerQt" "-DJOBTRACKER_STAGE_DIR=$installerStage"
~~~

~~~powershell
cmake --build "$installerTestRoot/deployment-tests" --config Release
~~~

~~~powershell
ctest --test-dir "$installerTestRoot/deployment-tests" -C Release --output-on-failure
~~~

The CTest name is `JobTrackerReleaseStaging`. Its default probe does not add
compiler runtime files to the common stage. To separately verify App-Local
loading using this Setup's verified private runtime snapshot:

~~~powershell
pwsh -NoProfile -File tests/deployment/Verify-Staging.ps1 -StageDir $installerStage -Probe "$installerTestRoot/deployment-tests/Release/JobTrackerDeploymentSmoke.exe" -WorkRoot "$installerTestRoot/app-local-smoke" -MsvcRuntimeDirectory "$installerEvidence/msvc-runtime"
~~~

## MSVC Runtime Harness

`Test-MsvcRuntime.ps1` and `MsvcRuntimeHarness.iss` consume the offline runtime
includes and lock-derived local input. They inspect payload/CRT import closure and
exercise result/reboot policy, real host runtime detection, current-user DLL
copies, destination guards, bad build inputs and native official Microsoft
embedded extraction/integrity. The harness never executes the redistributable:
system detection does not call the all-users prerequisite orchestration. These
checks cannot prove installation on a missing/old-runtime host. No fixture is a
release artifact.

`Verify-Staging.ps1 -MsvcRuntimeDirectory` adds only the verified CRT files to its
private test copy and asks the deployment probe to verify the actual loaded paths
of `msvcp140.dll`, `vcruntime140.dll` and `vcruntime140_1.dll`. The common staging
input must still contain no compiler runtime. This distinguishes App-Local loading
from a successful run masked by an installed system CRT. The owned offline installer
test additionally verifies per-copy CRT hashes in both scopes, its current-user policy
log, the runtime notice and uninstall removal. Actual clean-machine application
startup and all-users prerequisite execution remain separate acceptance checks.

Use the pinned redistributable snapshot from this candidate; the harness
extracts/verifies it but never executes the system prerequisite:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-MsvcRuntime.ps1 -BuildDirectory $installerBuild -InstallerDirectory $installerEvidence -RedistributablePath "$installerEvidence/msvc-runtime/vc_redist.x64.exe" -WorkDirectory "$installerTestRoot/runtime-tests" -IsccPath $installerIscc
~~~

## Owned Local Lifecycle Helper

`Test-LocalInstaller.ps1` operates only on its recorded synthetic installation
folders under `build/installer-tests/`. It verifies installed payload/runtime
hashes, local metadata/uninstaller, per-copy shortcuts, and absence of JobTracker
uninstall registration. Repair and uninstall preserve synthetic unrelated files
and other fixture copies. It never launches JobTracker or inspects personal data.
Use the same candidate, evidence, account, and fixture for related actions.
Actions are `Install`, `Verify`, `Reinstall`, `AdditionalCopy`,
`RepairMissingUninstaller`, `RepairMissingLog`, and `Uninstall`. Repeat
`-WithDesktopShortcut` on later actions when it was selected for installation.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Install -Language russian -WithDesktopShortcut
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Reinstall -Language russian -WithDesktopShortcut
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Uninstall -Language russian -WithDesktopShortcut
~~~

AllUsers integration needs an elevated token in a disposable environment.
Both scopes create actual per-copy shell links but no JobTracker installed-apps
entry. Do not substitute a personal installation for a missing test fixture.

## Installation Policy And Repair

`Test-InstallationPolicy.ps1` and `InstallationPolicyHarness.iss` consume the
production local installation policy. Fixtures cover local version comparison,
repairable missing evidence, nonempty unrelated destinations, nested/aliased
paths, personal-data overlap, and installer/uninstaller mutual exclusion.
Root-level and nested proposed folders exercise repeated canonicalization and
the wizard's pre-install policy sequence in both scopes without creating those
destinations. Existing empty folders remain covered; drive-root destinations
are rejected separately from resolving a root for path checks.
Tests must exercise the selected directory without requiring a global copy
catalog or every Windows profile. Synthetic version metadata establishes policy
behavior, not genuine historical application/database compatibility.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/policy" -IsccPath $installerIscc
~~~

For the interactive Browse regression, select a writable local drive, use New
Folder, name a fresh direct child, and accept the path. Preparation must reach
normal installation without an invalid-path error or duplicate separators.
Repeat with a typed new path, nested missing folders, and an existing empty
folder. The dialog selects a proposed folder; validation must not create it.

Use real installation integration to check two separate copies, same-folder
repair/update, known-newer same-folder downgrade rejection, and a separate
older-version destination. Remove owned fixture files to exercise missing EXE,
INI, uninstaller, and uninstall-data repair. Place unrelated sentinels inside
and outside the selected folder and verify their hashes afterward. Normal
repair must retain valid uninstall history; reconstructed history must not
claim unknown files.
With a missing ownership ledger, verify exact current-payload hash recognition
and preservation/incomplete reporting for modified files whose ownership cannot
be proved. Unverified history backups remain `.preserved-n` files.

Copy/move/rename only fixture installations. Invoke the relocated uninstaller
and verify the source files and shortcuts remain intact, then repair at the new
location and verify that its local removal affects only that copy. Check stale
and user-retargeted shortcuts, different scopes, and a disconnected other volume
in disposable Windows environments. Never use an old source-location uninstall
record as a fixture-cleanup shortcut.

## Shortcut Scope And Desktop Lifecycle

`Test-ShortcutLifecycle.ps1` compiles small native fixture Setups from the
current production lifecycle sources. Its default run uses a private test copy
with scope and user/common Start Menu/desktop destinations substituted by
disposable folders. This executes installation, reinstall, reconciliation,
failure/retry, and uninstall behavior while keeping the scope distinction
simulated; it is not evidence of Windows elevation or real shell placement.
Test substitutions remain outside production sources.
Both modes use synthetic payloads and a private prerequisite stub; they do not
execute the real MSVC prerequisite or establish UAC-dialog/credential-switch
behavior.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ShortcutLifecycle.ps1 -WorkDirectory "$installerFixtureRoot/shortcut-source" -IsccPath $installerIscc
~~~

Run the actual Windows scope mode from an already elevated disposable-account
session. If that token/environment is unavailable, the affected cases are
`NOT_RUN` with a reason; do not substitute policy-only scope combinations for
this evidence.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ShortcutLifecycle.ps1 -RunWindowsScopeIntegration -WorkDirectory "$installerFixtureRoot/shortcut-windows" -IsccPath $installerIscc
~~~

Cover both CurrentUser to AllUsers and AllUsers to CurrentUser in the same
directory, with reinstall and uninstall after each transition. Verify actual
resulting Start Menu and selected desktop locations, safe removal of old owned
links, preservation of retargeted/other-copy links, and retry records after
creation or verified-owned deletion fails. Cover desktop checked to unchecked
and back, including retargeting and deletion denial. A shortcut read failure
must preserve the link and its record and report incomplete work. Only an absent,
successfully removed, or verified foreign link can release the old record.
New-path intent must survive failed creation, and each shortcut kind retains
its own result if the other kind fails.

## Running Applications And Actual Filesystem Failures

`Test-InstallOperations.ps1` and `InstallOperationsHarness.iss` exercise actual
Windows replacement/removal failures without starting the real app against a
personal profile, using a synthetic mapped executable or native exclusive
file handle. Verify that installation to a different folder succeeds while
another fixture copy runs and that a same-folder attempt reaches the operation
rather than a process/idle gate. The process must remain alive and must not be
restarted or terminated by Setup/uninstall.

Record the actual locked/denied path, Windows error, remaining owned files, and
installer/uninstaller exit/log results. Check cancellation and interruption,
then repair where local evidence remains identifiable. Do not equate a failed
operation with complete rollback or a successful process exit with removal of
every file. A failed owned-program removal must produce native failure and
retain local uninstall metadata, log, and EXE for retry/repair. Confirm that the
data-choice/cleanup callback ran before the program attempt, independent of its
outcome. Program-removal results and confirmed data-cleanup results must be
reported independently. UI, another Windows session, and real running-app cases
require the separate acceptance matrix.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallOperations.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/operations" -IsccPath $installerIscc
~~~

The policy and operations suites accept `-RunCurrentUserIntegration` for
explicit real installation with a separate fresh fixture. Run from a
non-elevated Windows session; the synthetic harnesses do not establish real
AllUsers, UAC, or interactive-dialog behavior.

## Personal Data Safety

`Test-PersonalData.ps1` and the native personal-data harnesses consume the
production cleanup and consent policy using fresh synthetic fixture trees.
The ordinary standalone run records the current `PersonalData.iss` source hash,
executes its native cleanup/identity checks, and exercises consent branches
through test-only dialog/identity substitutions. It does not validate or run a
supplied Setup EXE and does not establish real interactive or UAC behavior.
Production has no test-only data path or consent switch. Preserve outside
sentinels, including files in the parent `JobTracker` directory, so a regression
from `JobTracker\Data` to the parent is observable. Missing `Data` is success.
Use safe disposable data for every destructive case.

Cover native deletion, Unicode names, path/ancestor/child reparse points,
hardlinks, access denial, read-only files, and real file locks. Include
user-owned Data, an Administrators-owned Data root, and Administrators-owned
children inside a user-owned root. Positive administrative-ownership fixtures
must retain and verify that ownership while Windows permissions allow deletion;
do not normalize them to the test user's SID. Give the empty-Data fixture an
explicit, verified owner and expected result. Access-denied fixtures must remain
denied during the cleanup attempt. Preserve parent, outside, and other-profile
sentinels in every case.
Verify complete versus incomplete cleanup and retained diagnostics; do not
promise restoration of files already deleted. A path-safety rejection is
separate from a Windows operation failure.

The consent policy must preserve on No, dismissal, or silent uninstall and
invoke cleanup immediately on affirmative confirmation, before program removal.
Running/remaining copies, disconnected volumes, leftover program files, and elevation are not
cleanup gates. Test-only callback substitutions can exercise policy branches,
but they do not establish that actual dialogs, UAC account changes, or known-
folder resolution work in a real session.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -WorkDirectory "$installerFixtureRoot/data-source" -IsccPath $installerIscc
~~~

To additionally validate the supplied Setup/build JSON and run that exact
candidate's CurrentUser install, reinstall, and silent-uninstall lifecycle,
use a separate fresh fixture from a non-elevated Windows session:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -RunCurrentUserIntegration -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/data-integration" -IsccPath $installerIscc
~~~

This integration mode records the validated `installerSha256` and checks the
real uninstaller's silent-retention path. It still does not authorize deletion
of the account's real JobTracker data or replace interactive/UAC acceptance.
If administrative ownership, elevation, or another required environment cannot
be established, record the affected cases as `NOT_RUN` with the reason.

Interactive EN/RU default-No prompts, exact account/profile/path display,
same-account elevation, alternate-credential UAC, actual filesystem permission
failures, and another running real copy require execution in disposable Windows
accounts/VMs. The target must never silently become the administrator's
Local AppData. Cover both desktop-token resolution and the explicit default-No
execution-account fallback selection when desktop identity cannot be resolved.
Do not enumerate every profile or touch the developer's real
JobTracker data to obtain this evidence.

## Acceptance Beyond Local Suites

Use the [checklist](../../tests/deployment/Windows-Release-Checklist.md) for
interactive EN/RU prompts/icons, real AllUsers/UAC and prerequisite
skip/run/failure/cancel/reboot, multiple accounts/sessions, actual
SQLite/CV/settings preservation, graphics, and disconnected clean Windows 10/11
x64 without Qt/Visual Studio. Synthetic version identities do not verify genuine
older-binary migrations; test supported application upgrades on disposable data
snapshots separately from installer policy.

No active suite serves HTTP, queries a release asset, or downloads/extracts an
application archive. The runtime lock URL is provenance and the localized
Microsoft license URL is explanatory text. Neither establishes disconnected
startup; only execution in the required environment can do that.

On a real maintenance/sharing failure, preserve the exact fixture, logs, path,
operation, timestamp, process identity, and access/share flags. Do not blindly
retry, force-close processes, weaken guards, or erase a failure after a later
pass. See [common failures](installer.md#changes-and-common-failures).
A successful suite does not resolve the separate
[distribution questions](installer.md#distribution-review-and-unavailable-history).
