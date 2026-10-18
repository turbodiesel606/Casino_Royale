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

Lifecycle suites use the test-only `OfflineInstallerEvidence.ps1` reader.
Their legacy `ManifestPath` parameter accepts private offline
`installer-build.json`, not an application archive manifest. Before fixtures
are created, the reader checks Setup filename, size, version, hash, and recorded
installer-source hashes. The compiled-artifact suite additionally checks
snapshot/generated identity and embedding. Use the Setup, build, stage, source
state, and private evidence from the same invocation; never choose arbitrary
latest metadata.

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

CurrentUser native/lifecycle suites need a non-elevated Windows token and
HKCU/shell-folder access. AllUsers lifecycle checks require administrator
elevation in a disposable environment. Codex host-tool access does not change
the Windows account/token requirement. Never start another real-install suite
until the prior owned fixture is safely uninstalled. Refuse an existing personal
registration or shortcuts; uninstall only the recorded test installation.

Only fresh owned synthetic trees under repository `dist/` are eligible for
test deletion. Keep private payload/probe copies under fresh `build/` paths.
Do not run against personal data or delete failed evidence. Actual app startup
and SQLite/CV/settings checks need disposable accounts/VMs; overriding environment
variables does not prove that Windows known-folder storage has been redirected.

## Paths For The Commands

Continue with the exact `$installerBuild`, `$installerStage`,
`$installerSetup`, `$installerEvidence`, `$installerMetadata`, and
`$installerIscc` values from the [build sequence](installer.md#build-sequence).
When testing an existing artifact, explicitly restore those matching paths;
the private build JSON records its stage and input identity. Do not rebuild
solely to read this guide.

Choose fresh test directories for each run, leaving the stage/evidence unchanged:

~~~powershell
$installerTestRun = [Guid]::NewGuid().ToString('N')
~~~

~~~powershell
$installerTestRoot = Join-Path (Get-Location).Path "build/qt-6.10.3/installer-tests-$installerTestRun"
~~~

~~~powershell
$installerFixtureRoot = Join-Path (Get-Location).Path "dist/installer-tests-$installerTestRun"
~~~

## Direct Payload And Compiled Artifact

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
test additionally verifies CRT hashes/absence by scope, its current-user policy
log, the runtime notice and uninstall removal. Actual clean-machine application
startup and all-users prerequisite execution remain separate acceptance checks.

Use the pinned redistributable snapshot from this candidate; the harness
extracts/verifies it but never executes the system prerequisite:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-MsvcRuntime.ps1 -BuildDirectory $installerBuild -InstallerDirectory $installerEvidence -RedistributablePath "$installerEvidence/msvc-runtime/vc_redist.x64.exe" -WorkDirectory "$installerTestRoot/runtime-tests" -IsccPath $installerIscc
~~~

## Owned Local Lifecycle Helper

`Test-LocalInstaller.ps1` uses fresh `dist/` fixtures, refuses existing scoped
registrations/shortcuts, verifies installed hashes, registration, shortcuts,
previous-directory reuse and scope conflict, then checks uninstall preservation
of synthetic outside/unknown files. The retained helper name does not denote
a separate release mode. It never launches JobTracker or inspects personal data.

Use the same account, Setup, JSON, and fixture for each action. Optional
Russian/Unicode-path and desktop-shortcut coverage:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Install -Language russian -WithDesktopShortcut
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Reinstall -Language russian
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Uninstall -Language russian
~~~

The helper also supports `Verify`, `ScopeConflict`, `AllUsers`, and
`-Language english`. AllUsers uses an elevated OS token; run real scope changes
only in the corresponding disposable environment. Both scopes create actual
shell/registry entries.

## Installation Policy

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
releases and GUI/clean-machine behavior need separate acceptance evidence.
Production CMake and normal builds remain independent.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/policy" -IsccPath $installerIscc
~~~

Opt-in real CurrentUser integration, only after confirming the environment and
absence of another owned/personal registration:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/policy-integration" -IsccPath $installerIscc -RunCurrentUserIntegration
~~~

## Manual Closure

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
Inno's own error summary. The installed executable is temporarily replaced by
the coherent synthetic process fixture and restored before successful
reinstall/uninstall checks.

Rerunning after cooperative closure is not an interactive Retry-button test.
The late-uninstall case directly invokes the production guard in a test Setup;
real uninstall covers the early running/closed gates, not a delayed native
uninstaller event. Cross-session/AllUsers, GUI/VM and concurrent-start races
require separate verification. Native rollback does not establish restoration of all previously
overwritten upgrade binaries. Do not claim transactional upgrade rollback.
Revalidate the cancellation sequence when upgrading Inno Setup: Check and
BeforeInstall exceptions alone cannot abort its native file loop.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ManualClose.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/manual-close" -IsccPath $installerIscc
~~~

Opt-in real CurrentUser integration with a separate fresh fixture:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ManualClose.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/manual-integration" -IsccPath $installerIscc -RunCurrentUserIntegration
~~~

## Personal Data Safety

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
`OfflineInstallerEvidence.ps1` before fixture creation. It checks actual
registration identity, another installation path being refused, presence-based shared-data protection, default
uninstall and registration removal, plus repair and unchanged synthetic sibling
data. It never resolves/reads the account's real
JobTracker data. All real test installations must be uninstalled afterward.
Test helper installations without registrations/shortcuts remain as retained
fixtures. Production CMake/packaging do not depend on these tests.

If a real maintenance failure leaves an incomplete fixture, the helper refuses
blind uninstall; retain its state/logs and recover
only after verifying its exact ownership and contents before another real suite.

Interactive EN/RU consent/default No/cancellation, true AllUsers/UAC account
mismatch, dual-scope uninstall, registry ACL denial, concurrent mutation and
clean-VM behavior remain manual acceptance checks. Direct EN/RU calls of the
cleanup core do not verify the prompt. AllUsers/elevated uninstall intentionally
preserves data; optional deletion is limited to verified non-elevated CurrentUser.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/data" -IsccPath $installerIscc
~~~

Opt-in real CurrentUser integration with a separate fresh fixture:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -InstallerPath $installerSetup -ManifestPath $installerMetadata -WorkDirectory "$installerFixtureRoot/data-integration" -IsccPath $installerIscc -RunCurrentUserIntegration
~~~

## Acceptance Beyond Local Suites

Use the [checklist](../../tests/deployment/Windows-Release-Checklist.md) for
interactive EN/RU prompts/icons/Retry, real AllUsers/UAC and prerequisite
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
