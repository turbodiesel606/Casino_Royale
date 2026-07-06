# Pass 6. Shared Filtering, Search, Sort, And Validation

Status: completed.

Scope: shared filtering, search, sort, and validation only. This pass introduced reusable C++ infrastructure for role-based filtering/sorting and validation, then wired existing job, CV, company, and contact screens to that infrastructure through their controllers.

## Context Used

- `For-Agent/Task-Report/implementation-passes.md`
- `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`
- `For-Agent/Task-Report/pass-5-companies-and-contacts.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `src/jobs/JobApplicationsController.h`
- `src/cvs/CvLibraryController.h`
- `src/directory/CompanyDirectoryController.h`
- `src/directory/ContactDirectoryController.h`
- `qml/pages/JobsApplicationsPane.qml`
- `qml/pages/CvLibraryBrowserPane.qml`
- `qml/pages/CompaniesPage.qml`
- `qml/pages/ContactsPage.qml`
- `CMakeLists.txt`

## What Changed

- Added `src/common/RoleFilterProxyModel.h`.
- Added `src/common/RoleFilterProxyModel.cpp`.
- Added `src/common/ValidationResult.h`.
- Added `src/common/ValidationService.h`.
- Added `src/common/ValidationService.cpp`.
- Added `tests/common/FilterAndValidationTest.cpp`.
- Updated `src/jobs/JobApplicationsController.h`.
- Updated `src/jobs/JobApplicationsController.cpp`.
- Updated `src/cvs/CvLibraryController.h`.
- Updated `src/cvs/CvLibraryController.cpp`.
- Updated `src/directory/CompanyDirectoryController.h`.
- Updated `src/directory/CompanyDirectoryController.cpp`.
- Updated `src/directory/ContactDirectoryController.h`.
- Updated `src/directory/ContactDirectoryController.cpp`.
- Updated `qml/pages/JobsApplicationsPane.qml`.
- Updated `qml/pages/JobsPage.qml`.
- Updated `qml/pages/CvLibraryPage.qml`.
- Updated `qml/pages/CvLibraryBrowserPane.qml`.
- Updated `qml/pages/CompaniesPage.qml`.
- Updated `qml/pages/ContactsPage.qml`.
- Updated `tests/jobs/JobApplicationsControllerTest.cpp`.
- Updated `tests/cvs/CvLibraryControllerTest.cpp`.
- Updated `tests/directory/DirectoryControllerTest.cpp`.
- Updated `CMakeLists.txt`.

## Backend Shape

`RoleFilterProxyModel` is a reusable `QSortFilterProxyModel` subclass for QML-facing list controllers.

It supports:

- multi-token case-insensitive search across configured roles;
- exact role filters;
- required non-empty role filters for contact channel-style filters;
- role-based sorting;
- integer sorting;
- English human-date sorting for labels like `May 12, 2026`;
- locale-aware case-folded string sorting.

`ValidationService` currently supports:

- required field validation from a `QVariantMap`;
- required or optional HTTP/HTTPS URL validation;
- result messages through `ValidationResult`.

## QML-Facing Controller Contracts

`JobApplicationsController` now exposes:

- `searchText`
- `statusFilter`
- `setSearchText(text)`
- `setStatusFilter(status)`
- `clearFilters()`
- `validateSelectedApplication()`

`CvLibraryController` now exposes:

- `searchText`
- `categoryFilter`
- `languageFilter`
- `sortMode`
- `setSearchText(text)`
- `setCategoryFilter(category)`
- `setLanguageFilter(language)`
- `setSortMode(sortMode)`
- `clearFilters()`

`CompanyDirectoryController` now exposes:

- `searchText`
- `sortMode`
- `setSearchText(text)`
- `setSortMode(sortMode)`
- `clearFilters()`

`ContactDirectoryController` now exposes:

- `searchText`
- `companyFilter`
- `channelFilter`
- `sortMode`
- `setSearchText(text)`
- `setCompanyFilter(company)`
- `setChannelFilter(channel)`
- `setSortMode(sortMode)`
- `clearFilters()`

All controllers continue to expose filtered proxy models to QML while mapping selected proxy rows back to source rows internally.

## QML Contract

`JobsApplicationsPane.qml` search and status controls now emit controller commands through `JobsPage.qml`.

`CvLibraryPage.qml` and `CvLibraryBrowserPane.qml` now bind CV search, category filter, language filter, sort mode, and reset controls to `CvLibraryController`.

`CompaniesPage.qml` now binds company search and sort controls to `CompanyDirectoryController`.

`ContactsPage.qml` now binds contact search, company filter, channel filter, and sort controls to `ContactDirectoryController`.

QML remains responsible for presentation, layout, and forwarding UI input. The filtering, sorting, selection mapping, and validation rules live in C++.

## Tests Added Or Updated

- Added `JobTrackerCommonTests` for shared proxy and validation behavior.
- Updated job controller tests for search, status filtering, and selected-application validation.
- Updated CV controller tests for search, category filtering, and linked-job sorting.
- Updated directory controller tests for company search/sort and contact company/channel/sort filters.

## What Intentionally Did Not Change

- No durable storage was added.
- No import/export behavior was added.
- No create, edit, or delete workflows were added.
- No dashboard aggregation migration was started.
- No final Pass 7 cleanup was started.
- No broad visual redesign was attempted.

## Verification

Commands were run from `D:\Project_CV\Root` in PowerShell.

1. `cmake --preset windows-debug-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

2. `cmake --build --preset windows-debug-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.
   - Note: `windeployqt` emitted the existing `VCINSTALLDIR` warning, but the build completed.

3. `cmake --preset windows-debug-tests-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

4. `cmake --build --preset windows-debug-tests-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

5. `ctest --preset windows-debug-tests-local`
   - Result: passed, 4/4 tests.

## Manual Checks

- Launch `build/windows-debug/Debug/JobTrackerApp.exe`.
- Open Job Applications.
- Type in search and confirm rows, preview, and result summary update.
- Change status filter and confirm rows and preview update.
- Open CV Library.
- Use search, category filter, language filter, sort, and reset controls.
- Open Companies.
- Use company search and sort controls, then confirm preview/linked rows follow the selected filtered row.
- Open Contacts.
- Use search, company filter, channel filter, and sort controls, then confirm preview/history follow the selected filtered row.
- Confirm sidebar navigation still works and closing the app exits normally.

## Remaining Pass Boundary

Pass 6 migrated shared filtering, search, sort, and validation only. Storage, import/export, create/edit/delete behavior, dashboard aggregation, leftover cleanup, API tightening, and final manual review remain for later explicit passes.

Do not start Pass 7 until explicitly requested.
