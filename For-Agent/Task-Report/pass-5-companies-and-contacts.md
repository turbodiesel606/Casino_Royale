# Pass 5. Companies And Contacts

Status: completed.

Scope: companies and contacts only. This pass moved company records, contact records, selected company/contact state, company-to-job links, company-to-contact links, and contact interaction history out of QML-owned arrays and into C++ model/controller code.

## Context Used

- `For-Agent/Task-Report/implementation-passes.md`
- `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`
- `For-Agent/Task-Report/pass-4-cv-library-and-linkage.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `src/jobs/JobApplicationListModel.h`
- `src/app/AppBootstrap.h`
- `src/app/AppBootstrap.cpp`
- `qml/pages/CompaniesPage.qml`
- `qml/pages/ContactsPage.qml`
- `CMakeLists.txt`

## What Changed

- Added `src/directory/Company.h`.
- Added `src/directory/CompanyListModel.h`.
- Added `src/directory/CompanyListModel.cpp`.
- Added `src/directory/Contact.h`.
- Added `src/directory/ContactListModel.h`.
- Added `src/directory/ContactListModel.cpp`.
- Added `src/directory/LinkedCompanyJobsModel.h`.
- Added `src/directory/LinkedCompanyJobsModel.cpp`.
- Added `src/directory/LinkedCompanyContactsModel.h`.
- Added `src/directory/LinkedCompanyContactsModel.cpp`.
- Added `src/directory/ContactInteractionListModel.h`.
- Added `src/directory/ContactInteractionListModel.cpp`.
- Added `src/directory/CompanyDirectoryController.h`.
- Added `src/directory/CompanyDirectoryController.cpp`.
- Added `src/directory/ContactDirectoryController.h`.
- Added `src/directory/ContactDirectoryController.cpp`.
- Added `tests/directory/DirectoryControllerTest.cpp`.
- Updated `src/app/AppBootstrap.h`.
- Updated `src/app/AppBootstrap.cpp`.
- Updated `qml/pages/CompaniesPage.qml`.
- Updated `qml/pages/ContactsPage.qml`.
- Updated `CMakeLists.txt`.

## Backend Shape

`Company` is a C++ value type for company data:

- `id`
- `name`
- `website`
- `logoText`
- `logoAccent`
- `openJobCount`
- `contactCount`
- `lastActivityLabel`
- `description`
- `notes`

`CompanyListModel` is a `QAbstractListModel` with roles:

- `id`
- `name`
- `website`
- `logoText`
- `logoAccent`
- `openJobCount`
- `openJobCountLabel`
- `contactCount`
- `contactCountLabel`
- `lastActivityLabel`
- `description`
- `notes`

`Contact` is a C++ value type for contact data:

- `id`
- `displayName`
- `initials`
- `avatarAccent`
- `roleTitle`
- `companyId`
- `companyName`
- `relatedApplicationId`
- `relatedApplicationTitle`
- `email`
- `telegram`
- `linkedin`
- `lastContactLabel`
- `notes`
- interaction history records

`ContactListModel` is a `QAbstractListModel` with roles:

- `id`
- `displayName`
- `initials`
- `avatarAccent`
- `roleTitle`
- `companyId`
- `companyName`
- `relatedApplicationId`
- `relatedApplicationTitle`
- `email`
- `telegram`
- `linkedin`
- `lastContactLabel`
- `notes`

`LinkedCompanyJobsModel` derives selected-company job rows from `JobApplicationListModel` by matching `companyId`.

`LinkedCompanyContactsModel` derives selected-company employee/contact rows from `ContactListModel` by matching `companyId`.

`ContactInteractionListModel` exposes selected-contact interaction history through roles:

- `type`
- `title`
- `timestampLabel`
- `notes`

## QML-Facing Controllers

`CompanyDirectoryController` exposes:

- `companyModel`
- `linkedJobsModel`
- `linkedContactsModel`
- `companyCount`
- `selectedCompanyIndex`
- `selectedCompanyId`
- `selectedCompany`
- `resultSummary`
- `selectCompany(index)`

`ContactDirectoryController` exposes:

- `contactModel`
- `interactionHistoryModel`
- `contactCount`
- `selectedContactIndex`
- `selectedContactId`
- `selectedContact`
- `resultSummary`
- `selectContact(index)`

`AppBootstrap` now owns and exposes:

- `companyDirectoryController`
- `contactDirectoryController`

## QML Contract

`CompaniesPage.qml` now consumes `companyDirectoryController.companyModel`, `selectedCompany`, `linkedJobsModel`, and `linkedContactsModel`.

`ContactsPage.qml` now consumes `contactDirectoryController.contactModel`, `selectedContact`, and `interactionHistoryModel`.

The remaining small QML arrays in these pages are presentation-only header labels or tab labels.

## What Intentionally Did Not Change

- No storage layer was added.
- No company/contact create, edit, delete, import, export, or persistence behavior was added.
- No functional search, filtering, sorting, pagination, or validation was added.
- No Pass 6 shared filtering/search/sort/validation service was started.
- No dashboard aggregation migration was started.
- No CV or job application behavior was broadened beyond relationship reads needed for company/contact previews.

## Verification

Commands were run from `D:\Project_CV\Root` in PowerShell.

1. `cmake --preset windows-debug-local`
   - Result: success.

2. `cmake --build --preset windows-debug-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.
   - Note: `windeployqt` emitted the existing `VCINSTALLDIR` warning, but the build completed.

3. `cmake --preset windows-debug-tests-local`
   - Result: success.

4. `cmake --build --preset windows-debug-tests-local`
   - First run result: failed because MSBuild could not access `C:\Users\elmar\AppData\Local\Microsoft SDKs` from the sandbox.
   - Rerun with required toolchain access: success.

5. `ctest --preset windows-debug-tests-local`
   - Result: passed, 3/3 tests.

## Manual Checks

- Launch `build/windows-debug/Debug/JobTrackerApp.exe`.
- Open Companies.
- Select each company row and confirm preview, linked jobs, and employees update.
- Open Contacts.
- Select each contact row and confirm preview, linked company, linked application, and interaction history update.
- Confirm Job Applications and CV Library still open normally.
- Confirm sidebar navigation still works and closing the app exits normally.

## Remaining Pass Boundary

Pass 5 migrated only companies and contacts. Shared filtering/search/sort/validation, durable storage, dashboard aggregation, editing workflows, import/export, and final cleanup remain for later explicit passes.

Do not start Pass 6 until explicitly requested.
