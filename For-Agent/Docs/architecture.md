# Architecture

JobTracker is a Qt 6 / QML / C++ desktop application.

## Entry Points

- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.
- Executable target: `JobTrackerApp`.
- QML module URI: `JobTracker`.

## Current Shape

`src/main.cpp` is intentionally tiny. `main()` only delegates to `App::start(argc, argv)`.

`src/app/App.h` and `src/app/App.cpp` own the process-level startup boundary. `App::start()` creates `QGuiApplication`, sets the application name, constructs `AppBootstrap`, runs it, catches startup exceptions, logs failures, and returns a non-zero exit code on startup failure.

`src/app/AppBootstrap.h` and `src/app/AppBootstrap.cpp` own the application graph and QML startup wiring. `AppBootstrap` creates and owns the `QQmlApplicationEngine`, connects object creation failure handling, exposes backend controllers to QML through context properties, and loads `qrc:/JobTracker/qml/Main.qml`.

The current bootstrap dependency order is:

1. `StoragePaths`
2. `SQLiteDataBase`
3. `CvRepository`
4. `JobRepository`
5. `CvImportService`
6. `AddJobService`
7. `JobApplicationsController`
8. `CvLibraryController`
9. `DashboardController`
10. `CompanyDirectoryController`
11. `ContactDirectoryController`

Current QML context properties are:

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

`qml/Main.qml` owns the main `ApplicationWindow`, sidebar shell, page index state, Add Job form visibility, and `StackLayout` navigation.

The currently reachable main pages are:

- `DashboardPage`
- `JobsPage`
- `CvLibraryPage`
- `CompaniesPage`
- `ContactsPage`
- `JobFormPage`, opened as a form surface through Add Job actions

`SettingsPage`, `ProfilePage`, and `MemoryPage` are registered in `CMakeLists.txt`, but they are not currently reachable from the main shell. `Sidebar.qml` displays a Settings item, but only emits navigation for indices below the five main pages. The sidebar also has an Add CV quick action with no current navigation or backend workflow.

QML pages live under `qml/pages`.

Shared QML components live under `qml/components`.

Keep concrete QML file registration in `CMakeLists.txt` as the source of truth for the current file inventory.

## Persistence And Product Data

Jobs and CVs are currently backed by SQLite.

`StoragePaths` resolves the application data location, creates the managed resume folder, and provides the database path.

`SQLiteDataBase` owns the Qt SQL connection lifetime and schema migration.

Schema version 1 contains:

- `cvs`
- `jobs`
- `job_technologies`

The canonical persisted relationships are:

- Job to CV through `jobs.cv_id`.
- Job to technologies through `job_technologies.job_id`.

`CvRepository` loads and inserts CV metadata and reconstructs linked application IDs from stored jobs.

`JobRepository` loads and inserts jobs and their ordered technology rows.

Companies and contacts are not currently persisted. The production bootstrap constructs empty company/contact models and exposes directory controllers over those empty sources. Directory pages can render the structure, filters, and selection contracts, but there is no current company/contact repository, mutation path, or durable storage.

Company linkage is incomplete at the persistence boundary. Jobs store `company_name`, while the in-memory job domain still has a `companyId_` field and company-linked models depend on IDs. Persisted jobs therefore do not currently rehydrate a usable company relationship.

## Add Job Flow

Add Job is implemented as a QML-to-C++ workflow.

`JobFormPage.qml` gathers form fields and calls `jobApplicationsController.createApplication(formValues, selectedCvUrl)`.

`JobApplicationsController::createApplication()` converts the QML map into `JobApplicationDraft`, publishes `saving`, calls `AddJobService`, appends the created row on success, updates filtering/selection summaries, emits `applicationCreated`, and emits `saveFailed` with field errors on failure.

`AddJobService` owns durable Add Job behavior:

- required field validation;
- ISO-date validation;
- HTTP/HTTPS URL validation;
- technology trimming and case-insensitive deduplication;
- CV import coordination;
- job insertion;
- SQLite transaction handling;
- copied-file cleanup on ordinary failures.

`CvImportService` accepts local PDF, DOC, and DOCX files, hashes file content, deduplicates by SHA-256, copies new files into managed storage, and inserts CV metadata.

After successful job creation, `AppBootstrap` connects `JobApplicationsController::cvUsed` to `CvLibraryController::recordCvUse`, so the CV library can update linked application state in memory. On restart, CV links are reconstructed from persisted jobs.

## Boundaries

Keep QML focused on presentation, layout, navigation, binding, and simple UI state.

Place durable business logic, storage, parsing, algorithms, and application state in C++ backend classes when they are introduced.

Use `For-Agent/Docs/qml-to-cpp-extraction.md` when moving durable behavior from QML into C++.

Keep `src/main.cpp` minimal. Move non-trivial startup wiring into dedicated bootstrap/application classes when the startup surface grows.

Avoid global visual rewrites unless the task explicitly asks for a broader design change.

Avoid large monolithic pages. Extract reusable QML components when repetition becomes meaningful.

Large current QML panes that should be treated carefully before adding more behavior include:

- `JobDescriptionPane.qml`
- `JobsApplicationsPane.qml`
- `CvLibraryBrowserPane.qml`
- `ContactsPage.qml`
- `JobFormPage.qml`
- `CompaniesPage.qml`
- `CvLibraryPreviewPanel.qml`

Presentation-only state should stay in QML. Examples include current page index, form visibility, local hover/selection styling, layout geometry, popup state, custom scroll visuals, and display-only table column metadata.

Backend-owned behavior should remain in C++ when it affects validation, persistence, import, filtering, sorting, search, grouping, cross-screen data, or behavior that should be tested.

## Backend Shape

Organize backend code by responsibility:

- Domain and value types for durable product data.
- Models for list or table data consumed by QML delegates.
- Controllers or view models for QML-facing properties, commands, and signals.
- Services for validation, parsing, algorithms, and business operations.
- Storage and configuration classes for persistence and platform-aware file access.
- Bootstrap/application classes for startup wiring that does not belong in `src/main.cpp`.

QML-facing controllers should expose a small screen contract and delegate non-trivial behavior to services or models.

Current backend areas are:

- `src/app`: process startup, exception boundary, dependency construction, QML engine setup, context properties, and main QML loading.
- `src/storage`: application data paths, SQLite connection lifetime, and schema migration.
- `src/common`: reusable role-based filtering/sorting/search and validation helpers.
- `src/jobs`: job value/draft types, job list model, QML controller, repository, and Add Job service.
- `src/cvs`: CV value type, CV list model, linked-job adapter model, QML controller, repository, and managed-file import service.
- `src/dashboard`: metric and recent-item read models over jobs and CVs.
- `src/directory`: company/contact value types, empty list models, QML-facing directory controllers, and linked read models.
- `tests`: common, jobs, CVs, dashboard, directory, and storage test suites.

## Known Architecture Gaps

Treat these as current constraints when planning implementation:

- Stored CV opening is incomplete. Managed files have stored paths, but `CvLibraryController::openCv()` does not yet expose a platform-aware opener contract.
- CV favorite changes are in-memory only and are not persisted.
- Dashboard recent models currently depend on source-model ordering; appended new rows can miss recent lists after enough rows exist.
- CV import and Add Job persistence run synchronously from the QML invocation and can block the GUI thread for large files.
- Completed CV files can be orphaned if the process crashes after the file copy but before SQLite commit.
- Company/contact backend storage and mutation workflows are still shells.
- Selection is still index-based across filtering and sorting in some controllers.
- Several QML option lists use display strings that C++ also interprets, which is fragile once localization or durable option contracts are introduced.
- Add Job validation and existing selected-job validation are separate code paths and can drift.
