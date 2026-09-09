# Application Wiring And Module Shape

Read this document for process startup, bootstrap construction, application
ownership, context-property exposure, module inventory, or QML reachability.

## Entry And Startup

`src/main.cpp` is intentionally tiny. `main()` only delegates to
`App::start(argc, argv)`.

`src/app/App.hpp` and `src/app/App.cpp` own the process-level startup boundary.
`App::start()` creates `QGuiApplication`, sets the application name, constructs
`AppBootstrap`, runs it, catches startup exceptions, logs failures, and returns a
non-zero exit code on startup failure.

`src/app/AppBootstrap.hpp` and `src/app/AppBootstrap.cpp` own the application
graph and QML startup wiring. `AppBootstrap` creates and owns the
`QQmlApplicationEngine`, connects object creation failure handling, exposes
backend controllers to QML through context properties, and loads
`qrc:/JobTracker/qml/Main.qml`.

The current bootstrap dependency order is:

1. `StoragePaths`
2. `SqliteDatabase`
3. `CvRepository`
4. `CvManagedFileStore`
5. `CvFileAccessService`
6. `CompanyRepository`
7. `JobRepository`
8. `CvLockWrapper`
9. `JobSaveWorker`
10. `CvImportWorker`
11. `StorageMutationGate`
12. `DataRemovalWorker`
13. `JobApplicationsController`
14. `CvLibraryController`
15. `DashboardController`
16. `ContactListModel`
17. `CompanyDirectoryController`
18. `ContactDirectoryController`

Read [threading.md](threading.md) when changing the workers, shared CV lock,
controller queues, mutation gate, or shutdown implications of this construction
order. Read
[storage.md](storage.md) when changing bootstrap hydration or recovery.

## QML Exposure And Reachability

Current QML context properties are:

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

`qml/Main.qml` owns the main `ApplicationWindow`, sidebar shell, page index state,
Add Job form visibility, and `StackLayout` navigation.

The currently reachable main pages are:

- `DashboardPage`
- `JobsPage`
- `CvLibraryPage`
- `CompaniesPage`
- `ContactsPage`
- `JobFormPage`, opened as a form surface through Add Job actions

`SettingsPage`, `ProfilePage`, and `MemoryPage` are registered in
`CMakeLists.txt`, but they are not currently reachable from the main shell.
`Sidebar.qml` displays a Settings item, but only emits navigation for indices
below the five main pages. Its Add CV quick action opens a multi-file picker
without changing the current page.

QML pages live under `qml/pages`.

Shared QML components live under `qml/components`.

Keep concrete QML file registration in `CMakeLists.txt` as the source of truth
for the current file inventory.

## Backend Areas

- `src/app`: process startup, exception boundary, dependency construction, QML
  engine setup, context properties, and main QML loading.
- `src/storage`: application data paths, SQLite connection lifetime, and schema
  migration.
- `src/common`: reusable role-based filtering, sorting, search, stable-ID and
  bulk selection, explicit role projection, presentation, timestamp, and error
  helpers, value-level serial queues, and single-active-worker lifecycle
  infrastructure.
- `src/jobs`: typed job value/draft types, canonical factory and validator, job
  list model, QML controller, repository, shared save worker, and create/update
  services.
- `src/cvs`: CV value type, CV list model, QML controller, repository,
  managed-file store, the application-owned cross-worker CV lock, import
  coordination, and safe file access.
- `src/maintenance`: the shared mutation gate plus asynchronous best-effort job
  and CV removal coordination.
- `src/dashboard`: metric and recent-item read models over jobs and CVs.
- `src/directory`: company/contact value types, durable company repository,
  directory list models and controllers, and linked read models. Contacts remain
  in-memory only.
- `tests`: common, jobs, CVs, dashboard, directory, and storage test suites.

## Known Architecture Gaps

Treat these as current constraints when planning implementation:

- Company editing and all contact persistence/mutation workflows are still
  unavailable.
- `QQmlApplicationEngine` is declared before the backend graph in
  `AppBootstrap`, so reverse member destruction currently destroys exposed
  controllers and models before the engine. The intended lifetime rule in
  [boundaries.md](boundaries.md) is therefore not yet guaranteed during engine
  teardown.
