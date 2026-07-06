# Pass 2. Bootstrap Foundation

Status: completed.

Scope: bootstrap foundation only. No QML business logic was migrated, no QML files were changed, and no job application, CV, dashboard, company, contact, filtering, sorting, validation, or storage contracts from Pass 1 were implemented.

## Context Used

- `For-Agent/Task-Report/implementation-passes.md`
- `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `src/main.cpp`
- `CMakeLists.txt`

## What Changed

- Added `src/app/AppBootstrap.h`.
- Added `src/app/AppBootstrap.cpp`.
- Updated `src/main.cpp`.
- Updated `CMakeLists.txt`.

## Bootstrap Shape

`src/main.cpp` now:

- creates `QGuiApplication`;
- creates `AppBootstrap`;
- runs the application through `AppBootstrap::run()`;
- catches `std::exception` and unknown exceptions;
- logs startup failures with `qCritical()`;
- returns `EXIT_FAILURE` on startup failure.

`AppBootstrap` now:

- owns `QQmlApplicationEngine`;
- connects `QQmlApplicationEngine::objectCreationFailed`;
- loads `qrc:/JobTracker/qml/Main.qml`;
- throws a controlled `std::runtime_error` if the root QML object is not created;
- keeps future QML-facing controller/model construction out of `src/main.cpp`.

## What Intentionally Did Not Change

- No QML files changed.
- No QML-owned mock data moved to C++.
- No models, controllers, services, storage classes, or domain value types were added.
- No QML context properties or type registrations were added yet.
- No visual styling changed.
- No tests were added because this pass did not add business logic, model roles, validation, parsing, storage, or algorithms.

## Verification

Commands were run from `D:\Project_CV\Root` in PowerShell.

1. `cmake --preset windows-debug-local`
   - Result: success.
   - Note: CMake reported missing `WrapVulkanHeaders`, but configuration completed and generated build files.

2. `cmake --build --preset windows-debug-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.
   - Output executable: `build/windows-debug/Debug/JobTrackerApp.exe`.
   - Note: `windeployqt` completed. It emitted a warning that `VCINSTALLDIR` was not set while deploying, but the build command still completed successfully.

## Manual Checks

- Launch `build/windows-debug/Debug/JobTrackerApp.exe`.
- Confirm the main window opens.
- Confirm navigation still works.
- Confirm closing the app exits normally.

## Next Pass Boundary

Pass 3 should migrate the first domain only, preferably job applications, using the contracts in `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`.

Do not start Pass 3 until explicitly requested.
