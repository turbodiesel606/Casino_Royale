# JobTracker

JobTracker is a C++20 / Qt Quick desktop application for tracking job
applications, linked CV documents, companies and dashboard summaries, with
SQLite persistence. It is under active development.

## Requirements

- Exactly Qt 6.10.3 with Quick/QML and Sql; Qt Test only for optional test builds.
- CMake 3.22 or newer.
- Windows 10/11 x64: Visual Studio 2022 / MSVC v143 and the Qt `msvc2022_64` kit.
- Linux: Ninja and a C++20 compiler with the Qt `gcc_64` kit. Native DEB
  packaging currently supports Ubuntu 24.04 amd64 only.

## Everyday Debug workflow

Run from the project root. Set `QTDIR` to your installed **kit** directory
(containing `bin/` and `lib/cmake/`). These paths are illustrative:

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
cmake --preset windows-debug
cmake --build --preset windows-debug --target JobTrackerApp
# When tests are required:
cmake --preset windows-debug-tests
cmake --build --preset windows-debug-tests --target JobTrackerTests --parallel 4
ctest --preset windows-debug-tests
# After the final changes and successful checks:
cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime
# Launch only when needed:
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
cmake --preset linux-debug
cmake --build --preset linux-debug --target JobTrackerApp --parallel 1
# When tests are required:
cmake --preset linux-debug-tests
cmake --build --preset linux-debug-tests --target JobTrackerTests --parallel 1
ctest --preset linux-debug-tests
# After the final changes and successful checks:
cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime --parallel 1
# Launch only when needed:
./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp
```

Run each command separately and check its exit status. Shared presets are the
common workflow. Optional `CMakeUserPresets.json` overrides stay local and
ignored; they are not required. Do not change Qt kits inside an existing cache.
Use parallelism suitable for available memory; `--parallel 1` is conservative
for Linux hosts. Routine validation uses Debug on the current OS. Production
presets keep tests OFF; test presets use separate build directories. Build
selected tests by replacing `JobTrackerTests` with a target such as
`JobTrackerCommonTests`, then use `ctest --preset <test-preset> -R '^JobTrackerCommonTests$'`.

`JobTrackerApp` compiles/links only, so intermediate edit/build cycles need no
deployment. After final changes and successful required checks,
`JobTrackerApp_dev_runtime` updates the usual development runtime, building the
application incrementally if needed. It preserves Windows Ctrl+F5 paths and
the Linux copy shown above. Prepare runtime earlier when needed for verification;
neither runtime target forces a full rebuild.

For an optional fresh copy, run one of these explicit targets:

```powershell
cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime_fresh
```

```bash
cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime_fresh --parallel 1
```

Each invocation deploys the current executable and SDK dependencies into a new
`<build-directory>/dev-runtime-fresh/<configuration>/<run-id>/`, prints its exact
path and launch command, and preserves earlier outputs. It does not copy an old
runtime. Linux development copies include QML debugging/profiling plugins;
`QT_QML_DEBUG` remains Debug-only. Windows test runtimes use separate directories
per target so parallel builds cannot overwrite another test's Qt files.

Launch the main GUI only when needed. Documentation/instruction-only changes
without build impact require no compilation, tests or deployment. Reuse
successful unchanged checks.

## Separate Release workflow

Release and other configurations, staging, installers and packages require an
explicit request. Use the Release commands in the [build reference](For-Agent/Docs/build.md)
and platform packaging guides; development runtime copies are not distributions.
Portable Linux wrapper invocations use `bash Installer/Linux/Stage.sh ...` and
`bash Installer/Linux/Package.sh ...`. Preserve script LF line endings; executable
bits are not required when invoking through Bash.

## Guides

- [Build configuration and targets](For-Agent/Docs/build.md), including the
  Windows/Linux development runtimes and optional test presets.
- [Windows and Linux packaging](Installer/README.md).
- [Windows installer acceptance checklist](tests/deployment/Windows-Release-Checklist.md)
  and [Linux deployment checks](tests/deployment/linux/README.md).
- [Testing policy](For-Agent/Docs/testing.md) and
  [architecture](For-Agent/Docs/architecture/overview.md).

The DEB workflow uses placeholder local-test release metadata. An
extracted-package probe does not replace installation, desktop or package
lifecycle checks.
