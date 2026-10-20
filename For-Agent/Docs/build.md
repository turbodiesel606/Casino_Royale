# Build And Verification

This is the authoritative reference for application configuration, presets,
targets and development runtime preparation. Use the [testing policy](testing.md)
for test selection and the [installer navigation](../../Installer/README.md) for
packaging and platform deployment checks.

## Routine Codex Workflow

For everyday code implementation and verification, use **Debug only** on the
current execution host. Select the matching `*-local` preset when it exists
and points to the installed Qt kit; otherwise use the shared preset with
`QTDIR` configured as described below.

- Build the production application with tests OFF. On Windows and Linux, use
  `JobTrackerApp_dev_runtime` as the routine application build target: it builds
  `JobTrackerApp` and prepares its Qt/QML/platform/SQLite runtime together,
  even when a GUI launch is not needed. Windows prepares dependencies beside
  the executable; Linux prepares an application copy under the selected build
  tree's `dev-runtime/<configuration>/` directory.
- When the [testing policy](testing.md) requires tests, keep that production
  build and additionally build/run the affected tests in the separate Debug
  test preset. Windows test targets automatically copy their required Qt
  libraries and SQLite plugin. Preparing the application runtime does not
  build or run tests, and preparing test dependencies does not deploy the
  production application. If launching the application from a test tree on
  either platform, also build `JobTrackerApp_dev_runtime` in that tree. Linux
  test executables continue to use their configured SDK runtime; the deployed
  application copy is separate from the test executables.
- Broader verification expands production targets or test coverage within
  Debug, including the full relevant Debug suite when required. It does not
  automatically add Release or another operating system.
- Release builds and Release tests require an explicit user request. Other
  configurations, staging, and packaging also require an explicit user request.
  Their commands remain in the reference tables; do not select them for the
  day-to-day code workflow or infer Release coverage from broader tests or
  a Debug failure.
- Documentation/instruction-only changes without build impact and read-only
  code investigations do not trigger builds or application launches. An
  explicitly requested build/test investigation follows its requested scope.

On Windows, the routine presets are `windows-debug-local` and, when needed,
`windows-debug-tests-local`; their shared equivalents omit `-local`. On Linux,
use `linux-debug-local` and `linux-debug-tests-local`, or their shared
equivalents. Production and test build directories stay separate.

Use [test-and-review](../../.agents/skills/test-and-review/SKILL.md) for required
verification and reuse of successful unchanged evidence. These workflow rules
choose the configuration and runtime preparation; they do not require every
command in this reference to run for every task.

## Toolchain And Execution

Require C++20, CMake **3.22+**, and exactly **Qt 6.10.3**. Production requires
Quick/QML and Sql; optional test builds also require Qt Test. Windows 10/11 x64
uses Visual Studio 2022 / MSVC v143 with Qt's `msvc2022_64` kit. Linux uses Ninja
and the `gcc_64` kit. Native DEB support is limited to Ubuntu 24.04 amd64.

Run from the repository root, one configure/build/test command at a time,
checking the final exit code before continuing. Use PowerShell on Windows and
a native shell such as Bash on Linux. Apply the tool-access, process-wait and
failure-handling rules in
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md).

CMake 3.22 is the intended minimum, not a claim of execution on that version.
Preset schema 3 is compatible with that minimum.

## Shared Presets And Local Qt Selection

`CMakePresets.json` is maintained and portable. Its `CMAKE_PREFIX_PATH` reads
`QTDIR`, which must name the kit containing `bin/` and `lib/cmake/`, not the
parent Qt version directory. Illustrative selections:

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
```

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
```

These assignments apply to the current shell and its child processes. With
shared presets, make `QTDIR` available when configuring; it does not need to be
set again before every build in the same shell session.

The real `CMakeUserPresets.json` is optional, local and ignored. Preserve local
preferences on both platforms; common commands do not depend on them. Local
presets may inherit a shared preset and override `CMAKE_PREFIX_PATH`. User
presets implicitly include the shared file. An inherited local preset normally
shares its parent's build directory: do not switch SDKs in a populated cache.
Inspect `CMAKE_PREFIX_PATH`, `Qt6_DIR` and the resolved Qt component paths after
configuration. If an incompatible cache cannot safely be reused, preserve it
and configure the shared preset with an explicit fresh `-B build/<new-name>`;
then build that exact directory rather than the preset's original directory.

A local preset that explicitly sets `CMAKE_PREFIX_PATH` to its Qt kit does not
require `QTDIR` on either platform. The `*-local` examples below assume matching
configure/build/test presets exist in your `CMakeUserPresets.json`; check with
`cmake --list-presets=all`. Their Qt paths remain local to that file.

| Configure/build preset | Directory below `build/qt-6.10.3/` | Configuration | Tests | DEB |
| --- | --- | --- | --- | --- |
| `windows-debug` | `windows-debug` | Debug (multi-config) | OFF | N/A |
| `windows-debug-tests` | `windows-debug-tests` | Debug (multi-config) | ON | N/A |
| `windows-release` | `windows-release` | Release (multi-config) | OFF | N/A |
| `windows-release-tests` | `windows-release-tests` | Release (multi-config) | ON | N/A |
| `linux-debug` | `linux-debug` | Debug | OFF | OFF |
| `linux-debug-tests` | `linux-debug-tests` | Debug | ON | OFF |
| `linux-release` | `linux-release` | Release | OFF | OFF |
| `linux-release-tests` | `linux-release-tests` | Release | ON | OFF |
| `linux-deb-local-test` | `linux-deb-local-test` | Release | OFF | ON, local-test ON |

Optional `windows-debug-tests`, `windows-release-tests`, `linux-debug-tests`
and `linux-release-tests` have matching configure/build/test presets and
separate directories of the same names. Only these set tests ON. Ordinary
Linux test presets inherit explicit DEB OFF from `linux-base`; packaging has
its own production cache. Reconfiguring `linux-release` explicitly restores
DEB OFF even if an earlier invocation enabled it. Preserved stale CPack files
are not authority to package an ordinary cache; the wrapper checks the live cache.

## Build And Launch Command Tables

The tables cover all nine shared build presets and the eight matching local
variants currently defined for this project. Choose the row for the requested
platform/configuration; these are alternatives, not a checklist. All commands
run from the repository root. Configure after CMake/preset/options/source
registration changes or when the build tree is absent or stale, then build.
Run the launch command only when application startup or GUI verification is
part of the task. Local and shared variants use the same output directories.

Test-preset rows below build the application in a test-enabled tree; building
and running the tests requires the separate [test commands](#optional-tests).
Keep routine production verification in its tests-OFF tree.

Launching uses the account's persistent JobTracker data, including startup CV
recovery. Use a disposable account when personal data must remain untouched.
Build/runtime preparation and application startup are separate evidence.

### Windows Build

Use PowerShell. Visual Studio presets select x64; each build preset selects
Debug or Release. `QT_QML_DEBUG` and MSVC Just My Code stay Debug-only.

| Preset / tests | Configure | Build application and deploy Qt runtime | Launch application |
| --- | --- | --- | --- |
| `windows-debug` / OFF | `cmake --preset windows-debug` | `cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe` |
| `windows-debug-local` / OFF | `cmake --preset windows-debug-local` | `cmake --build --preset windows-debug-local --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe` |
| `windows-debug-tests` / ON | `cmake --preset windows-debug-tests` | `cmake --build --preset windows-debug-tests --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-debug-tests\Debug\JobTrackerApp.exe` |
| `windows-debug-tests-local` / ON | `cmake --preset windows-debug-tests-local` | `cmake --build --preset windows-debug-tests-local --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-debug-tests\Debug\JobTrackerApp.exe` |
| `windows-release` / OFF | `cmake --preset windows-release` | `cmake --build --preset windows-release --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release\Release\JobTrackerApp.exe` |
| `windows-release-local` / OFF | `cmake --preset windows-release-local` | `cmake --build --preset windows-release-local --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release\Release\JobTrackerApp.exe` |
| `windows-release-tests` / ON | `cmake --preset windows-release-tests` | `cmake --build --preset windows-release-tests --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release-tests\Release\JobTrackerApp.exe` |
| `windows-release-tests-local` / ON | `cmake --preset windows-release-tests-local` | `cmake --build --preset windows-release-tests-local --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release-tests\Release\JobTrackerApp.exe` |

### Windows Development Runtime And Startup

For local Windows launches, prepare the development runtime separately for
each configuration after its first build or a Qt/QML dependency change. This
applies to both Debug and Release, including Visual Studio Ctrl+F5 launches.
The routine Codex workflow includes this preparation in its Debug application
build command every time that build is required.

`JobTrackerApp_dev_runtime` builds the application if needed and uses the
matching SDK's `windeployqt` for Qt/QML/platform/SQLite dependencies, without
compiler runtime deployment. It preserves existing files and prepares a local
development runtime, not a redistributable Release package. A populated old
directory alone is not proof of current deployment. If `windeployqt` is missing
and the target is unavailable, report runtime preparation as incomplete.

An application-only build using `--target JobTrackerApp`, or a full build
without `--target`, does not invoke this opt-in runtime target. Neither form
stages, packages or cleans output.

### Linux Build

Use Bash or a suitable native shell on Linux. Each Ninja tree selects its
matching build type. `--parallel 1` is suitable for a memory-constrained host
and does not change preset settings.

| Preset / tests | Configure | Build application and deploy Qt runtime | Launch deployed application |
| --- | --- | --- | --- |
| `linux-debug` / OFF | `cmake --preset linux-debug` | `cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp` |
| `linux-debug-local` / OFF | `cmake --preset linux-debug-local` | `cmake --build --preset linux-debug-local --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp` |
| `linux-debug-tests` / ON | `cmake --preset linux-debug-tests` | `cmake --build --preset linux-debug-tests --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-debug-tests/dev-runtime/Debug/bin/JobTrackerApp` |
| `linux-debug-tests-local` / ON | `cmake --preset linux-debug-tests-local` | `cmake --build --preset linux-debug-tests-local --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-debug-tests/dev-runtime/Debug/bin/JobTrackerApp` |
| `linux-release` / OFF | `cmake --preset linux-release` | `cmake --build --preset linux-release --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-release/dev-runtime/Release/bin/JobTrackerApp` |
| `linux-release-local` / OFF | `cmake --preset linux-release-local` | `cmake --build --preset linux-release-local --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-release/dev-runtime/Release/bin/JobTrackerApp` |
| `linux-release-tests` / ON | `cmake --preset linux-release-tests` | `cmake --build --preset linux-release-tests --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-release-tests/dev-runtime/Release/bin/JobTrackerApp` |
| `linux-release-tests-local` / ON | `cmake --preset linux-release-tests-local` | `cmake --build --preset linux-release-tests-local --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-release-tests/dev-runtime/Release/bin/JobTrackerApp` |
| `linux-deb-local-test` / OFF, Release packaging input | `cmake --preset linux-deb-local-test` | `cmake --build --preset linux-deb-local-test --target JobTrackerApp_dev_runtime --parallel 1` | `./build/qt-6.10.3/linux-deb-local-test/dev-runtime/Release/bin/JobTrackerApp` |

### Linux Development Runtime And Startup

The routine Linux workflow requires `JobTrackerApp_dev_runtime`, even though
the original build-tree executable can use its configured SDK directly.
The target builds the application and installs the opt-in
`JobTrackerDevelopmentRuntime` component into
`<build-directory>/dev-runtime/<configuration>/`. Launch the deployed copy
shown in the table for routine application/runtime verification.

This component reuses the existing Qt QML/runtime deployment script to prepare
`bin/JobTrackerApp`, private libraries under `lib/`, plugins, QML imports,
translations and `bin/qt.conf`. CMake installs the executable with
`$ORIGIN/../lib` as its runtime library path. System libraries remain host
dependencies. The target clears an inherited `DESTDIR` for its child install
command and preserves existing output files.

The development component supports Debug and Release, including test-enabled
trees, and is excluded from full installation/CPack. It does not install
desktop integration, package notices, or write to `/opt/jobtracker`.
Release packaging keeps its existing production-build, prefix and `DESTDIR`
guards. A full build or `--target JobTrackerApp` alone does not prepare the
development runtime; select `JobTrackerApp_dev_runtime` explicitly.

### Target Selection

Replace `<preset>` with a compatible preset from the tables above and
`<test-target>` with an actual target from the [test map](testing.md#subsystem-to-test-map).
These command forms cover application-only, full and selected-test builds.
For routine Windows and Linux work, the production application build must
include runtime preparation through `JobTrackerApp_dev_runtime`.

| Scope | Windows | Linux |
| --- | --- | --- |
| Application compile/link only | `cmake --build --preset <preset> --target JobTrackerApp` | `cmake --build --preset <preset> --target JobTrackerApp --parallel 1` |
| Application with local runtime preparation | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime` | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime --parallel 1` |
| Full configured build | `cmake --build --preset <preset>` | `cmake --build --preset <preset> --parallel 1` |
| Selected tests in a test-enabled preset | `cmake --build --preset <preset> --target <test-target>` | `cmake --build --preset <preset> --target <test-target> --parallel 1` |

### Other Windows Configurations

Generated Visual Studio projects also expose RelWithDebInfo and MinSizeRel.
There are no dedicated presets for them. For an explicit request, first
configure `windows-release` or `windows-release-local` from the table above,
then select the configuration on the build command. These examples use that
existing tests-OFF build tree and prepare its matching local Qt runtime.

| Configuration | Build application and deploy Qt runtime | Launch application |
| --- | --- | --- |
| RelWithDebInfo | `cmake --build build/qt-6.10.3/windows-release --config RelWithDebInfo --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release\RelWithDebInfo\JobTrackerApp.exe` |
| MinSizeRel | `cmake --build build/qt-6.10.3/windows-release --config MinSizeRel --target JobTrackerApp_dev_runtime` | `& .\build\qt-6.10.3\windows-release\MinSizeRel\JobTrackerApp.exe` |

## Optional Tests

Production must never depend on test code, targets, Qt Test, fixtures, probes,
or test-only hooks. With tests OFF, no test-only package discovery, compilation,
registration or deployment is allowed. Standalone deployment consumers remain
separate CMake projects under `tests/deployment/`.

Select the relevant registered target/suite from [testing.md](testing.md).
Routine code work uses Debug test presets. Configure the selected test preset
using its row above. Build the selected tests, then run CTest; neither command
replaces the tests-OFF production application/runtime build.

For bounded coverage, use the following examples with the actual affected
target and CTest filter substituted from the test map:

```powershell
cmake --preset windows-debug-tests
cmake --build --preset windows-debug-tests --target JobTrackerCommonTests
ctest --preset windows-debug-tests -R "^JobTrackerCommonTests$"
```

```bash
cmake --preset linux-debug-tests
cmake --build --preset linux-debug-tests --target JobTrackerCommonTests --parallel 1
ctest --preset linux-debug-tests -R '^JobTrackerCommonTests$'
```

When matching local test presets set the Qt kit path, use
`windows-debug-tests-local` or `linux-debug-tests-local` consistently for all
three commands without setting `QTDIR`. Their Release equivalents are
`windows-release-tests-local` and `linux-release-tests-local`, reserved for
explicit Release work.

Use the full build/suite table only when the testing policy requires full
coverage. Full coverage in routine tasks still uses Debug; Release test rows
require an explicit user request. Windows test targets deploy their own
required Qt runtime and SQLite plugin; the full build does not invoke
`JobTrackerApp_dev_runtime`. Use the application/runtime table as well if
launching the application from that test tree.

| Test preset | Build full test-enabled tree | Run full suite |
| --- | --- | --- |
| `windows-debug-tests` | `cmake --build --preset windows-debug-tests` | `ctest --preset windows-debug-tests` |
| `windows-debug-tests-local` | `cmake --build --preset windows-debug-tests-local` | `ctest --preset windows-debug-tests-local` |
| `windows-release-tests` | `cmake --build --preset windows-release-tests` | `ctest --preset windows-release-tests` |
| `windows-release-tests-local` | `cmake --build --preset windows-release-tests-local` | `ctest --preset windows-release-tests-local` |
| `linux-debug-tests` | `cmake --build --preset linux-debug-tests --parallel 1` | `ctest --preset linux-debug-tests` |
| `linux-debug-tests-local` | `cmake --build --preset linux-debug-tests-local --parallel 1` | `ctest --preset linux-debug-tests-local` |
| `linux-release-tests` | `cmake --build --preset linux-release-tests --parallel 1` | `ctest --preset linux-release-tests` |
| `linux-release-tests-local` | `cmake --build --preset linux-release-tests-local --parallel 1` | `ctest --preset linux-release-tests-local` |

A zero-test selection is `no matching tests`, not PASS. Build/distribution
verification does not imply business/UI testing.

## Explicit Release Packaging

These commands belong to an explicitly requested release/packaging task.
First build the tests-OFF Release application from the command tables.
For packaging without a local GUI launch, the application-only
`--target JobTrackerApp` form is sufficient; staging prepares the release
payload separately. Follow the prerequisites and checks in the
[Windows installer guide](installer.md) or
[Linux guide](../../Installer/Linux/README.md) before these entry commands.
Windows wrappers require PowerShell 7.2+.

| Operation | Command after the corresponding Release build |
| --- | --- |
| Windows fresh Release stage | `$stageResult = & ./Installer/Stage-Release.ps1 -BuildDirectory ./build/qt-6.10.3/windows-release -PassThru` |
| Windows offline Setup from that stage | `& ./Installer/Build-Installer.ps1 -BuildDirectory $stageResult.BuildDirectory -StageDirectory $stageResult.StageDirectory` |
| Linux fresh Release stage | `Installer/Linux/Stage.sh build/qt-6.10.3/linux-release` |
| Ubuntu 24.04 amd64 local-test DEB from the dedicated preset | `Installer/Linux/Package.sh build/qt-6.10.3/linux-deb-local-test` |

The wrappers consume existing builds and preserve previous outputs. Linux
installed paths remain `/opt/jobtracker` and `/usr/share` desktop resources.
Staging uses an absolute separate `DESTDIR`, never a changed runtime prefix.
The `linux-deb-local-test` preset has its own cache and placeholder package
metadata; do not turn an ordinary production/test cache into a package build.
There is no dedicated Windows installer build preset.

## Preset Inspection And Validation

```bash
cmake --list-presets
cmake --list-presets=all
ctest --list-presets
```

Host conditions filter configure presets. A build/test name listed on another
OS does not prove it can execute there. Use
[test-and-review](../../.agents/skills/test-and-review/SKILL.md) for sufficient
verification and documentation impact. Reuse successful unchanged evidence;
rerun affected checks after changed inputs or diagnosed failures. Preserve
caches and failed output. Compile/link, package inspection, extracted runtime,
native GUI and clean-target installation are distinct results.
