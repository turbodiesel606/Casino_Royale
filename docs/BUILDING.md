# Building JobTracker

The [canonical command catalog](../For-Agent/Docs/build.md) owns all configure,
build, test and packaging commands. Run each separately and wait for its final
exit code. On Windows use PowerShell from `D:/Project_CV/Root`; adapt the checkout
path on another machine. Codex must request elevated tool access for every
CMake build from the first attempt. Preserve existing build trees and logs.

## Prerequisites And Presets

- C++20, CMake 3.21+, exactly Qt 6.10.3 on Windows and Linux.
- Windows: Visual Studio 2022 / MSVC v143, x64 Qt kit.
- Linux: the existing Ninja Debug presets and the Qt 6.10.3 Linux kit.
- Qt Test only for test-enabled configurations; production defaults tests OFF.

Shared `CMakePresets.json` uses `QTDIR` as the kit root containing `bin/` and
`lib/cmake/`. `CMakeUserPresets.json` supplies local overrides; the current
Windows path is `D:/Qt_install/6.10.3/msvc2022_64` and Linux path is
`/home/elmar/Qt/6.10.3/gcc_64`. Check these on the target host. The local file is
already tracked in this workspace; its ignore rule does not remove it from Git.

| Work | Canonical instructions |
|---|---|
| Windows development | [Debug configure/build](../For-Agent/Docs/build.md#windows-build) |
| Run/debug locally | [Explicit development runtime](../For-Agent/Docs/build.md#local-development-runtime-and-startup) |
| Backend tests | [Separate Windows test tree](../For-Agent/Docs/build.md#windows-build-with-tests) and [testing policy](../For-Agent/Docs/testing.md) |
| Windows Release | [Release build/test](../For-Agent/Docs/build.md#windows-release-build-and-tests), then [release preparation](WINDOWS-RELEASE.md) |
| Linux development/tests | [Existing Linux Debug commands](../For-Agent/Docs/build.md#linux-build), NOT_RUN in the Windows release stages |

Normal builds compile/link only. Windows development runtime preparation is
opt-in; Release installation, archive and Setup generation are separate.
Production/test presets use distinct trees under `build/qt-6.10.3/`. Test helpers
must never enter a release payload. Reconfigure when build inputs/options change;
do not rebuild unchanged code merely for documentation edits.

Launching JobTracker uses persistent account data and startup CV recovery.
Use a disposable account/VM for acceptance involving personal storage. A staging
probe does not launch the application. Existing Debug/Linux workflows are
preserved; Linux packaging is deferred.

For the numbered Windows workflow, follow [00_RUN_NEXT](../Tasks/Windows-Release/00_RUN_NEXT.md)
and [STATUS](../For-Agent/Task-Report/windows-release/STATUS.md). Passing local
builds is not release approval: the [publication hold](../packaging/windows/REDISTRIBUTION.md)
still applies.
