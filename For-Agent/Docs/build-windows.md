# Windows Build Reference

Read this file for Windows toolchain, preset, command, and runtime details.
Use [build.md](build.md#routine-codex-workflow) to decide which actions are
required and in what order; this catalog does not require every command.

## Toolchain And SDK

Require Windows 10/11 x64, C++20, CMake **3.22+**, Visual Studio 2022 / MSVC v143,
and exactly **Qt 6.10.3** with the `msvc2022_64` kit. Production requires
Quick/QML and Sql; optional test builds also require Qt Test.
CMake 3.22 is the intended minimum, not a claim of execution on that version;
preset schema 3 is compatible with it.

Use PowerShell from the repository root. For a shared preset, an illustrative
kit selection is:

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
```

Apply the shared [local/SDK selection rules](build.md#shared-presets-and-local-qt-selection).
The examples below use shared presets; substitute corresponding `*-local`
names throughout when those presets exist and match the installed kit.
Local and shared variants use the same output directories.

## Presets And Configurations

Visual Studio presets select x64; each build preset selects Debug or Release.
`QT_QML_DEBUG` and MSVC Just My Code stay Debug-only.

| Configure/build preset | Directory below `build/qt-6.10.3/` | Configuration | Tests |
| --- | --- | --- | --- |
| `windows-debug` | `windows-debug` | Debug (multi-config) | OFF |
| `windows-debug-tests` | `windows-debug-tests` | Debug (multi-config) | ON |
| `windows-release` | `windows-release` | Release (multi-config) | OFF |
| `windows-release-tests` | `windows-release-tests` | Release (multi-config) | ON |

The two test presets also have matching CTest presets and separate build
directories. The routine local names are `windows-debug-local` and, when needed,
`windows-debug-tests-local`. Release and other configurations retain the
[explicit-request condition](build.md#routine-codex-workflow).

## Build And Test Commands

Application configure, when required:

```powershell
cmake --preset windows-debug
```

Compile/link before required intermediate tests; skip this separate build in
the no-test branch of the [routine workflow](build.md#routine-codex-workflow):

```powershell
cmake --build --preset windows-debug --target JobTrackerApp
```

Selected tests, when required by the [test map and scope](testing.md#subsystem-to-test-map):

```powershell
cmake --preset windows-debug-tests
cmake --build --preset windows-debug-tests --target JobTrackerCommonTests
ctest --preset windows-debug-tests -R "^JobTrackerCommonTests$"
```

Ordinary runtime preparation (the only production build in the no-test branch):

```powershell
cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime
```

Only after that command succeeds, launch when required by the task:

```powershell
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

Replace `<preset>` with a compatible preset and `<test-target>` with an actual
target from the test map. Configure with `cmake --preset <preset>` under the
shared [configuration-freshness rules](build.md#action-and-target-selection).

| Action | Command |
| --- | --- |
| Application compile/link only | `cmake --build --preset <preset> --target JobTrackerApp` |
| Ordinary local runtime | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime` |
| Fresh local runtime | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime_fresh` |
| Full configured build | `cmake --build --preset <preset>` |
| All tests in a test-enabled preset | `cmake --build --preset <preset> --target JobTrackerTests --parallel 4` |
| Selected tests in a test-enabled preset | `cmake --build --preset <preset> --target <test-target>` |
| Selected CTest run | `ctest --preset <preset> -R "<filter>"` |
| Full relevant CTest suite | `ctest --preset <preset>` |

The full configured build is not the command for building just the full test
suite. Building test targets does not run CTest. Choose a suitable `--parallel`
value for available memory.

### Other Windows Configurations

Generated Visual Studio projects also expose RelWithDebInfo and MinSizeRel.
There are no dedicated presets. For an explicit request, configure the tests-OFF
`windows-release` (or local equivalent), then use
`cmake --build build/qt-6.10.3/windows-release --config <configuration> --target <target>`.
Select the target from the table above; the executable is under
`build/qt-6.10.3/windows-release/<configuration>/JobTrackerApp.exe`.

## Windows Development Runtime And Startup

Prepare each configuration after its first build or a Qt/QML dependency change,
following the shared workflow's ordinary/final preparation rules. This also
supports explicitly requested Release work and Visual Studio Ctrl+F5.

`JobTrackerApp_dev_runtime` builds the application if needed and uses the
matching SDK's `windeployqt` for Qt/QML/platform/SQLite dependencies beside the
executable, without compiler runtime deployment. It prepares a local development
runtime, not a redistributable Release package.

Configuration requires `windeployqt.exe` from the selected Qt kit's `bin/`
directory and validates an existing `WINDEPLOYQT_EXECUTABLE` cache value.
A missing or mismatched tool fails with a diagnostic. For a stale cached path,
reconfigure with `cmake --preset <preset> -UWINDEPLOYQT_EXECUTABLE`; no global
PATH/QTDIR or SDK changes are needed.

For an explicitly prepared application runtime in a test-enabled tree,
substitute that tree's directory in the ordinary launch path. See
[runtime selection](build.md#development-runtime-selection) for freshness,
preservation, and persistent-data considerations.

### Fresh Development Runtime

When selected under the shared [fresh-runtime conditions](build.md#fresh-development-runtime),
`JobTrackerApp_dev_runtime_fresh` builds the application if needed and creates
`<build-directory>/dev-runtime-fresh/<configuration>/<UTC-time>-<random-id>/`.
The executable is at that directory's root. The helper copies only the current
executable, then invokes the validated SDK's `windeployqt` with the same options
as ordinary preparation; it never copies an old prepared runtime.
It prints the exact absolute output path and launch command.

## Test Runtime

The Windows helper copies configuration-matched Qt libraries and the SQLite
driver into each test's separate output directory:
`<build-directory>/test-runtime/<configuration>/<test-target>/`
(`sqldrivers/qsqlite[d].dll` below it). Parallel targets write disjoint runtime
files. The QML feedback test's additional Qt Quick/QML/offscreen deployment uses
that same test-specific directory. Building the named target followed by the
ordinary selected CTest command is sufficient: CTest prepares that QML runtime
before starting the feedback test. A content fingerprint covers the executable,
selected deployment tool, offscreen plugin, preparation script, and the complete
project QML subtree (`.qml`, `.js`, `.mjs`, `qmldir`, `.qmltypes`), including nested
components. Changed inputs trigger deployment without C++ recompilation; unchanged
inputs reuse it. The private `qt.conf` and cleared process-local QML/plugin import
overrides prevent SDK imports from masking missing deployed modules. No machine
environment changes are made. Direct executable invocation bypasses this CTest
preparation; use CTest for current QML verification. The existing timeout and
offscreen/software rendering settings apply to the complete test command.

QtSql loads SQLite dynamically; linking/copying QtSql alone does not supply the
driver. This preparation stays inside `JOBTRACKER_BUILD_TESTS`; it does not
deploy the production application or create a Release staging directory.
For an empty Windows CTest failure log, see
[fixture and diagnostic-log details](testing-reference.md#fixtures-and-diagnostic-logs).

## Release And Installer Navigation

For an explicit Release/staging/Setup task, use the
[Windows installer guide](installer.md#build-sequence), including its
PowerShell 7.2+ wrapper requirements. No installer preset is needed.
Use [installer-testing.md](installer-testing.md) for standalone deployment and
installer verification.
