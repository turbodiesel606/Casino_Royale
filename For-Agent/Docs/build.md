# Build And Verification

This project uses CMake 3.21 or newer and requires exactly Qt 6.10.3 on both
Windows and Linux. Production and test discovery use the same version pin.

Use this file as the single executable command catalog. See [Building](../../docs/BUILDING.md),
[Windows Release](windows-release.md), and the [testing policy](testing.md) for
context; exact dated results remain in [stage reports](../../Tasks/Offline-Installer/STATUS.md).

**Publication HOLD:** findings in [LICENSE_ISSUES.md](../Task-Report/offline-installer/LICENSE_ISSUES.md) remain
unresolved. These commands can produce unapproved candidates; the hold is a
maintainer requirement, not an automated packaging gate.

Run Windows commands from `D:\Project_CV\Root` in PowerShell. The Linux section
is for PowerShell in the corresponding Linux checkout, not execution on Windows.

Run commands one at a time. Do not combine build or test commands with `;`, `&&`, pipes, or other shell separators.

Do not delete build directories, CMake caches, generated files, Qt deployment output, or other build artifacts without explicit user approval.

## Build Execution Requirements

Always run every `cmake --build ...` command with elevated access from the first attempt. Do not first attempt a build with restricted or sandboxed access.

Allow enough execution time for the build to finish and return its final exit code. If the command runner yields a still-running build, keep waiting on that same process until it completes. Do not infer completion from a period without output, do not report a final build status while it is still running, and do not launch a duplicate build process.

If an external command timeout terminates the build, confirm that no build process remains before retrying once with a longer timeout and elevated access.

Keep normal configure, build, and deployment independent of tests. When `JOBTRACKER_BUILD_TESTS=OFF`, CMake must not require Qt Test or another test-only package, create or register test targets, compile test sources, deploy test dependencies, or otherwise change the `JobTrackerApp` build. Keep test-only package discovery, targets, registration, compilation, and deployment inside the test-enabled configuration.

## Windows Build

Shared presets contain no machine-specific SDK path. They read `QTDIR` as the
Qt kit root through `CMAKE_PREFIX_PATH`; alternatively, an ignored local
`CMakeUserPresets.json` can override `CMAKE_PREFIX_PATH`. This workspace's local
Windows presets preserve `D:/Qt_install/6.10.3/msvc2022_64`. Set the path to the
Qt kit containing `bin/` and `lib/cmake/`, not to its parent version directory.

Presets use schema version 3, matching CMake 3.21; user presets implicitly
include the shared file. Windows presets select Visual Studio 2022 and x64.
Debug/Release and production/test configurations use distinct build directories
under `build/qt-6.10.3/`. Existing Debug preset names and directories are retained.
`QT_QML_DEBUG` and MSVC Just My Code remain Debug-only.

CMakeUserPresets.json is already tracked in this checkout. The ignore rule
prevents new additions but cannot untrack an existing file. The
[offline stage-02 report](../Task-Report/offline-installer/02.md) records the
optional index-only operation for a separately authorized Git task; existing
local preset values must be preserved.

Configure when the build tree is missing or stale, or when CMake, presets, source registration, or build options changed:

```powershell
cmake --preset windows-debug-local
```

Targeted production build for an ordinary bounded change:

```powershell
cmake --build --preset windows-debug-local --target JobTrackerApp
```

Full production preset build when the validation policy requires broader coverage:

```powershell
cmake --build --preset windows-debug-local
```

### Local Development Runtime And Startup

Normal application builds compile and link; they do not run `windeployqt`,
install, package, or remove a deployment directory. After the first build, or
after changing Qt/QML dependencies, explicitly prepare the Windows development
runtime beside the executable:

```powershell
cmake --build --preset windows-debug-local --target JobTrackerApp_dev_runtime
```

This opt-in target requires `windeployqt` from the Qt SDK. It builds the
application first if needed, copies Qt/QML/platform/SQLite dependencies, and
omits compiler-runtime deployment. Use it on the Qt/MSVC development machine;
its Debug output is not a redistributable Release package. Existing build-tree
DLLs are preserved, so an old populated tree alone does not prove deployment.

Then start the built Debug executable, or use it in the existing debugger:

```powershell
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

This launch uses the account's persistent JobTracker data, including startup
CV recovery. Use a disposable Windows test account for manual startup checks
when existing personal data must remain untouched. Build/runtime preparation
results and manual verification limits are recorded separately in stage reports.

## Windows Build With Tests

Configure when the test build tree is missing or stale, or when CMake, presets, source registration, or build options changed:

```powershell
cmake --preset windows-debug-tests-local
```

Build one directly associated test target (concrete example; select the affected
registered target from `testing.md`, not this example for every change):

```powershell
cmake --build --preset windows-debug-tests-local --target JobTrackerCommonTests
```

Run one directly associated CTest suite:

```powershell
ctest --preset windows-debug-tests-local -R "^JobTrackerCommonTests$"
```

Build the full test-enabled preset when the validation policy requires broader coverage:

```powershell
cmake --build --preset windows-debug-tests-local
```

Run the full registered test set when the validation policy requires broader coverage:

```powershell
ctest --preset windows-debug-tests-local
```

## Verification Provenance And Paths

These are current command forms, checked against the real presets, targets and
script parameters in offline stage 09. Documentation validation does not execute
a build or installer. [Stage 02](../Task-Report/offline-installer/02.md) records
Windows Debug/Release production/test builds and 18/18 CTest in each;
[stage 08](../Task-Report/offline-installer/08.md) records the offline candidate,
native suites, standalone probe and owned CurrentUser lifecycle. Linux and
disconnected clean-host acceptance remain NOT RUN.

For a new deployment run, use the session variables below to create unique
staging/probe/test paths. Keep the configured production/test trees. Never replay
creation commands into prior evidence or delete failed outputs. Installer scripts
require PowerShell 7.2+ and Inno Setup 7+; the verified native-x64 compiler was
7.1.0. Inno 6 creates an I386 launcher and is rejected.

## Windows Release Build And Tests

Normal Release builds compile/link. Staging and offline Setup generation
are separate explicit steps. Production and tests use different directories.

```powershell
cmake --preset windows-release-local
```

```powershell
cmake --build --preset windows-release-local
```

```powershell
cmake --preset windows-release-tests-local
```

```powershell
cmake --build --preset windows-release-tests-local --parallel 4
```

```powershell
ctest --preset windows-release-tests-local
```

### Windows Release Staging

After the production Release build, initialize these PowerShell session variables
one command at a time. Run all subsequent offline sections in the same session.
The compiler and redistributable paths below are verified local inputs on this
host; select authorized local equivalents on another machine. The runtime lock
requires 14.44.35211.0. No command downloads a dependency or refreshes that lock.

~~~powershell
$offlineRun = [Guid]::NewGuid().ToString('N')
~~~

~~~powershell
$offlineBuild = (Resolve-Path -LiteralPath build/qt-6.10.3/windows-release).Path
~~~

~~~powershell
$offlineRoot = Join-Path (Get-Location).Path "build/qt-6.10.3/offline-$offlineRun"
~~~

~~~powershell
$offlineStage = Join-Path $offlineRoot 'stage'
~~~

~~~powershell
$offlineFixtureRoot = Join-Path (Get-Location).Path "dist/offline-$offlineRun"
~~~

~~~powershell
$offlineIscc = 'D:/Inno Setup 7/ISCC.exe'
~~~

~~~powershell
$offlineRedist = (Resolve-Path -LiteralPath build/qt-6.10.3/stage09-evidence/vc_redist.x64.exe).Path
~~~

The redistributable path is retained local evidence from the older workflow;
its directory name does not refer to this documentation stage. If absent, supply
the authentic locally available package matching `Installer/MsvcRuntime.lock.json`.
The build fails with actionable diagnostics for missing/mismatched inputs.

~~~powershell
cmake --install $offlineBuild --config Release --prefix $offlineStage
~~~

The absolute prefix is new. CMake's Qt QML deployment supplies `bin/`,
`plugins/`, `qml/`, `translations/` and relative `bin/qt.conf`, including
`qwindows.dll` and `qsqlite.dll`. `NO_COMPILER_RUNTIME` leaves MSVC CRT delivery
to the installer. Do not use the development-runtime target for staging.

The reviewed [inventory](../../Installer/payload-files.txt) currently has 148
application/Qt files. Embedded QML resources are allowed; loose C++ sources,
tests, PDBs and internal documentation are not. Validation checks identity,
exact paths/hashes, x64 Release PEs, Qt version, reparse points and streams.
The common stage alone is not a complete clean-machine distribution.
DXC/DXIL warnings and ICU/Mesa/D3D provenance remain open acceptance/license
items. Keep logs/probes outside the stage and `Installer/output/`.

### Windows Identity Verification

Root `project(JobTracker VERSION ...)` owns the application and Setup version.
Windows numeric metadata adds `.0`. The binary stays `JobTrackerApp.exe`;
QML module version is independent. Generated RC/CMake/Inno identity lives under
`<build>/Installer/<configuration>/`. Reconfigure and rebuild after version/icon
changes; never edit generated identity. `Installer/JobTracker.ico` is the current
temporary seven-size icon; publisher is blank and builds are unsigned by default.

Use the built PE version to select the output filename:

~~~powershell
$offlineVersion = (Get-Item -LiteralPath "$offlineBuild/Release/JobTrackerApp.exe").VersionInfo.ProductVersion.Trim()
~~~

~~~powershell
$offlineSetup = Join-Path (Get-Location).Path "Installer/output/JobTracker-Setup-$offlineVersion.exe"
~~~

Visual icon/Explorer/taskbar acceptance remains manual. Signing is optional and
does not clear the publication hold.

<a id="windows-offline-installer-stage-04"></a>

### Windows Offline Installer

Build only after staging. Before rebuilding a version, preserve the existing
same-name EXE outside customer output, verify its hash and resolved destination,
and move only that exact owned file. Never overwrite it, delete evidence, or
move a whole directory. The wrapper refuses an existing candidate.

~~~powershell
pwsh -NoProfile -File Installer/Build-Installer.ps1 -BuildDirectory $offlineBuild -StageDirectory $offlineStage -IsccPath $offlineIscc -RedistributablePath $offlineRedist
~~~

The single output is `Installer/output/JobTracker-Setup-<version>.exe`, a native
AMD64/PE32+ Setup. The wrapper validates the direct stage, snapshots exact local
files, validates MSVC inputs, and compiles hashed explicit file entries. It does
not configure/build the application, deploy Qt, install, publish or download.
There is no archive prerequisite or Online/Local release-mode switch.

The command prints `Private build evidence: <directory>`. Enter that exact path
below; do not select an arbitrary newest directory or substitute old metadata:

~~~powershell
$offlineEvidence = (Resolve-Path -LiteralPath (Read-Host 'Paste the Private build evidence path printed by this build')).Path
~~~

~~~powershell
$offlineMetadata = Join-Path $offlineEvidence 'installer-build.json'
~~~

Private snapshots, logs, includes and JSON stay under
`<build>/Installer/work/<id>/`. They are not customer artifacts. Every test below
must use the matching Setup, metadata and source state; changed inputs need a
new candidate and matching evidence.

### Windows Hybrid MSVC Runtime

CurrentUser receives ten verified App-Local x64 Release DLLs beside the EXE.
AllUsers checks system x64 runtime registry/DLL identity and uses the embedded
official redistributable only if insufficient. Failure/cancellation stops before
application copying; restart-required results require reboot and rerunning Setup.
The shared runtime is neither rolled back nor removed with JobTracker.

`Installer/Prepare-MsvcRuntime.ps1` checks pinned signatures, hashes, versions,
sizes and x64 CRT identity against the selected Visual Studio kit. The lock URL
is provenance, not a download route. The official x64 prerequisite's signed x86
bootstrapper is distinct from the required native x64 JobTracker Setup.

Optional standalone input preparation (fresh output, no Setup or prerequisite
execution; the normal installer wrapper already performs these checks):

~~~powershell
pwsh -NoProfile -File Installer/Prepare-MsvcRuntime.ps1 -BuildDirectory $offlineBuild -RedistributablePath $offlineRedist -OutputDirectory "$offlineRoot/runtime-input-check"
~~~

<a id="offline-deployment-test-commands-stage-08"></a>
<a id="windows-release-integration-verification"></a>

### Offline Deployment Tests

These independent consumers never become production CMake dependencies. Run
one command at a time and retain each final exit code. `ManifestPath` is the
legacy parameter name for private offline JSON, not an application archive.

Direct payload rejection checks use copies and preserve the original stage:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-OfflinePayload.ps1 -BuildDirectory $offlineBuild -StageDirectory $offlineStage -WorkDirectory "$offlineRoot/payload-tests"
~~~

Static compiled-artifact, snapshot, identity and local-embedding checks:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-OfflineInstaller.ps1 -BuildDirectory $offlineBuild -InstallerPath $offlineSetup -MetadataPath $offlineMetadata -WorkDirectory "$offlineRoot/artifact-tests"
~~~

The native runtime harness extracts/verifies the embedded prerequisite without
executing it; result/reboot assertions are policy tests:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-MsvcRuntime.ps1 -BuildDirectory $offlineBuild -InstallerDirectory $offlineEvidence -RedistributablePath $offlineRedist -WorkDirectory "$offlineRoot/runtime-tests" -IsccPath $offlineIscc
~~~

Default policy, process-closure and data suites use isolated synthetic fixtures.
They need the non-elevated account's HKCU/shell-folder access. Host-tool permission
is distinct from OS administrator elevation:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/policy" -IsccPath $offlineIscc
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ManualClose.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/manual-close" -IsccPath $offlineIscc
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/data" -IsccPath $offlineIscc
~~~

### Isolated Staging Smoke Check

The standalone project runs a helper from a private copy with system-only PATH
and cleared Qt/QML overrides. It checks Windows platform/QML imports, SQLite
and SVG/JPEG without constructing the production application or touching its
data. A developer host can still mask system dependencies.

~~~powershell
cmake -S tests/deployment -B "$offlineRoot/deployment-tests" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:/Qt_install/6.10.3/msvc2022_64 "-DJOBTRACKER_STAGE_DIR=$offlineStage"
~~~

Adapt the Qt kit path on another host. This host's standalone Visual Studio
configure previously needed host-tool access for MSBuild SDK lookup. Every
CMake build needs elevated tool access from its first attempt:

~~~powershell
cmake --build "$offlineRoot/deployment-tests" --config Release
~~~

~~~powershell
ctest --test-dir "$offlineRoot/deployment-tests" -C Release --output-on-failure
~~~

Separate App-Local loading check against this candidate's verified CRT inputs:

~~~powershell
pwsh -NoProfile -File tests/deployment/Verify-Staging.ps1 -StageDir $offlineStage -Probe "$offlineRoot/deployment-tests/Release/JobTrackerDeploymentSmoke.exe" -WorkRoot "$offlineRoot/app-local-smoke" -MsvcRuntimeDirectory "$offlineEvidence/msvc-runtime"
~~~

<a id="windows-offline-installation-lifecycle-stage-06"></a>
<a id="windows-installer-upgrade-and-coexistence-policy"></a>
<a id="windows-installer-manual-closure"></a>

### Windows Offline Installation Lifecycle

Real owned CurrentUser integration is opt-in. Use a non-elevated token, no
existing JobTracker registration/shortcuts, fresh repository-owned fixtures and
the matching Setup/JSON. Do not start another real-install suite until the prior
owned fixture is uninstalled. Never run these against a personal installation.
The suites do not launch JobTracker; their sentinels and synthetic version
identities do not establish real database/CV/settings compatibility.

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/policy-integration" -IsccPath $offlineIscc -RunCurrentUserIntegration
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-ManualClose.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/manual-integration" -IsccPath $offlineIscc -RunCurrentUserIntegration
~~~

Opposite-scope installs reject all versions, including equal versions. Switch
scope by manual uninstall with data preserved, then reinstall. Downgrade,
inconsistent registry/PE/marker identity and unsafe relocation fail closed.
Manual-close guards never terminate or restart JobTracker.

Earlier error-32 failures remain unresolved. If one recurs, retain the exact
fixture/logs and diagnose file locks at that path; do not blindly retry,
force-close processes, weaken guards or reuse a historical recovery script.
A passing retry is not a root-cause fix or an atomic rollback guarantee.

### Windows Safe Uninstall And Personal Data

Only fresh owned synthetic trees below `dist/` are eligible for test deletion.
The native data harness exercises ownership/paths/links/ACL/handles; callback
account/scope/dialog cases are simulated. Actual CurrentUser install/repair/
silent uninstall can be enabled with a fresh fixture:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/data-integration" -IsccPath $offlineIscc -RunCurrentUserIntegration
~~~

Production defaults to preserving data. Only verified non-elevated CurrentUser
uninstall can offer default-No consent after complete program removal; explicit
Yes deletes the owned `%LocalAppData%/JobTracker` tree. Silent, elevated/AllUsers,
incomplete, inaccessible or ambiguous cases preserve data. AllUsers never
enumerates profiles for cleanup. There is no command-line data-delete switch.

### Local Installer Lifecycle And Language

`Test-LocalInstaller.ps1` is the retained owned-fixture helper name, not a Local
release mode. Run each action separately with the same account, Setup, JSON and
fixture. Optional Russian/Unicode-path coverage:

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Install -Language russian -WithDesktopShortcut
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Reinstall -Language russian
~~~

~~~powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath $offlineSetup -ManifestPath $offlineMetadata -WorkDirectory "$offlineFixtureRoot/Установка с пробелами" -Scope CurrentUser -Action Uninstall -Language russian
~~~

The helper also supports `Verify`, `ScopeConflict` and `AllUsers`. Actual AllUsers,
UAC, missing/old/sufficient runtime, cancellation/reboot, interactive cleanup and
real app/data acceptance require disposable Windows 10/11 x64 environments
with networking disconnected and no Qt/VS. Follow the
[acceptance checklist](../../tests/deployment/Windows-Release-Checklist.md) and
[matrix](../../Tasks/Offline-Installer/00_ACCEPTANCE_MATRIX.md). These cases remain
NOT RUN until executed; compilation, static network scans and sanitized PATH
cannot establish them.

### Optional Installer Signing

Unsigned local builds require no signing tool or certificate. For a later
owner-authorized signature, use the installed Windows SDK SignTool with an
existing code-signing certificate in the current user's certificate store.
Set these session variables to the intended values: `$JobTrackerSignTool`
(full tool path), `$JobTrackerCertificateThumbprint` (certificate thumbprint),
`$JobTrackerTimestampUrl` (the certificate provider's RFC 3161 endpoint), and
`$JobTrackerSetup` (the finished canonical `Installer/output/JobTracker-Setup-<version>.exe`).
No PFX, password, private key or secret belongs in the repository or commands.
Preserve the unsigned EXE and its `installer-build.json` in private evidence
first. Require exit code 0 after each command before proceeding:

```powershell
& $JobTrackerSignTool sign /sha1 $JobTrackerCertificateThumbprint /fd SHA256 /tr $JobTrackerTimestampUrl /td SHA256 $JobTrackerSetup
```

```powershell
& $JobTrackerSignTool verify /pa /all /tw /v $JobTrackerSetup
```

Only after signing, timestamping and verification succeed, calculate the final
digest and record it, final size, signer and timestamp in a separate private
signed-artifact receipt. The original build receipt describes the unsigned
bytes and must not be presented as the signed artifact's hash:

```powershell
Get-FileHash -LiteralPath $JobTrackerSetup -Algorithm SHA256
```

This signs only the finished Setup. Signing the application would have to occur
before staging and input hashing; signing the outer Setup does not sign its
embedded application or uninstaller. Timestamping contacts the selected service
on the signing machine; it adds no installation-time network dependency.
No signing or publication is performed by the normal build. Certificate-backed
execution remains NOT RUN until an owner supplies and authorizes that identity.
The [Microsoft SignTool reference](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool)
defines these options. Signing does not clear the redistribution hold.

## Linux Build

Linux Debug and Release presets use Ninja, a Linux host condition and exactly
Qt 6.10.3. Use a C++20 compiler, CMake 3.21+, Ninja, PowerShell and the Linux
Qt kit with Quick, Sql and (for tests) Test. Shared presets read `QTDIR`; local
presets currently name `/home/elmar/Qt/6.10.3/gcc_64`. Confirm that SDK on the
Linux host and adapt only the local path, or set `QTDIR` and use the matching
shared preset name without `-local` in each command below.

Each configuration has its own directory under `build/qt-6.10.3/`:
`linux-debug`, `linux-release`, `linux-debug-tests`, `linux-release-tests`.
Single-config `CMAKE_BUILD_TYPE` resolves to Debug or Release to match the
preset; test builds alone set `JOBTRACKER_BUILD_TESTS=ON`. Shared and local
counterparts intentionally select the same corresponding directory; do not
switch Qt kits in an existing cache.

**NOT RUN on Linux during offline stage 02:** JSON/preset inspection on Windows
does not verify Linux configuration, compilation or runtime. The following is
the Linux verification recipe for stage 11. Run from the Linux repository root
in PowerShell, one command at a time; retain each exit code and both CTest
summaries. Use the required elevated tool access for each build.

Debug production and tests:

```powershell
cmake --preset linux-debug-local
```

```powershell
cmake --build --preset linux-debug-local
```

```powershell
cmake --preset linux-debug-tests-local
```

```powershell
cmake --build --preset linux-debug-tests-local
```

```powershell
ctest --preset linux-debug-tests-local
```

Release production and tests:

```powershell
cmake --preset linux-release-local
```

```powershell
cmake --build --preset linux-release-local
```

```powershell
cmake --preset linux-release-tests-local
```

```powershell
cmake --build --preset linux-release-tests-local
```

```powershell
ctest --preset linux-release-tests-local
```

Normal Linux builds keep Qt build-tree lookup/embedded QML and do not install
or clean `deploy/`. The existing runtime install script remains explicit.
Linux packaging, startup and deployment acceptance are outside this Windows pass.

## Preset Inspection

These read-only commands inspect presets. On this Windows host, Linux configure
presets are filtered by their condition, while Linux build/test names are still
listed. Their presence in a list does not establish that they can run on Windows;
inspect the inherited Linux host condition in the JSON declarations.

```powershell
cmake --list-presets
```

```powershell
cmake --list-presets=all
```

```powershell
ctest --list-presets
```

## Windows Release Workflow

Enter through [00_RUN_NEXT.md](../../Tasks/Offline-Installer/00_RUN_NEXT.md),
select the lowest non-DONE row in [STATUS.md](../../Tasks/Offline-Installer/STATUS.md),
complete only that stage, save its report under
`For-Agent/Task-Report/offline-installer/`, update STATUS last and stop.
[Binding requirements](../../Tasks/Offline-Installer/00_REQUIREMENTS.md) govern;
[implemented behavior](windows-release.md) and dated evidence are separate.

Add current commands here with evidence or explicit NOT RUN labels. Do not
duplicate catalogs in skills or developer guides. Retired historical commands
are not part of the executable workflow. CI/CD, automatic updates, Linux
packaging, Git writes and publication are outside this refactor. Signing remains
optional and requires an authorized certificate; the normal build stays unsigned.

## Validation Policy

Select verification scope from the final diff. For a bounded code, QML, resource, storage, or runtime-behavior change, use the targeted `JobTrackerApp` command above to build the smallest affected production target that provides meaningful compile and link confidence.

Expand to the full production preset build when shared infrastructure, cross-target APIs, build-system changes, several affected modules, merge/release readiness, or a diagnosed failure requires broader coverage.

When tests are required, use the targeted test commands above to build and run the directly associated test target or CTest suite by default.

Use the full test-enabled build and registered test set when schema or migrations, storage semantics, worker/runtime or concurrency behavior, shared infrastructure, several subsystems, cross-module contracts, unexpected targeted-test failures, or merge/release readiness requires broader verification.

Do not repeat an unchanged successful configure, build, or test run without a concrete reason. Reconfigure when the build tree is missing or stale, or when CMake, presets, source registration, or build options changed.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless the change affects build behavior.

If a command fails, capture these details and diagnose before retrying or expanding scope:

- Exact command.
- Exit code.
- Key error excerpt.

After each build command, report the command and status: `success` or `error`.

For `ctest`, report the final result: `passed` or `failed`.
