# C++ Research: Whole Backend Refresh
Created: 2026-07-23 12:34 local time (Asia/Baku)

## Scope

This is a read-only research refresh of the whole JobTracker C++ codebase.
It consolidates the lead Codex pass and the requested `cpp_researcher`
subagent pass.

Inspected:

- active instruction layers: `AGENTS.md`, `For-Agent/Docs/architecture.md`,
  `For-Agent/Docs/artifacts.md`, `For-Agent/Docs/coding-style.md`,
  `For-Agent/Docs/testing.md`, and
  `For-Agent/Docs/qml-to-cpp-extraction.md`;
- newest prior whole-C++ artifact:
  `For-Agent/Research/cpp-research-2026-07-09-1535-whole-backend.md`;
- all current C++ files under `src/`;
- all current C++ tests under `tests/`;
- `CMakeLists.txt` source and test registration;
- QML-facing C++ contracts through `Q_PROPERTY`, `Q_INVOKABLE`, signals, and
  bootstrap context properties.

No production code was edited. No build, CTest run, or GUI launch was
performed because this was a research-only task.

## Current C++ Inventory

Current live inventory:

- 29 production `.cpp` files;
- 35 production `.hpp` files;
- 6 test `.cpp` files;
- 1 test-support `.hpp` file.

The backend areas are:

- `src/main.cpp`: tiny process entry point.
- `src/app`: process startup, exception boundary, dependency graph, QML engine,
  context properties, and main QML loading.
- `src/storage`: app data paths, SQLite connection lifetime, schema migration.
- `src/utils`: shared SQL error helpers.
- `src/common`: role-based proxy filtering/sorting/search and validation.
- `src/jobs`: job value/draft types, list model, QML controller, repository,
  and Add Job service.
- `src/cvs`: CV value type, list model, linked-application model, QML
  controller, repository, and managed-file import service.
- `src/dashboard`: dashboard metric and recent-item read models.
- `src/directory`: company/contact value types, empty list models, QML-facing
  directory controllers, and linked read models.
- `tests`: common, jobs, CVs, dashboard, directory, and storage/Add Job suites.

`git status --short` was empty during this research pass.

## Startup And Ownership

`src/main.cpp` delegates to `App::start(argc, argv)`.

`App::start()` owns the process startup boundary: it creates `QGuiApplication`,
sets the application name, constructs `AppBootstrap`, calls `run()`, catches
startup exceptions, logs failures, and returns a non-zero exit code on failure.

`AppBootstrap` owns the runtime graph in this order:

1. `StoragePaths`
2. `SQLiteDataBase`
3. `CvRepository`
4. `JobRepository`
5. `CvImportService`
6. `AddJobService`
7. `JobApplicationsController`
8. `CvLibraryController`
9. `DashboardController`
10. shared empty `ContactListModel`
11. `CompanyDirectoryController`
12. `ContactDirectoryController`

`AppBootstrap` exposes these context properties to QML:

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

It also connects:

`JobApplicationsController::cvUsed` -> `CvLibraryController::recordCvUse`

That connection keeps the in-memory CV library linked-application state in sync
after a successful Add Job operation.

## Persistence And Storage Flow

`StoragePaths` resolves app-local storage, creates the managed resumes
directory, and provides the SQLite database path.

`SQLiteDataBase` owns a named Qt SQL connection, enables foreign keys, sets a
busy timeout, runs schema migration, and removes the connection on teardown.

Schema version 1 contains:

- `cvs`
- `jobs`
- `job_technologies`

Persisted relationships:

- job -> CV through `jobs.cv_id`;
- job -> ordered technologies through `job_technologies.job_id`.

`SchemaMigrator` uses `PRAGMA user_version`, creates schema v1 for a new
database, no-ops on v1, and throws if the database version is newer than this
application supports.

`CvRepository::findAll()` loads CVs with linked job IDs through one joined
query and reconstructs one `CvDocument` per CV.

`JobRepository::findAll()` loads jobs joined to CV filenames and then loads all
job technologies in a second query, mapping technology rows back by job ID.
This is an improvement over the older artifact's N+1 concern.

## Add Job Flow

QML calls:

`JobApplicationsController::createApplication(formValues, selectedCvUrl)`

The controller:

- ignores duplicate saves while `saving_` is true;
- converts the QML `QVariantMap` into `JobApplicationDraft`;
- emits `savingChanged`;
- calls `AddJobService::create`;
- appends the created application on success;
- re-sorts the proxy model;
- refreshes selection;
- emits `applicationsModelChanged`, `resultSummaryChanged`, `cvUsed`, and
  `applicationCreated`;
- emits `saveFailed` with field errors and a message on failure.

`AddJobService` owns durable Add Job behavior:

- normalizes fields;
- defaults empty status to `Applied`;
- defaults empty applied date to today's ISO date;
- validates required job title, company, date, selected CV, and HTTP/HTTPS URL
  scheme;
- starts a SQLite transaction;
- imports or reuses the CV;
- builds and inserts the job record;
- commits the transaction;
- rolls back and removes a newly copied file on ordinary failures.

`CvImportService` owns managed CV import:

- rejects non-local URLs;
- accepts PDF, DOC, and DOCX;
- checks readability;
- hashes file contents with SHA-256;
- deduplicates by content hash;
- creates a sanitized, timestamped, UUID-backed stored filename;
- copies through a `.part` file and renames to the final stored file;
- inserts CV metadata through `CvRepository`;
- removes the final copied file if metadata insertion fails.

## QML-Facing Contracts

### Jobs

`JobApplicationsController` exposes:

- `applicationsModel`
- `applicationCount`
- `selectedApplicationIndex`
- `selectedApplicationId`
- `selectedApplication`
- `searchText`
- `statusFilter`
- `resultSummary`
- `saving`

Invokables:

- `selectApplication`
- `setSearchText`
- `setStatusFilter`
- `clearFilters`
- `validateSelectedApplication`
- `createApplication`

Signals:

- `applicationsModelChanged`
- `selectedApplicationChanged`
- `filtersChanged`
- `resultSummaryChanged`
- `savingChanged`
- `applicationCreated`
- `saveFailed`
- `cvUsed`

### CVs

`CvLibraryController` exposes:

- `cvModel`
- `linkedApplicationsModel`
- `categorySummary`
- `cvCount`
- `selectedCvIndex`
- `selectedCvId`
- `selectedCv`
- `searchText`
- `categoryFilter`
- `languageFilter`
- `sortMode`
- `resultSummary`

Invokables:

- `selectCv`
- `setSearchText`
- `setCategoryFilter`
- `setLanguageFilter`
- `setSortMode`
- `clearFilters`
- `toggleFavorite`
- `openCv`

Signals:

- `cvModelChanged`
- `linkedApplicationsModelChanged`
- `selectedCvChanged`
- `filtersChanged`
- `resultSummaryChanged`
- `openCvRequested`
- `operationFailed`

### Dashboard

`DashboardController` exposes four constant models:

- `statsModel`
- `funnelModel`
- `recentApplicationsModel`
- `recentCvsModel`

Metrics refresh on job row insertion and model reset. Recent applications and
recent CV models reset on source insert/reset; recent CVs also reset on source
data changes.

### Directory

`CompanyDirectoryController` and `ContactDirectoryController` expose list,
selection, filtering, sorting, linked jobs/contacts, and interaction-history
contracts. Production company/contact data is currently empty because there are
no repositories, storage tables, mutation services, or seed-data injection for
those domains.

## CMake And Test Registration

`CMakeLists.txt` registers the application target and six test executables:

- `JobTrackerCommonTests`
- `JobTrackerJobsTests`
- `JobTrackerCvTests`
- `JobTrackerDashboardTests`
- `JobTrackerDirectoryTests`
- `JobTrackerStorageTests`

Static source inventory did not find missing current production `.cpp` files or
test `.cpp` files from `CMakeLists.txt`.

One build-system concern remains: `find_package(Qt6 6.5 REQUIRED COMPONENTS
Quick Sql Test)` makes Qt Test a configure-time dependency even when
`JOBTRACKER_BUILD_TESTS` is `OFF`.

## Existing Test Coverage

Current tests cover:

- role-filter proxy search, exact filters, required non-empty roles, and sort;
- validation helper behavior;
- job controller role exposure, selection, filtering, clearing, validation, and
  selected data mapping;
- CV controller roles, selection, filters, favorite toggling, category summary,
  and linked-application model behavior;
- dashboard metric and recent model data from backend source models;
- directory role exposure and empty-state controller behavior;
- SQLite schema creation/reopen;
- foreign-key activation;
- CV copying and SHA-256 deduplication;
- job persistence across reopen;
- rollback and copied-file cleanup after insert failure;
- Add Job invalid input rejection;
- controller-level Add Job success publication.

## High-Priority Static Risks

### 1. `utils::executeQuery` is incorrectly marked `[[noreturn]]`

`utils::executeQuery` is declared and defined as `[[noreturn]]`, but it returns
normally when `query.exec(statement)` succeeds. That is a real C++ contract bug
and can make compiler assumptions unsafe.

`throwQueryError` is correctly `[[noreturn]]`; `executeQuery` is not.

### 2. Storage tests use stale database class spelling

`tests/storage/StorageAndAddJobTest.cpp` includes
`storage/SqliteDatabase.hpp`, but instantiates `SqliteDatabase`. The production
class in `src/storage/SqliteDatabase.hpp` is `SQLiteDataBase`.

Because `JobTrackerStorageTests` is registered, the storage test target likely
fails to compile until the spelling is aligned.

## Architecture And Behavior Risks

### Stored CV opening is incomplete

`CvLibraryController::openCv()` emits `cv->fileName_`, which is the display or
original filename, not a managed absolute file path. No in-repo consumer was
observed for `openCvRequested`.

The data model already stores `storedFileName_` and `relativePath_`, but the
controller does not yet expose a platform-aware open-file workflow.

### CV favorite changes are in-memory only

`CvLibraryController::toggleFavorite()` delegates to `CvListModel`, which
mutates memory. `CvRepository` has no update path for `is_favorite`, so
favorites are not durable across restart.

### Dashboard recent models depend on source row order

Repositories load newest-first, but newly created jobs and newly inserted CVs
are appended to the end of source models. Dashboard recent models expose the
first five source rows. Once there are enough existing rows, a newly appended
item can be omitted from recent lists.

### Company identity and linkage are incomplete

Schema v1 stores `company_name`, while `JobApplication` still has `companyId_`
and `LinkedCompanyJobsModel` filters by `CompanyIdRole`. Persisted and newly
created jobs therefore cannot form durable company-ID relationships.

### Company/contact backend is structural only

Company and contact models expose roles and controllers expose QML contracts,
but production models contain empty vectors and there is no storage, mutation
workflow, or repository layer.

### Add Job validation can drift from shared validation

`ValidationService` is used for selected-job validation. `AddJobService`
implements Add Job validation separately. The URL rules already differ:
`ValidationService::validateHttpUrl` checks URL validity, scheme, and host,
while Add Job currently checks validity and scheme.

### CV import runs synchronously and reads entire files into memory

`CvImportService` hashes with `source.readAll()` and runs hashing, copying, and
SQL insertion synchronously under a QML-triggered save flow. This is simple, but
large files can block the GUI thread.

### Selection remains index-based across filters and sorting

Job and CV controllers clamp selected proxy indices after filter/sort changes.
That preserves a valid row, but it does not preserve the same selected record by
stable ID.

### Filesystem and database commits are not crash-atomic

Ordinary failures remove copied files, and `.part` files are cleaned on startup.
A process crash after final file rename but before durable commit can still
leave an orphan completed file.

### Display strings remain API values

Filtering and sorting contracts compare user-facing strings such as `All`,
`Last Modified`, `Linked Jobs`, and status labels. This is fragile for future
localization or durable enum-like contracts.

## Suggested Implementation Direction

1. Fix compile/static correctness first:
   - remove `[[noreturn]]` from `utils::executeQuery`;
   - align `SqliteDatabase`/`SQLiteDataBase` spelling in storage tests and
     production naming.
2. Run the documented configure, build, and CTest flow after those fixes.
3. Stabilize user-visible persistence gaps:
   - stored CV opening through a platform-aware path/opener contract;
   - favorite persistence;
   - dashboard recent ordering after appended rows.
4. Define durable company identity before adding company/contact repositories
   or reviving linked company views.
5. Consolidate Add Job validation around a canonical service-level validation
   path.
6. Preserve selection by stable IDs rather than proxy row numbers.
7. Move CV hashing/copying off the GUI thread if large imported files are
   expected.
8. Add reconciliation for orphaned managed CV files.

## Verification Status

Verified statically against the live source, tests, `CMakeLists.txt`, current
instruction layers, the newest prior C++ research artifact, and the requested
`cpp_researcher` subagent output.

No build, CTest run, or GUI launch was performed.
