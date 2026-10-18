# Build And Verification

For normal application configuration, builds, development runtime preparation,
and backend tests, use this document. The [testing policy](testing.md) describes
suite responsibilities and test selection. Installer construction and standalone
deployment verification have their own [reference](installer.md) and
[test guide](installer-testing.md); consult them for that subsystem.

## Toolchain And Execution

The project requires C++20, CMake 3.21 or newer, and exactly Qt 6.10.3 on Windows
and Linux. Production needs Qt Quick and Sql; test-enabled configurations also
need Qt Test. Windows presets use Visual Studio 2022 / MSVC v143 and x64;
Linux presets use Ninja and a C++20 compiler.

Run from the actual repository root. Windows examples use PowerShell; Linux
examples use a native shell such as Bash. Follow the execution, Codex tool-access,
process-wait, and diagnosis rules in
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md).

Keep normal configure, build, and deployment independent of tests. With
`JOBTRACKER_BUILD_TESTS=OFF`, do not discover test-only packages, create/register
test targets, compile test sources, deploy test dependencies, or change the
production artifact to support tests. Keep all test-only setup in the enabled
configuration.

## Canonical Command Forms And Substitutions

The command catalog below defines supported forms, not an exhaustive allowlist
of example target names. Substitute a preset verified in `CMakePresets.json` or
`CMakeUserPresets.json`, a build target verified in `CMakeLists.txt`, and a CTest
selection matching registered tests. Use the [subsystem-to-test map](testing.md#subsystem-to-test-map)
as an initial aid, then inspect the relevant contracts and tests.

Keep production/test presets, host conditions, Qt kit, generator, and build/test
configuration consistent. Windows multi-config presets select Debug or Release;
Linux presets use the matching single-config build type. Targeted Linux or
Release checks may use the same `--target` and CTest `-R` forms as the Debug
examples with verified names. Do not invent targets, presets, scripts, or options.
A zero-test selection is `no matching tests`, not a passing suite; apply the
[outcome rules](testing.md#test-outcomes).

## Windows Build

Shared presets contain no machine-specific SDK path. They read `QTDIR` as the
Qt kit root through `CMAKE_PREFIX_PATH`; alternatively, an ignored local
`CMakeUserPresets.json` can override `CMAKE_PREFIX_PATH`. This workspace's local
Windows presets preserve `D:/Qt_install/6.10.3/msvc2022_64`. Set the path to the
Qt kit containing `bin/` and `lib/cmake/`, not to its parent version directory.

With `QTDIR` set to the intended kit, use the matching shared preset name without
`-local` in the commands below. Local and shared counterparts intentionally use
the same corresponding build directory; do not switch Qt kits in an existing
cache. Preserve machine-local overrides.

Presets use schema version 3, matching CMake 3.21; user presets implicitly
include the shared file. Windows presets select Visual Studio 2022 and x64.
Debug/Release and production/test configurations use distinct build directories
under `build/qt-6.10.3/`. Existing Debug preset names and directories are retained.
`QT_QML_DEBUG` and MSVC Just My Code remain Debug-only.

`CMakeUserPresets.json` contains machine-local overrides and is ignored for new
files, but is already tracked in this checkout. An ignore rule cannot untrack it;
preserve its values and do not change the Git index without an explicit request.

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
results and manual verification must be reported separately.

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

## Linux Build

Linux Debug and Release presets use Ninja, a Linux host condition and exactly
Qt 6.10.3. Use a C++20 compiler, CMake 3.21+, Ninja and the Linux
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

Run from the Linux repository root in a native shell such as Bash, one command
at a time. Record each exit code and CTest result; commands shown here are
procedures, not evidence that they were executed on a particular host.

Debug production and tests:

```bash
cmake --preset linux-debug-local
```

```bash
cmake --build --preset linux-debug-local
```

```bash
cmake --preset linux-debug-tests-local
```

```bash
cmake --build --preset linux-debug-tests-local
```

```bash
ctest --preset linux-debug-tests-local
```

Release production and tests:

```bash
cmake --preset linux-release-local
```

```bash
cmake --build --preset linux-release-local
```

```bash
cmake --preset linux-release-tests-local
```

```bash
cmake --build --preset linux-release-tests-local
```

```bash
ctest --preset linux-release-tests-local
```

Normal Linux builds keep Qt build-tree lookup/embedded QML and do not install
or clean `deploy/`. The existing runtime install script remains explicit.
Verify actual startup separately on the target Linux host.

## Preset Inspection

These read-only commands inspect presets. On Windows, Linux configure
presets are filtered by their condition, while Linux build/test names are still
listed. Their presence in a list does not establish that they can run on Windows;
inspect the inherited Linux host condition in the JSON declarations.

```bash
cmake --list-presets
```

```bash
cmake --list-presets=all
```

```bash
ctest --list-presets
```

## Validation Policy

Use [test-and-review](../../.agents/skills/test-and-review/SKILL.md) for integrated
verification and production-build expansion criteria. Use [testing.md](testing.md)
for test selection, quality, and expansion. Apply the appropriate targeted or full
preset command forms above to that scope; neither compile/link success nor a
listed test establishes runtime, UI, or executed-test verification.

Do not repeat an unchanged successful configure, build, or test run without a concrete reason. Reconfigure when the build tree is missing or stale, or when CMake, presets, source registration, or build options changed.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless the change affects build behavior.

Use `cmake-build-debug` for failure capture, diagnosis, process completion, and
result reporting. Check the selected Qt kit, generator/compiler, configuration,
source registration, and production/test preset before changing inputs. For a
missing runtime dependency, distinguish compile/link success from startup and
prepare the development runtime explicitly. Preserve diagnostic logs and avoid
cache deletion as a default repair.
