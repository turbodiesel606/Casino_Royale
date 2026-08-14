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
2. `SqliteDatabase`
3. `CvRepository`
4. `CvManagedFileStore`
5. `CvFileAccessService`
6. `CompanyRepository`
7. `JobRepository`
8. `AddJobWorker`
9. `JobApplicationsController`
10. `CvLibraryController`
11. `DashboardController`
12. `CompanyDirectoryController`
13. `ContactDirectoryController`

`AddJobWorker` is a GUI-thread facade. On its first submitted request it starts
one reusable dedicated thread. The GUI controller maps form values and runs
storage-independent canonical preflight synchronously. For a validated request,
the worker defensively revalidates the normalized draft and then lazily
constructs a private `StoragePaths`, `SqliteDatabase`, repository, managed-file,
import-service, and Add Job service graph inside that thread. Production no
longer owns GUI-thread `CvImportService` or `AddJobService` instances.

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

`SqliteDatabase` owns the Qt SQL connection lifetime and schema migration.
Storage-specific SQL helpers execute standalone statements and translate Qt SQL
failures into exceptions that identify the failed operation. Repositories use
the same contextual error boundary for prepared queries. `SqlTransaction`
starts one transaction, requires an explicit successful commit, and
automatically rolls back while still active.

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

`SchemaMigrator` reads `PRAGMA user_version`, rejects unsupported newer
databases, selects the direct version-3 initialization or applies each forward
migration sequentially, verifies foreign keys, and commits through
`SqlTransaction`. The explicit initialization, `v1 -> v2`, and `v2 -> v3` SQL
live in separate version-specific implementation units. Version 1 databases
first receive the CV identity migration to version 2, then continue through the
company identity migration. The `v2 -> v3` step trims and case-folds existing
job company names, creates one company per normalized identity, rebuilds jobs
with required company foreign keys, and preserves technology rows. Blank
legacy company names abort the transaction with a clear error. Initialization
and all upgrades run transactionally and verify foreign keys before commit.

`CvRepository` loads and inserts CV metadata, persists favorite changes with an
updated timestamp, and reconstructs each CV's linked application IDs with a
left join from `cvs.id` to `jobs.cv_id`.

`CvFileAccessService` resolves persisted relative paths beneath the managed data
directory, rejects unsafe or unavailable files, and delegates valid local-file
URLs to the platform desktop opener.

`CvManagedFileStore` owns the managed CV filesystem boundary. It validates PDF,
DOC, and DOCX inputs, streams SHA-256 while copying into uniquely named `.part`
files, atomically finalizes staged files, and removes abandoned stages through
RAII cleanup. Startup reconciliation removes stale `.part` files and moves any
completed top-level managed file without a matching `cvs.stored_file_name` row
into `Resumes/Quarantine`; completed orphaned user files are never silently
deleted.

`CompanyRepository` loads durable companies and resolves Add Job company names
through the same trimmed, case-folded identity rule.

`JobRepository` loads and inserts jobs by durable company and CV IDs. Its read
query joins `companies.display_name` and `cvs.original_file_name` so the
existing QML-facing company and CV display roles remain unchanged. It then
loads each job's technologies from `job_technologies` in `position` order.

Repositories translate the schema's existing ISO text representation into
typed domain values. Job applied dates use `QDate`; job, CV, and company
creation/update timestamps use `QDateTime`; job and company URLs use `QUrl`;
and job status/work format use closed enum values. Writes serialize those
values back to the existing schema version 3 text columns, so this type
boundary does not require a schema migration.

Treat `SchemaMigrator`, the repository queries, and storage tests as the source
of truth for the current persisted schema and relationships.

The production bootstrap hydrates the company directory from
`CompanyRepository` and publishes companies resolved by Add Job immediately.
Company-linked job rows use the same durable IDs after restart. Contacts remain
non-persisted, and there is no company/contact edit or delete workflow.

## Add Job Flow

Add Job is implemented as a QML-to-C++ workflow.

`JobFormPage.qml` gathers form fields and calls `jobApplicationsController.createApplication(formValues, selectedCvUrl)`.

`JobApplicationsController::createApplication()` maps the raw `QVariantMap` to
`JobApplicationDraft` and calls the storage-independent
`AddJobService::preflight()` synchronously. Preflight normalizes the draft and
runs the canonical validator without reading the CV, touching the filesystem,
opening SQLite, or waiting for the worker. Invalid input emits
`saveFailed(fieldErrors, message)` and returns before operation-ID allocation,
queue mutation, pending-state publication, or worker startup.

For valid input, the controller allocates a monotonically increasing operation
ID, inserts the normalized draft and CV URL into its GUI-owned FIFO, publishes
pending state, emits `applicationQueued(operationId)`, and schedules the front
request. The queue keeps one active request plus normalized waiters, so it needs
no mutex. `pendingSaveCount` is the active-plus-waiting total, and `saving` is
true exactly while that total is non-zero.

When a request becomes active, the controller creates a shared
`CancellationState` and submits one `AddJobRequest` to `AddJobWorker`. The
worker thread is started lazily and reused until shutdown. Its private SQLite
connection opens the same database file under an independent unique Qt
connection name only after the worker's defensive validation succeeds; the
existing GUI connection remains responsible for startup hydration and
non-Add-Job operations.

The worker executes the complete durable pipeline in order:

1. Check the cancellation state and cooperative cancellation.
2. Defensively run the canonical validator on the normalized draft.
3. Lazily initialize the worker-owned persistence context.
4. Call `AddJobService::prepare()`, which revalidates before streaming the CV
   hash and staged copy.
5. Check cancellation immediately before the transaction.
6. Resolve the company and CV, insert the job, and commit through
   `AddJobService::complete()` on the worker thread.
7. Queue one value-only `AddJobSaveOutcome` to the GUI facade.

Every failure after queueing, including a defensive validation anomaly,
cancellation, worker initialization, CV processing, or SQLite failure, is
reported through `applicationSaveCompleted`. The controller retains the active
slot until this final outcome, then releases it and advances the FIFO. The
controller ignores stale outcomes unless both operation ID and cancellation
identity match the active request.

Only the GUI controller mutates Qt models or publishes cross-screen signals.
On a successful final result it appends the job, refreshes sorting, and emits
`applicationCreated`, `companyResolved`, and `cvUsed`. The worker transports no
model, repository, SQL handle/query, or staged-file object across threads.

`CancellationState` protects only its boolean with a mutex. Cancellation checks
never hold that mutex during file I/O, SQL, signal emission, or queued delivery.
`cancelCreateApplication()` cancels only the active request and then continues
the FIFO. `cancelAllCreateApplications()` removes normalized waiters, cancels
the active request, suppresses exit-time notifications, and emits
`saveQueueDrained` only after active cleanup. Once a transaction begins it is
allowed to commit or roll back; it is never forcibly terminated.

`JobApplicationFactory` owns draft trimming, status/date defaults,
case-insensitive technology deduplication, typed conversion, and final job
domain-object construction. `JobApplicationValidator` is the one canonical job
validation path for both Add Job and selected-application validation. It
returns field-addressable errors for required values, optional HTTP/HTTPS URLs
with a required host, ISO dates, and allowed status/work-format choices.

The worker-owned `AddJobService` owns durable Add Job orchestration:

- delegation to the factory and validator;
- worker-safe managed-file preparation without database access;
- durable company resolution by normalized name;
- database-thread duplicate resolution by `(sha256, original_file_name)`;
- staged-file finalization and CV insertion;
- job insertion;
- SQLite transaction handling;
- completed-file cleanup on ordinary failures.

`CvImportService` connects the worker's managed-file store to its private
`CvRepository`. It prepares the file, then resolves the existing composite CV
identity or finalizes and inserts a new CV on the same worker thread. The
transaction orders new-file work as finalization, CV insertion, job insertion,
and commit. A process crash after finalization but before commit leaves a
completed orphan that startup recovery quarantines.

After successful job creation, `AppBootstrap` forwards CV usage to
`CvLibraryController` and the resolved company to `CompanyDirectoryController`,
so both directories update immediately. On restart, CV and company links are
reconstructed from persisted jobs.

`JobFormPage.qml` keeps unsaved form values and the selected CV while the user
navigates between pages. Synchronous `saveFailed` preserves the entered values
and selected CV while applying inline errors and the global validation message.
`applicationQueued` resets the form immediately after the normalized values have
been captured; the form stays enabled and ready for another submission. Final
completion outcomes belong to previously queued requests and never change the
current form. Discard resets current fields and errors, clears the CV selection,
restores the `Applied` status, and neither cancels queued work nor deletes a
selected source file.

`Main.qml` owns presentation-only save notifications and close confirmation.
Completion notifications are non-modal, display one at a time for 15 seconds,
and queue later outcomes. A close request while `pendingSaveCount` is non-zero
is rejected and offers only Wait or Interrupt and Exit. Wait resumes hidden or
queued notifications without affecting saves. Interrupt and Exit discards
notifications, calls `cancelAllCreateApplications()`, and closes only after
`saveQueueDrained`. If the queue drains naturally while the confirmation is
open, the confirmation closes and the application remains open.

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

`StableIdSelectionTracker` is the shared selection contract for the jobs, CV,
company, and contact controllers. Each controller composes one tracker with
its proxy model and ID role. The tracker owns the selected domain ID and
derived proxy row, reconciles proxy insert, remove, move, reset, layout, and
data changes, and applies the common fallback policy: choose the first visible
row when the selected ID is hidden, or clear selection when no row is visible.
Controller-owned filter and sort updates are reconciled as one completed model
update so transient proxy-removal batches do not publish intermediate fallback
selections. Linked selection-dependent models refresh only when the effective
selected ID changes.

Controller model-pointer properties are constant because their model objects
do not change after construction. Model content changes are published through
the models' row, reset, layout, and data signals. Scalar controller properties
use semantic notify signals: selected ID, selected proxy index, selected data,
individual filters, visible counts, result summaries, and the CV category
summary notify only for their own contract changes.

`RelationFilterProxyModel` is the shared linked-view contract for CV
applications, company jobs, and company contacts. Each controller configures
the proxy with the source relation role and selected domain ID. The proxy
preserves source role names and reacts directly to source insertion, removal,
movement, reset, and relation-role data changes without manual source-row
caches. An empty selected ID produces an empty linked view.

`LimitedSortedProxyModel` owns dashboard recency projections. Recent
applications expose the five newest jobs by typed creation timestamp; recent
CVs expose the five newest CVs by typed update timestamp. Both projections use
the domain ID as a deterministic tie-break and rebuild after relevant source
structure or data changes. The dashboard's existing QML role names remain
available, including the `appliedDateLabel` alias. CV category summaries notify
when rows are inserted, removed, or reset and when category-role data changes.

Durable domain objects do not store repository-built display strings.
Job, CV, and company list models derive initials, accent colors, status/date
labels, and file-size labels from typed values while preserving the existing
QML role names and displayed values. The job, CV, and company models also
publish typed `createdAt` and `updatedAt` roles for backend sorting; jobs add
typed status, work-format, and applied-date roles. `RoleFilterProxyModel`
compares `QDate` and `QDateTime` values directly. QML continues to receive the
pre-existing string roles and selected-item map fields.

Current backend areas are:

- `src/app`: process startup, exception boundary, dependency construction, QML engine setup, context properties, and main QML loading.
- `src/storage`: application data paths, SQLite connection lifetime, and schema migration.
- `src/common`: reusable role-based filtering, sorting, search, and stable-ID selection helpers.
- `src/jobs`: typed job value/draft types, canonical factory and validator, job list model, QML controller, repository, and Add Job service.
- `src/cvs`: CV value type, CV list model, QML controller, repository,
  managed-file store, import coordination, and safe file access.
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
- Add Job form mapping and canonical preflight run synchronously in the GUI
  controller and remain pure and storage-independent. Defensive validation,
  managed CV work, duplicate lookup, repositories, and transactions run on the
  dedicated Add Job worker and its private SQLite connection. Only value
  outcomes return to the GUI thread.
- Keep Add Job model mutation and QML/cross-screen signal publication in the
  GUI controller. Destroy the worker persistence graph on its own thread before
  quitting and waiting for that thread; never terminate it.

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

- Company editing and all contact persistence/mutation workflows are still
  unavailable.
