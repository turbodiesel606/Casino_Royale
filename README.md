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

## Build

Run from the project root. Set `QTDIR` to your installed **kit** directory
(containing `bin/` and `lib/cmake/`). These paths are illustrative:

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
cmake --preset windows-debug
cmake --build --preset windows-debug --target JobTrackerApp
cmake --preset windows-release
cmake --build --preset windows-release --target JobTrackerApp
```

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
cmake --preset linux-debug
cmake --build --preset linux-debug --target JobTrackerApp --parallel 1
cmake --preset linux-release
cmake --build --preset linux-release --target JobTrackerApp --parallel 1
```

Run each command separately and check its exit status. Shared presets are the
common workflow. Optional `CMakeUserPresets.json` overrides stay local and
ignored; they are not required. Do not change Qt kits inside an existing cache.
Normal builds compile/link; runtime staging and packaging are explicit steps.

## Guides

- [Build configuration and targets](For-Agent/Docs/build.md), including the
  Windows development runtime and optional test presets.
- [Windows and Linux packaging](Installer/README.md).
- [Windows installer acceptance checklist](tests/deployment/Windows-Release-Checklist.md)
  and [Linux deployment checks](tests/deployment/linux/README.md).
- [Testing policy](For-Agent/Docs/testing.md) and
  [architecture](For-Agent/Docs/architecture/overview.md).

The DEB workflow uses placeholder local-test release metadata. An
extracted-package probe does not replace installation, desktop or package
lifecycle checks.
