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

For an initial local launch, run these commands separately from the project
root. Do not switch Qt kits inside a populated cache; follow the
[local/SDK selection rules](For-Agent/Docs/build.md#shared-presets-and-local-qt-selection)
when an existing cache points to another kit.

On Windows, prefer `windows-debug-local` when `CMakeUserPresets.json` defines
matching configure/build presets pointing to the installed Qt 6.10.3
`msvc2022_64` kit. Its explicit Qt path removes the need to set `QTDIR`:

```powershell
cmake --preset windows-debug-local
cmake --build --preset windows-debug-local --target JobTrackerApp_dev_runtime
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

Without suitable local presets, use the shared `windows-debug` preset after
setting `QTDIR` in the same PowerShell session to the installed Qt 6.10.3
`msvc2022_64` kit. Adjust the illustrative path below. The shared preset reads
`QTDIR` and does not apply the Qt path from `windows-debug-local`; an unset or
outdated `QTDIR` can cause Qt discovery to fail. Both variants use the same
output directory.

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
cmake --preset windows-debug
cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

On Linux, prefer `linux-debug-local` when `CMakeUserPresets.json` defines
matching configure/build presets pointing to the installed Qt 6.10.3 `gcc_64`
kit. Its explicit Qt path removes the need to set `QTDIR`:

```bash
cmake --preset linux-debug-local
cmake --build --preset linux-debug-local --target JobTrackerApp_dev_runtime --parallel 1
./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp
```

Without suitable local presets, use the shared `linux-debug` preset after
exporting `QTDIR` in the same shell to the installed Qt 6.10.3 `gcc_64` kit.
Adjust the illustrative path below. The shared preset reads `QTDIR` and does
not apply the Qt path from `linux-debug-local`; an unset or outdated `QTDIR`
can cause Qt discovery to fail. Both variants use the same output directory.

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
cmake --preset linux-debug
cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime --parallel 1
./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp
```

For code changes, follow the canonical
[routine workflow](For-Agent/Docs/build.md#routine-codex-workflow) for action
selection and verification order. See the [target map](For-Agent/Docs/build.md#target-selection),
[Windows commands](For-Agent/Docs/build-windows.md#build-and-test-commands),
[Linux commands](For-Agent/Docs/build-linux.md#build-and-test-commands), and
[fresh runtime](For-Agent/Docs/build.md#fresh-development-runtime) for the
optional new-directory copy. Documentation-only changes without build impact
require no compilation.

## Separate Release workflow

Release and other configurations, staging, installers and packages require an
explicit request. Use the [Windows installer guide](For-Agent/Docs/installer.md#build-sequence)
or [Linux staging and DEB guide](Installer/Linux/README.md) for detailed Release
and packaging commands; development runtime copies are not distributions.
Windows staging and Setup wrappers live under `Installer/Windows/`.
Portable Linux wrapper invocations use `bash Installer/Linux/Stage.sh ...` and
`bash Installer/Linux/Package.sh ...`. Preserve script LF line endings; executable
bits are not required when invoking through Bash.

## Guides

- [Shared build workflow and action selection](For-Agent/Docs/build.md).
- [Windows build reference](For-Agent/Docs/build-windows.md) or
  [Linux build reference](For-Agent/Docs/build-linux.md) for the relevant OS's
  toolchain, presets, commands, and development/test runtime details.
- [Windows and Linux packaging](Installer/README.md).
- [Windows installer acceptance checklist](tests/deployment/Windows-Release-Checklist.md)
  and [Linux deployment checks](tests/deployment/linux/README.md).
- [Testing policy and target map](For-Agent/Docs/testing.md) for selecting,
  writing, and assessing tests; [testing reference](For-Agent/Docs/testing-reference.md)
  only for specific suite, fixture, log, or manual-scenario details.
- [Architecture](For-Agent/Docs/architecture/overview.md).

DEB presets separate placeholder local-test metadata from explicitly supplied release metadata. An
extracted-package probe does not replace installation, desktop or package
lifecycle checks.
