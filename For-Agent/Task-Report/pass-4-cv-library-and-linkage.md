# Pass 4. CV Library And CV Linkage

Status: completed.

Scope: CV library and CV linkage only. This pass moved CV records, category/language display metadata, selected CV state, favorite toggling, result summary, and linked application lookup out of QML-owned arrays/helpers and into C++ model/controller code.

## Context Used

- `For-Agent/Task-Report/implementation-passes.md`
- `For-Agent/Task-Report/pass-1-inventory-and-contracts.md`
- `For-Agent/Task-Report/pass-3-first-domain-migration.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `src/jobs/JobApplication.h`
- `src/jobs/JobApplicationListModel.h`
- `src/jobs/JobApplicationsController.h`
- `src/app/AppBootstrap.h`
- `src/app/AppBootstrap.cpp`
- `qml/pages/CvLibraryPage.qml`
- `qml/pages/CvLibraryBrowserPane.qml`
- `qml/pages/CvLibraryPreviewPanel.qml`
- `qml/pages/DashboardRecentCvsPanel.qml`
- `qml/pages/JobDescriptionPane.qml`
- `CMakeLists.txt`

## What Changed

- Added `src/cvs/CvDocument.h`.
- Added `src/cvs/CvListModel.h`.
- Added `src/cvs/CvListModel.cpp`.
- Added `src/cvs/LinkedApplicationListModel.h`.
- Added `src/cvs/LinkedApplicationListModel.cpp`.
- Added `src/cvs/CvLibraryController.h`.
- Added `src/cvs/CvLibraryController.cpp`.
- Added `tests/cvs/CvLibraryControllerTest.cpp`.
- Updated `src/app/AppBootstrap.h`.
- Updated `src/app/AppBootstrap.cpp`.
- Updated `src/jobs/JobApplicationsController.h`.
- Updated `src/jobs/JobApplicationsController.cpp`.
- Updated `qml/pages/CvLibraryPage.qml`.
- Updated `qml/pages/CvLibraryBrowserPane.qml`.
- Updated `qml/pages/CvLibraryPreviewPanel.qml`.
- Updated `qml/pages/DashboardRecentCvsPanel.qml`.
- Updated `qml/pages/JobDescriptionPane.qml`.
- Updated `CMakeLists.txt`.

## Backend Shape

`CvDocument` is a C++ value type for one CV document. It keeps:

- stable CV id;
- file name;
- display title;
- category and category accent;
- language and language accent;
- last-modified label;
- file-size label;
- description;
- linked application ids;
- favorite state.

`CvListModel` is a `QAbstractListModel` with roles:

- `id`
- `fileName`
- `title`
- `category`
- `categoryAccent`
- `language`
- `languageAccent`
- `lastModifiedLabel`
- `fileSizeLabel`
- `description`
- `linkedApplicationCount`
- `linkedApplicationCountLabel`
- `isFavorite`

`LinkedApplicationListModel` is a `QAbstractListModel` that derives the preview list by filtering `JobApplicationListModel` rows by the selected CV id. Its roles are:

- `id`
- `jobTitle`
- `companyName`
- `statusLabel`
- `statusAccent`
- `dateLabel`
- `cvFileName`

`CvLibraryController` is the QML-facing controller. It exposes:

- `cvModel`
- `linkedApplicationsModel`
- `categorySummary`
- `cvCount`
- `selectedCvIndex`
- `selectedCvId`
- `selectedCv`
- `resultSummary`
- `selectCv(index)`
- `toggleFavorite(cvId)`
- `openCv(cvId)`

`AppBootstrap` now owns `CvLibraryController` and exposes it to QML as `cvLibraryController`.

## QML Contract

`CvLibraryPage.qml` now consumes `cvLibraryController` instead of owning a local `cvs` array or selected CV lookup.

`CvLibraryBrowserPane.qml` now consumes `cvLibraryController.cvModel` and backend category summary data. Category and language colors come from model roles.

`CvLibraryPreviewPanel.qml` now consumes `cvLibraryController.selectedCv` and `cvLibraryController.linkedApplicationsModel`. Linked application cards are derived from the job applications model through the selected `cvId`.

`DashboardRecentCvsPanel.qml` now consumes the same CV model instead of a separate hard-coded recent CV array.

`JobDescriptionPane.qml` now displays the selected application's backend `cvFileName` in the CV-used fields instead of hard-coding one CV file name.

## Linkage Rule Preserved

Job applications continue to store a stable `cvId` and visible `cvFileName`.

The CV preview does not duplicate linked jobs in QML. It asks C++ for applications linked to the selected CV by matching the selected CV id against each job application's `cvId`.

## What Intentionally Did Not Change

- No storage layer was added.
- No file-system open behavior was implemented beyond the `openCvRequested(filePath)` signal.
- No CV import/export, parsing, or PDF preview rendering was added.
- No functional search, category filter, language filter, sort, or reset behavior was added.
- No companies or contacts migration was started.
- No broad dashboard statistics migration was started.
- No full job edit/detail migration was completed; only the visible CV-used value was tied to backend selected-application data.

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
   - Result: passed, 2/2 tests.

## Manual Checks

- Launch `build/windows-debug/Debug/JobTrackerApp.exe`.
- Open the CV Library page.
- Select each CV row and confirm the preview updates.
- Confirm the linked applications in the preview match the selected CV.
- Open Dashboard and confirm the Recent CVs table still renders.
- Open Job Applications, select different rows, open the Job Description view, and confirm the CV-used field matches the selected application.
- Confirm sidebar navigation still works and closing the app exits normally.

## Remaining Pass Boundary

Pass 4 migrated only the CV library and CV linkage slice. Functional filtering/search/sorting/validation, durable storage, CV file opening, dashboard aggregation, companies, and contacts remain for later explicit passes.

Do not start Pass 5 until explicitly requested.
