# Architecture

JobTracker is a Qt 6 / QML / C++ desktop application.

## Entry Points

- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.
- Executable target: `JobTrackerApp`.
- QML module URI: `JobTracker`.

## Current Shape

`src/main.cpp` is intentionally tiny. `main()` only delegates to `App::start(argc, argv)`.

`src/app/App.hpp` and `src/app/App.cpp` own the process-level startup boundary. `App::start()` creates `QGuiApplication`, sets the application name, constructs `AppBootstrap`, runs it, catches startup exceptions, logs failures, and returns a non-zero exit code on startup failure.

`src/app/AppBootstrap.hpp` and `src/app/AppBootstrap.cpp` own the application graph and QML startup wiring. `AppBootstrap` creates and owns the `QQmlApplicationEngine`, connects object creation failure handling, exposes backend controllers to QML through context properties, and loads `qrc:/JobTracker/qml/Main.qml`.

The current bootstrap dependency order is:

1. `StoragePaths`
2. `SQLiteDataBase`
3. `CvRepository`
4. `CvFileAccessService`
5. `CompanyRepository`
6. `JobRepository`
7. `CvImportService`
8. `AddJobService`
9. `JobApplicationsController`
10. `CvLibraryController`
11. `DashboardController`
12. `CompanyDirectoryController`
13. `ContactDirectoryController`

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

Jobs, CVs, and company identities are currently backed by SQLite.

`StoragePaths` resolves the application data location, creates the managed resume folder, and provides the database path.

`SQLiteDataBase` owns the Qt SQL connection lifetime and schema migration.

The current SQLite schema is version 3 and contains:

- `cvs`, which stores managed CV file metadata and uses
  `(sha256, original_file_name)` as a unique identity pair.
- `companies`, which stores a display name and a unique trimmed, case-folded
  normalized name for each durable company identity.
- `jobs`, where every row references one persisted company and one persisted CV
  through the non-null `jobs.company_id` and `jobs.cv_id` foreign keys.
- `job_technologies`, which stores a job's ordered technology rows. Its
  `(job_id, position)` primary key preserves ordering, and its `job_id` foreign
  key references `jobs.id` with `ON DELETE CASCADE`.

`SchemaMigrator` initializes new databases directly at version 3. Version 1
databases first receive the CV identity migration to version 2, then continue
through the company identity migration. The `v2 -> v3` step trims and
case-folds existing job company names, creates one company per normalized
identity, rebuilds jobs with required company foreign keys, and preserves
technology rows. Blank legacy company names abort the transaction with a clear
error. Initialization and all upgrades run transactionally and verify foreign
keys before commit.

`CvRepository` loads and inserts CV metadata, persists favorite changes with an
updated timestamp, and reconstructs each CV's linked application IDs with a
left join from `cvs.id` to `jobs.cv_id`.

`CvFileAccessService` resolves persisted relative paths beneath the managed data
directory, rejects unsafe or unavailable files, and delegates valid local-file
URLs to the platform desktop opener.

`CompanyRepository` loads durable companies and resolves Add Job company names
through the same trimmed, case-folded identity rule.

`JobRepository` loads and inserts jobs by durable company and CV IDs. Its read
query joins `companies.display_name` and `cvs.original_file_name` so the
existing QML-facing company and CV display roles remain unchanged. It then
loads each job's technologies from `job_technologies` in `position` order.

Treat `SchemaMigrator`, the repository queries, and storage tests as the source
of truth for the current persisted schema and relationships.

The production bootstrap hydrates the company directory from
`CompanyRepository` and publishes companies resolved by Add Job immediately.
Company-linked job rows use the same durable IDs after restart. Contacts remain
non-persisted, and there is no company/contact edit or delete workflow.

## Add Job Flow

Add Job is implemented as a QML-to-C++ workflow.

`JobFormPage.qml` gathers form fields and calls `jobApplicationsController.createApplication(formValues, selectedCvUrl)`.

`JobApplicationsController::createApplication()` converts the QML map into `JobApplicationDraft`, publishes `saving`, calls `AddJobService`, appends the created row on success, updates filtering/selection summaries, emits `applicationCreated`, and emits `saveFailed` with field errors on failure.

`AddJobService` owns durable Add Job behavior:

- required field validation;
- durable company resolution by normalized name;
- ISO-date validation;
- HTTP/HTTPS URL validation;
- technology trimming and case-insensitive deduplication;
- CV import coordination;
- job insertion;
- SQLite transaction handling;
- copied-file cleanup on ordinary failures.

`CvImportService` accepts local PDF, DOC, and DOCX files, hashes file content, deduplicates by SHA-256, copies new files into managed storage, and inserts CV metadata.

After successful job creation, `AppBootstrap` forwards CV usage to
`CvLibraryController` and the resolved company to `CompanyDirectoryController`,
so both directories update immediately. On restart, CV and company links are
reconstructed from persisted jobs.

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
- `src/directory`: company/contact value types, durable company repository,
  directory list models and controllers, and linked read models. Contacts remain
  in-memory only.
- `tests`: common, jobs, CVs, dashboard, directory, and storage test suites.

## Backend Implementation Rules

Use the dependency direction `QML -> controllers/models -> services -> repositories -> storage`.

### Dependency Direction

- QML consumes focused controllers and models. Do not expose repositories or broad service objects directly to QML.
- Controllers translate QML values and actions into backend calls, own UI-facing state, and publish the smallest useful property, command, signal, and model-role contract.
- Models present repeated data and maintain Qt model/view contracts. They do not query storage or coordinate multi-step business operations.
- Services own validation, parsing, algorithms, business operations, and transaction coordination.
- Repositories encapsulate persistence queries and map stored rows to domain values without depending on QML contracts.
- Storage classes own database connections, schema migration, and platform-aware storage primitives.
- Bootstrap/application classes construct and own the dependency graph and connect cross-component notifications. Do not place business rules in bootstrap wiring.

### Ownership And Lifetime

- Give each object one explicit owner. Use either Qt parent-child ownership, direct member ownership, or a standard C++ ownership type; do not combine ownership mechanisms for the same object.
- Treat injected pointers and references as non-owning unless the API explicitly transfers ownership.
- Keep every controller and model exposed through a QML context property alive until the QML engine can no longer evaluate or call that object.
- Define destruction order for connected QObjects, asynchronous work, database connections, and the QML engine so no callback can target a destroyed object.

### Errors And QML Boundaries

- Represent expected validation and business rejections as structured results, including field errors when the UI can act on them.
- Use exceptions for unexpected startup, storage, file-system, or infrastructure failures when the caller cannot handle them locally.
- Catch and translate failures at controller or application boundaries before they reach QML. Do not allow C++ exceptions to cross QML invocations, signal delivery, or queued callbacks.
- Preserve the process-level startup exception boundary in `App::start()`.

### Threading

- Keep QML-facing controllers, Qt models, and their mutations on the GUI thread.
- Return worker results through queued delivery and apply model or property changes on the owning thread.
- Define worker ownership, cancellation, shutdown, and late-result handling before moving work off the GUI thread.
- Create, use, and close each Qt SQL connection in one thread. Do not share `QSqlDatabase` connections or active `QSqlQuery` objects across threads.

### Storage And Transactions

- Let services define business-operation transaction boundaries and coordinate repositories. Keep repositories focused on persistence operations and mapping.
- Make schema upgrades forward-only, versioned, transactional, and safe for both new databases and every supported prior version.
- Verify foreign-key state and required invariants before committing schema migrations.
- When one operation changes both SQLite and managed files, define rollback cleanup for ordinary failures and document any remaining crash-recovery limitation.

### Models And QML Contracts

- Keep model role IDs and names stable while QML consumes them.
- Wrap row insertion, removal, movement, and reset operations with the matching Qt begin/end notifications. Emit `dataChanged` with the affected indexes and roles for in-place updates.
- Emit `Q_PROPERTY` notify signals whenever exposed state changes, and avoid emitting change notifications when the value is unchanged unless the contract requires a refresh.
- Preserve selection by stable domain ID across filtering and sorting instead of relying on proxy row numbers.
- Update matching tests whenever properties, signals, model roles, transaction behavior, or QML-facing commands change.

## Known Architecture Gaps

Treat these as current constraints when planning implementation:

- Dashboard recent models currently depend on source-model ordering; appended new rows can miss recent lists after enough rows exist.
- CV import and Add Job persistence run synchronously from the QML invocation and can block the GUI thread for large files.
- Completed CV files can be orphaned if the process crashes after the file copy but before SQLite commit.
- Company editing and all contact persistence/mutation workflows are still
  unavailable.
- Selection is still index-based across filtering and sorting in some controllers.
- Several QML option lists use display strings that C++ also interprets, which is fragile once localization or durable option contracts are introduced.
- Add Job validation and existing selected-job validation are separate code paths and can drift.
