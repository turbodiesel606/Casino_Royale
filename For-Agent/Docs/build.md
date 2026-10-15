# Build And Verification

This project uses CMake 3.21 or newer and requires exactly Qt 6.10.3 on both
Windows and Linux. Production and test discovery use the same version pin.

Use this file as the single executable command catalog. See [Building](../../docs/BUILDING.md),
[Windows Release](windows-release.md), and the [testing policy](testing.md) for
context; exact dated results remain in [stage reports](../Task-Report/windows-release/STATUS.md).

**Publication HOLD:** [R1-R7](../../packaging/windows/REDISTRIBUTION.md) remain
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
prevents new additions but cannot untrack an existing file; the stage-03 report
records the optional index-only operation for an explicitly authorized Git task.

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

The commands below are implemented forms. Their recorded Windows executions are
in [03.md](../Task-Report/windows-release/03.md) (Debug/Release and 18/18 tests
in each configuration), [13.md](../Task-Report/windows-release/13.md) (full Release
candidate/integration pass), and [14.md](../Task-Report/windows-release/14.md)
(audit/security checks). Stage 15 checked documentation, inputs and retained
evidence; it did not repeat builds or native suites. Targeted command variants
are supported by the registered targets; no fresh execution is implied.

Release examples use the exact stage-13 paths that passed. Those paths already
exist in this workspace. **For a new run, choose new staging, package, installer,
smoke and test directories and propagate them through every dependent command.**
Keep the configured production/test trees unless a fresh build is required.
Do not replay a creation command into retained evidence. Packaging and Setup
wrappers refuse existing output directories. Preserve failed runs.

Current identity: `0.1.0`, Qt `6.10.3`, Windows x64. When the project version
changes, regenerate identity and substitute the new version in all asset paths
and `-ExpectedVersion`. Do not edit generated metadata or weaken checks.

## Windows Release Build And Tests

Normal Release builds compile/link. Stage/install, archive and Setup generation
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

After building, install into a fresh **absolute** prefix:

```powershell
cmake --install build/qt-6.10.3/windows-release --config Release --prefix D:/Project_CV/Root/build/qt-6.10.3/windows-release-stage13
```

Qt's install-time QML deployment writes `bin/`, `plugins/`, `qml/`,
`translations/` and relative `bin/qt.conf`. It includes `qwindows.dll` and
`qsqlite.dll`, excluding development/tooling/input plugins and other SQL drivers.
`NO_COMPILER_RUNTIME` excludes MSVC CRTs; the installer supplies their scope
policy. Do not add the development-runtime target to this workflow.

Stage 04 verified `--config release` case-insensitively and compared three
148-file installs; exact old commands remain in [04.md](../Task-Report/windows-release/04.md).
Windows Debug installation still copies only the executable; it is not a release
payload. Linux installation remains explicit and separate from normal builds.

The current kit also deploys `icuuc.dll`, `D3Dcompiler_47.dll` and
`opengl32sw.dll`; their provenance/licensing remains subject to the release hold.
Missing `dxcompiler.dll`/`dxil.dll` warnings are not clean-machine graphics
acceptance. A development host can mask absent system dependencies.

### Windows Identity Verification

`project(JobTracker VERSION ...)` owns the three-part version; Windows numeric
versions append `.0`. The internal target/filename stay `JobTrackerApp` /
`JobTrackerApp.exe`. The QML import version is unrelated.

The generated `<build>/packaging/windows/<configuration>/JobTracker.rc`,
`JobTrackerIdentity.cmake` and `JobTrackerIdentity.iss` carry the shared identity.
`packaging/windows/JobTracker.ico` is the replaceable seven-size temporary icon.
Inspect PE icon/version metadata after a build; Explorer/taskbar/native dialog
appearance needs manual checks. Publisher is blank and signing is not implemented.
Generated identity inputs and the source ICO are not runtime assets.

### Windows Release Archive

Requires Windows, PowerShell 7.2+ and CMake 3.21+ on PATH; the recorded archive
writer was CMake 4.0.2. Use the matching production Release build and staging.
The exact reviewed allowlist is `packaging/windows/payload-files.txt`.

```powershell
pwsh -NoProfile -File packaging/windows/Package-Release.ps1 -BuildDirectory build/qt-6.10.3/windows-release -StageDirectory build/qt-6.10.3/windows-release-stage13 -OutputDirectory dist/stage13-package
```

```powershell
pwsh -NoProfile -File packaging/windows/Package-Release.ps1 -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -ExpectedVersion 0.1.0
```

Creation produces the versioned `.7z`, `.7z.sha256` and `.7z.manifest.json`.
Verification checks archive size/hash, sidecar, exact paths and every extracted
file. Outputs must be below ignored `dist/`. Repeated verification retains new
`verify-*` directories. Never publish `work/`, extracted copies, logs or generated
includes. Only approved release assets may be published after the hold is cleared.

The common archive has no compiler runtime and is not a standalone clean-machine
distribution. [Packaging contracts](../../packaging/windows/README.md) describe
the manifest and same-input determinism limits.

### Windows Version-Locked Online Installer

Requires Inno Setup 6.7.1+ (recorded compiler: 6.7.1), PowerShell 7.2+, CMake,
the selected Visual Studio installation and matching locked runtime inputs.
Supply the actual compiler path. The wrapper verifies the local manifest and
Release executable, prepares the pinned CRT inputs, then compiles Setup.

```powershell
pwsh -NoProfile -File packaging/windows/Build-Installer.ps1 -BuildDirectory build/qt-6.10.3/windows-release -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -OutputDirectory dist/stage13-online -IsccPath "D:/Inno Setup 6/ISCC.exe"
```

Online is the default mode. The fixed URL is
`https://github.com/turbodiesel606/Releases/releases/download/v<version>/JobTracker-<version>-windows-x64.7z`.
The verified archive hash/size and per-file inventory are compiled into Setup.
There is no `latest` lookup or production URL/hash override; downloaded checksum
files cannot replace the compiled trust anchor. Builds do not download, install
or publish. The public asset returned 404 in stage 13; a successful public
installation remains NOT_RUN, not a fresh network finding from stage 15.

### Windows Local-Payload Installer

Local embeds the verified common payload for local integration; both modes share
the same scope/runtime/upgrade/uninstall policy.

```powershell
pwsh -NoProfile -File packaging/windows/Build-Installer.ps1 -BuildDirectory build/qt-6.10.3/windows-release -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -OutputDirectory dist/stage13-local -IsccPath "D:/Inno Setup 6/ISCC.exe" -Mode Local
```

Outputs are `JobTracker-<version>-windows-x64-online-setup.exe` or
`JobTracker-<version>-windows-x64-local-setup.exe` plus internal
`installer-build.json`, includes, runtime inputs and `compile.log`.
Only a separately approved Setup EXE is a potential public asset.

### Windows Hybrid MSVC Runtime

`MsvcRuntime.lock.json` pins version `14.44.35211.0`, ten x64 Release CRT DLLs
and the official Microsoft package's URL, size and SHA-256. The wrapper discovers
the Visual Studio instance from CMake and validates hashes, signatures, versions
and architecture. Its optional `-RedistributablePath` selects a local input,
not a URL/hash override. Use it when the SDK copy does not match the lock;
the recorded retained input is `build/qt-6.10.3/stage09-evidence/vc_redist.x64.exe`.
Never update the lock merely to silence an unexplained mismatch.

CurrentUser copies the verified App-Local DLLs without a system prerequisite.
AllUsers checks the machine x64 runtime, then downloads/verifies/executes the
official package only if needed. Reboot-required results stop before application
files change and require a fresh Setup run after restart. Neither scope removes
the machine runtime. Detailed behavior and unresolved acceptance:
[installer guide](../../docs/WINDOWS-INSTALLER.md).

### Isolated Staging Smoke Check

This standalone project is never included by production CMake. It uses a retained
private copy, system-only PATH and cleared Qt/QML overrides to check platform/QML
imports, images and in-memory SQLite. It never starts JobTracker or accesses its
personal storage. The helper must not enter the payload.

```powershell
cmake -S tests/deployment -B build/qt-6.10.3/windows-deployment-tests -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:/Qt_install/6.10.3/msvc2022_64 -DJOBTRACKER_STAGE_DIR=D:/Project_CV/Root/build/qt-6.10.3/windows-release-stage13
```

```powershell
cmake --build build/qt-6.10.3/windows-deployment-tests --config Release
```

```powershell
ctest --test-dir build/qt-6.10.3/windows-deployment-tests -C Release --output-on-failure
```

Verify extracted files with App-Local CRTs and check the actual loaded CRT paths:

```powershell
pwsh -NoProfile -File tests/deployment/Verify-Staging.ps1 -StageDir dist/stage13-package/work/extracted -Probe build/qt-6.10.3/windows-deployment-tests/Release/JobTrackerDeploymentSmoke.exe -WorkRoot build/qt-6.10.3/stage13-smoke -MsvcRuntimeDirectory dist/stage13-online/msvc-runtime
```

Stage 13: staging CTest 1/1 and App-Local smoke passed. This does not establish
application startup, GUI, D3D12 or clean-machine behavior.

### Windows Release Integration Verification

Use [testing.md](testing.md) and the
[acceptance checklist](../../tests/deployment/Windows-Release-Checklist.md).
The following exact stage-13 invocations returned exit 0. All fixtures/logs are
retained; select fresh work directories for a new run.

Archive regression (19 checks; uses the Qt kit's Debug Core DLL as a negative
fixture):

```powershell
pwsh -NoProfile -File tests/deployment/Test-ReleasePackage.ps1 -BuildDirectory build/qt-6.10.3/windows-release -StageDirectory build/qt-6.10.3/windows-release-stage13 -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json
```

Native Online pipeline (25 checks with public safe-failure check):

```powershell
pwsh -NoProfile -File tests/deployment/Test-OnlineInstaller.ps1 -BuildDirectory build/qt-6.10.3/windows-release -InstallerDirectory dist/stage13-online -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory dist/stage13-online-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" -CheckPublicDownload
```

MSVC policy/import/download suite (15 checks; never executes the redistributable):

```powershell
pwsh -NoProfile -File tests/deployment/Test-MsvcRuntime.ps1 -BuildDirectory build/qt-6.10.3/windows-release -InstallerDirectory dist/stage13-online -RedistributablePath build/qt-6.10.3/stage09-evidence/vc_redist.x64.exe -WorkDirectory dist/stage13-runtime-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" -CheckOfficialDownload
```

Omit `-CheckPublicDownload` / `-CheckOfficialDownload` for local-only variants;
stages 10-14 record those variants separately (24 Online / 12 runtime checks).
Mock endpoint/hash inputs belong only to test harnesses. No real JobTracker
registration, application launch or personal-data access occurs in these two
harness suites.

### Windows Installer Upgrade And Coexistence Policy

Policy plus real CurrentUser integration (54 checks, twice in stage 13):

```powershell
pwsh -NoProfile -File tests/deployment/Test-InstallationPolicy.ps1 -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory dist/stage13-policy-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" -RunCurrentUserIntegration
```

Same-version repair requires a valid identity; downgrade/unsafe relocation is
refused. Opposite scopes require equal versions because they can share account
data. Unloaded/unreadable profiles block AllUsers changes. Synthetic older/newer
identities test policy, not historical application/database compatibility.

### Windows Installer Manual Closure

Manual-close suite (21 checks):

```powershell
pwsh -NoProfile -File tests/deployment/Test-ManualClose.ps1 -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory dist/stage13-manual-close-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" -RunCurrentUserIntegration
```

The synthetic x64 process fixture uses the host Framework64 C# compiler and exits
cooperatively; it does not run JobTracker. Never bypass the exact-file gate.
Unattended Setup needs `/VERYSILENT` and `/SUPPRESSMSGBOXES`. Late native copy
cancellation can briefly show progress. No full old-version rollback guarantee
or interactive Retry/other-session acceptance follows from these checks.

### Windows Safe Uninstall And Personal Data

Synthetic-data and CurrentUser suite (19 checks):

```powershell
pwsh -NoProfile -File tests/deployment/Test-PersonalData.ps1 -WorkDirectory dist/stage13-data-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -RunCurrentUserIntegration
```

Only fresh synthetic data below `dist/` is eligible for test deletion. Production
cleanup offers default-No consent only after verified non-elevated CurrentUser
uninstall. Silent, elevated/AllUsers, shared, incomplete or unverifiable cases
preserve data. There is no production command-line cleanup switch.

All three `-RunCurrentUserIntegration` suites above need the actual non-elevated
account's registry/shell-folder access. Host tool access is not Windows
administrator elevation. They refuse existing registrations and uninstall only
their verified fixture. Omit the integration switch for isolated checks.
Never start a new real-install suite until the prior fixture is removed.
The data suite can also omit `-InstallerPath`/`-ManifestPath` when integration is
not selected.

A stage-12 policy/repair sharing error 32 also occurred with the unchanged
stage-11 installer. Stage 13 passed two 54-check runs with unchanged stage-12
bytes; no lock owner or root-cause fix was established. Preserve that risk and
the earlier failed evidence.

### Local Installer Lifecycle And Language

Stage 13 passed this Russian CurrentUser sequence with a custom Unicode path.
Run each action separately, keeping installer, manifest, account and fixture
constant. Choose a new fixture root on a repeat.

```powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory "dist/stage13-currentuser-ru/Установка с пробелами" -Scope CurrentUser -Action Install -Language russian -WithDesktopShortcut
```

```powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory "dist/stage13-currentuser-ru/Установка с пробелами" -Scope CurrentUser -Action Reinstall -Language russian
```

```powershell
pwsh -NoProfile -File tests/deployment/Test-LocalInstaller.ps1 -InstallerPath dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory "dist/stage13-currentuser-ru/Установка с пробелами" -Scope CurrentUser -Action Uninstall -Language russian
```

The same helper supports `Verify` and `ScopeConflict` actions, English and
`-Scope AllUsers`. These are separate fixture scenarios; AllUsers needs an
elevated disposable account/VM and has not passed real-host acceptance. The
helper never launches JobTracker. Application startup uses persistent account
storage and must be checked in a disposable account/VM.

### Windows Redistribution Audit

Stage 14 executed these read-only-input collectors against the unchanged
stage-13 candidates. Collection success does not clear the publication hold.
The collector requires the recorded local kit, builds and artifacts and refuses
an existing output directory. Choose a fresh output path for a rerun.

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/14-evidence/Collect-PayloadAudit.ps1 -OutputDirectory dist/stage14-audit-final
```

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/14-evidence/Inspect-PrivateResources.ps1
```

The second command checks resource bytes in memory without launching the app
or saving recovered source. Its exit 0 means inspection succeeded; the
private-source criterion failed. Internal JSON contains private build paths.

Stage-14 local security regression (24 checks):

```powershell
pwsh -NoProfile -File tests/deployment/Test-OnlineInstaller.ps1 -BuildDirectory build/qt-6.10.3/windows-release -InstallerDirectory dist/stage13-online -ManifestPath dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory dist/stage14-online-tests -IsccPath "D:/Inno Setup 6/ISCC.exe"
```

Use [REDISTRIBUTION.md](../../packaging/windows/REDISTRIBUTION.md) for R1-R7 and
[14.md](../Task-Report/windows-release/14.md) for the dated audit.

## Linux Build

Existing Linux Debug presets use Ninja, a Linux host condition and exactly Qt
6.10.3. Shared presets use `QTDIR`; local presets currently name
`/home/elmar/Qt/6.10.3/gcc_64`. Confirm/install that SDK on Linux and adapt the
local path to that host. Versioned directories are
`build/qt-6.10.3/linux-debug` and `build/qt-6.10.3/linux-debug-tests`.

These existing commands are preserved from `BuildCommands.txt` and match the
preset declarations. **NOT_RUN on Linux in the Windows release workflow.**
Run from the Linux repository root in PowerShell, one at a time:

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

Normal Linux builds keep Qt build-tree lookup/embedded QML and do not install
or clean `deploy/`. The existing runtime install script remains explicit.
Linux packaging, startup and deployment acceptance are outside this Windows pass.

## Preset Inspection

These read-only commands inspect presets. On this Windows host, Linux configure
presets are filtered by their condition, while Linux build/test names are still
listed. Their presence in a list does not establish that they can run on Windows;
inspect the inherited Linux host condition in the JSON declarations.

```powershell
cmake --list-presets=all
```

```powershell
ctest --list-presets
```

## Windows Release Workflow

For a numbered stage, enter through [00_RUN_NEXT.md](../../Tasks/Windows-Release/00_RUN_NEXT.md),
select the lowest non-DONE row in [STATUS.md](../Task-Report/windows-release/STATUS.md),
complete only that stage, save its report, then update STATUS and stop.
[Accepted requirements](windows-release-requirements.md) are distinct from
[implemented behavior](windows-release.md) and dated acceptance evidence.

Add implemented commands here with actual evidence or an explicit NOT_RUN label.
Do not duplicate executable command catalogs in skills or developer guides.
CI/CD, Linux packaging, automatic updates and signing remain deferred. No stage
completion authorizes Git writes or publication.

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
