# Pass 3. First Domain Migration

Status: completed.

Scope: first domain migration only. This pass moved the job applications list data, selection state, selected-row details, status labels, status accents, and result summary out of QML-owned JavaScript/data and into focused C++ model/controller code.

## Context Used

- `For-Agent/Task-Report/implementation-passes.md`
- `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`
- `For-Agent/Task-Report/pass-2-bootstrap-foundation.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `qml/pages/JobsPage.qml`
- `qml/pages/JobsApplicationsPane.qml`
- `qml/pages/JobDescriptionPane.qml`
- `src/app/AppBootstrap.h`
- `src/app/AppBootstrap.cpp`
- `CMakeLists.txt`

## What Changed

- Added `src/jobs/JobApplication.h`.
- Added `src/jobs/JobApplicationListModel.h`.
- Added `src/jobs/JobApplicationListModel.cpp`.
- Added `src/jobs/JobApplicationsController.h`.
- Added `src/jobs/JobApplicationsController.cpp`.
- Added `tests/jobs/JobApplicationsControllerTest.cpp`.
- Updated `src/app/AppBootstrap.h`.
- Updated `src/app/AppBootstrap.cpp`.
- Updated `qml/pages/JobsPage.qml`.
- Updated `qml/pages/JobsApplicationsPane.qml`.
- Updated `qml/pages/JobDescriptionPane.qml`.
- Updated `CMakeLists.txt`.

## Backend Shape

`JobApplication` is a C++ value type for one application record. It keeps domain fields such as company, job title, URL, work format, city, salary, status, applied date, linked CV, description, requirements, tech stack, and notes.

`JobApplicationListModel` is a `QAbstractListModel` that exposes job applications to QML through named roles:

- `id`
- `companyId`
- `companyName`
- `companyInitials`
- `companyAccent`
- `jobTitle`
- `jobUrl`
- `workFormat`
- `city`
- `salary`
- `status`
- `statusLabel`
- `statusAccent`
- `appliedDate`
- `dateLabel`
- `nextStep`
- `cvId`
- `cvFileName`
- `description`
- `requirements`
- `techStack`
- `notes`

`JobApplicationsController` is the QML-facing controller. It exposes:

- `applicationsModel`
- `applicationCount`
- `selectedApplicationIndex`
- `selectedApplicationId`
- `selectedApplication`
- `resultSummary`
- `selectApplication(index)`

`AppBootstrap` now owns `JobApplicationsController` and exposes it to QML as `jobApplicationsController` before loading `qrc:/JobTracker/qml/Main.qml`.

## QML Contract

`JobsPage.qml` now consumes `jobApplicationsController` for:

- the applications model;
- selected application state;
- selected application id;
- result summary;
- row selection commands.

`JobsApplicationsPane.qml` now consumes model roles directly instead of reading from a QML-owned JavaScript array. The old QML status accent helper was moved to C++ role data.

`JobDescriptionPane.qml` now receives `selectedApplication` from the C++ controller. The full edit/detail form is not yet completely migrated to C++ behavior.

## What Intentionally Did Not Change

- No CV library migration was started.
- No company or contact domain migration was started.
- No storage layer was added.
- No search, filtering, sorting, or validation service was added.
- No durable editing or persistence behavior was added.
- No global visual style changes were made.

## Verification

Commands were run from `D:\Project_CV\Root` in PowerShell.

1. `cmake --preset windows-debug-local`
   - Result: success.

2. `cmake --build --preset windows-debug-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

3. `cmake --preset windows-debug-tests-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

4. `cmake --build --preset windows-debug-tests-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

5. `ctest --preset windows-debug-tests-local`
   - First run result: failed with test process exit code `0xc0000135` because the test executable could not find required Qt DLLs.
   - Fix: added a Windows-only test post-build copy for Qt Core and Qt Test DLLs.
   - Rerun result: passed, 1/1 tests.

6. `git diff --check`
   - Result: success.
   - Note: Git reported line-ending warnings for tracked files, but no whitespace errors.

## Manual Checks

- Launch `build/windows-debug/Debug/JobTrackerApp.exe`.
- Open the Job Applications page.
- Select several application rows and confirm the selected row/details update correctly.
- Confirm the result summary still reflects the application count.
- Open the job description/detail flow and confirm it receives the selected application.
- Confirm other sidebar pages still open normally.
- Close the application and confirm it exits normally.

## Remaining Pass Boundary

Pass 3 migrated only the first job applications slice. The full Job Description/edit form, search/filter/sort behavior, validation, storage, CV linkage, dashboard, companies, and contacts remain for later explicit passes.

Do not start Pass 4 until explicitly requested.
