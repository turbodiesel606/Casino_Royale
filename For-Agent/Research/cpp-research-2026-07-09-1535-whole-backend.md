# C++ Research: Current Whole Backend Surface
Created: 2026-07-09 15:35 local time (Asia/Baku)

## Scope

This is a read-only research snapshot of the current JobTracker working tree.
It consolidates the lead Codex pass and the requested `cpp_researcher`
subagent pass.

Inspected:

- all 62 C++ headers and sources under `src/`;
- all 7 C++ test and test-support files under `tests/`;
- `CMakeLists.txt` and `CMakePresets.json`;
- startup, ownership, storage, models, controllers, repositories, services,
  and cross-domain data flow;
- relevant QML call sites for C++ contracts;
- current Git status and the changes since
  `For-Agent/Research/cpp-research-2026-07-07-backend-surface.md`.

The working tree contains substantial uncommitted and untracked backend work.
This report describes that live working tree, not only Git `HEAD`.

No production code was changed by this research. No build, CTest run, or GUI
launch was performed.

## Executive Summary

The backend has moved materially beyond the July 7 artifact:

- SQLite persistence, schema migration, CV file import, repositories, and a
  transactional Add Job workflow now exist.
- Jobs and CVs load from SQLite instead of production seed data.
- Add Job is connected from QML through `JobApplicationsController`.
- Dashboard and linked-CV models now react to inserted source rows.
- Company and contact production models are empty and have no repository or
  storage implementation.
- Important integration gaps remain: opening a CV still has no usable stored
  path contract, favorites are not persisted, new records can miss the
  dashboard's recent rows, and persisted jobs cannot link to companies by ID.

## Current Inventory

The live C++ surface contains 28 production translation units under `src/`
and 6 test translation units under `tests/`.

- `src/app`: process startup, exception boundary, dependency construction,
  QML engine, and context properties.
- `src/storage`: application data paths, SQLite connection lifetime, and
  schema migration.
- `src/common`: reusable role-based filtering/sorting/search and validation.
- `src/jobs`: job value/draft types, model, controller, repository, and Add
  Job service.
- `src/cvs`: CV value type, model, controller, linked-job adapter, repository,
  and managed-file import.
- `src/dashboard`: metrics and recent-item read models.
- `src/directory`: company/contact value types, empty list models,
  QML-facing controllers, and linked read models.
- `tests`: common, jobs, CVs, dashboard, directory, and storage suites.

All current C++ translation units under `src/` and `tests/` are referenced by
`CMakeLists.txt`. The test-support header is included by tests and does not
need separate target registration.

## Startup And Ownership

`src/main.cpp` is now a minimal delegate: `main()` only calls `App::start()`
(`src/main.cpp:1-6`).

`App::start()` owns `QGuiApplication`, sets the application name, constructs
`AppBootstrap`, catches startup exceptions, and returns a failure code when
startup fails (`src/app/App.cpp:11-25`).

`AppBootstrap` constructs the application graph in dependency order:

1. `StoragePaths`
2. `SqliteDatabase`
3. `CvRepository`
4. `JobRepository`
5. `CvImportService`
6. `AddJobService`
7. `JobApplicationsController`, initialized from SQLite
8. `CvLibraryController`, initialized from SQLite
9. `DashboardController`
10. one empty `ContactListModel` and the two directory controllers

Evidence: `src/app/AppBootstrap.h:31-44` and
`src/app/AppBootstrap.cpp:10-21`.

The five QML context properties are unchanged:

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

Evidence: `src/app/AppBootstrap.cpp:49-56`.

Successful job creation propagates CV usage directly through:

`JobApplicationsController::cvUsed` -> `CvLibraryController::recordCvUse`

Evidence: `src/app/AppBootstrap.cpp:23-27`.

## Persistence And Storage

### Paths And Files

`StoragePaths` resolves an application-local location through
`QStandardPaths`, creates `Data/Resumes`, places SQLite at
`Data/database.sqlite`, and removes leftover `*.part` files
(`src/storage/StoragePaths.cpp:10-24`, `42-58`).

The implementation uses Qt path APIs and is suitable for Windows and Linux.

### SQLite Lifecycle

`SqliteDatabase` creates a unique named QSQLITE connection, enables foreign
keys, sets a three-second busy timeout, runs migration, then closes and
removes the named Qt SQL connection during destruction
(`src/storage/SqliteDatabase.cpp:23-41`).

### Schema

Schema version 1 contains:

- `cvs`;
- `jobs`;
- `job_technologies`.

Jobs require a valid CV foreign key. Technologies are ordered per job and
cascade on job deletion (`src/storage/SchemaMigrator.cpp:28-71`).

Migration is transaction-wrapped and rejects schemas newer than the
application supports (`src/storage/SchemaMigrator.cpp:76-98`).

### Repositories

`CvRepository` loads and inserts CV metadata, looks up content by SHA-256, and
reconstructs linked job IDs (`src/cvs/CvRepository.cpp:60-120`).

`JobRepository` loads jobs joined to CV metadata and executes a separate
technology query per job (`src/jobs/JobRepository.cpp:29-72`). Insert writes
the job followed by its ordered technology rows
(`src/jobs/JobRepository.cpp:75-113`).

Both repositories currently use N+1 reads for related rows. This is simple
and acceptable for small local datasets, but it is the main query-scaling
concern in the persistence layer.

## Add Job Workflow

`AddJobService` owns:

- required-field, ISO-date, and HTTP/HTTPS URL validation;
- technology trimming and case-insensitive deduplication;
- CV import and job insertion;
- the SQLite transaction;
- copied-file cleanup after ordinary failures.

Evidence: `src/jobs/AddJobService.cpp:43-126`.

`CvImportService` accepts local PDF, DOC, and DOCX files, hashes the entire
content with SHA-256, reuses an existing CV with identical content, copies new
files through a `.part` path, creates a unique stored filename, and inserts CV
metadata (`src/cvs/CvImportService.cpp:22-93`).

`JobApplicationsController::createApplication()` converts a QML map into
`JobApplicationDraft`, publishes saving state, calls the service, appends the
new row, and emits success/failure and CV-linkage signals
(`src/jobs/JobApplicationsController.cpp:184-232`).

`JobFormPage.qml` calls this contract and handles `applicationCreated` and
`saveFailed` (`qml/pages/JobFormPage.qml:29-45`, `107-120`).

## Domain Ownership And Behavior

### Common

`RoleFilterProxyModel` remains the shared implementation for tokenized search,
multiple exact filters, one required-non-empty role, and role-aware numeric,
date, and string sorting (`src/common/RoleFilterProxyModel.cpp:20-173`).

`ValidationService` remains a small required-field and HTTP/HTTPS URL helper
(`src/common/ValidationService.cpp:5-40`). Add Job currently performs its own
parallel validation rather than reusing this service.

### Jobs

Production job data is empty unless loaded from SQLite. Embedded production
seed construction has been removed.

`JobApplicationListModel` owns current in-memory rows and supports reset and
append operations (`src/jobs/JobApplicationListModel.cpp:77-153`).

`JobApplicationsController` owns proxy filtering, selection, validation, and
the new QML-facing save contract (`src/jobs/JobApplicationsController.h:18-63`).

### CVs

Production CV data is loaded from SQLite.

`CvListModel` supports reset, append, linked-application updates, and
in-memory favorite toggling (`src/cvs/CvListModel.cpp:96-135`).

`LinkedApplicationListModel` refreshes on job insertion, reset, and data
changes (`src/cvs/LinkedApplicationListModel.cpp:3-10`, `88-92`).

### Dashboard

Dashboard metric models now refresh after job insertion or source reset
(`src/dashboard/DashboardController.cpp:103-113`, `135-139`).

The recent-job and recent-CV adapters also reset after relevant source-model
changes (`src/dashboard/DashboardRecentApplicationsModel.cpp:11-23`,
`src/dashboard/DashboardRecentCvsModel.cpp:11-27`).

### Companies And Contacts

`CompanyListModel` and `ContactListModel` now construct empty storage and
expose only roles/accessors (`src/directory/CompanyListModel.cpp:44-82`,
`src/directory/ContactListModel.cpp:43-83`).

There is no company/contact repository, persistence, mutation path, or test
data injected into production bootstrap. The Companies and Contacts screens
therefore receive zero production rows. Their controllers and derived models
remain structurally present but operate over empty sources.

## Cross-Domain Data Flow

The canonical persisted relationships are currently:

- Job -> CV through `jobs.cv_id`;
- Job -> technologies through `job_technologies.job_id`.

After a job is created, the in-memory CV model is updated through the
`cvUsed`/`recordCvUse` connection. On restart, `CvRepository::findAll()`
reconstructs linked application IDs from the jobs table.

Company linkage is incomplete at the persistence boundary:

- `JobApplication` still has `companyId_`
  (`src/jobs/JobApplication.h:8-10`);
- schema version 1 stores `company_name`, but no `company_id`
  (`src/storage/SchemaMigrator.cpp:45-63`);
- `JobRepository` never reconstructs `companyId_`
  (`src/jobs/JobRepository.cpp:40-58`);
- `LinkedCompanyJobsModel` joins only through `CompanyIdRole`
  (`src/directory/LinkedCompanyJobsModel.cpp:71-79`).

Persisted and newly created jobs therefore cannot appear as linked jobs for a
company even after company rows are reintroduced.

## Main Risks And Maintainability Gaps

1. **Open CV has no usable stored-path contract.**
   `CvLibraryController::openCv()` emits `cv->fileName_`, which is only the
   display/original filename (`src/cvs/CvLibraryController.cpp:263-273`).
   No C++ or QML consumer handles `openCvRequested`. Managed files now have a
   `relativePath_`, but it is not exposed through a platform-aware opener.

2. **Favorite changes are not durable.**
   `CvListModel::toggleFavorite()` mutates only memory
   (`src/cvs/CvListModel.cpp:125-135`). `CvRepository` has no update method,
   so favorite state reverts after restart.

3. **New rows can miss the dashboard's recent lists.**
   Repositories load newest-first, but new jobs/CVs are appended to the end of
   source models (`src/jobs/JobApplicationListModel.cpp:148-153`,
   `src/cvs/CvListModel.cpp:103-109`). Dashboard recent models expose the
   first five source rows. After five rows exist, a newly appended item can be
   omitted from “recent.”

4. **Filesystem and database commits are not crash-atomic.**
   Normal exceptions remove newly copied CV files and this is tested.
   However, the final CV file exists before SQLite commit. A process crash in
   that window can leave an orphan completed file. Startup removes `.part`
   files, not completed files lacking database rows.

5. **Add Job performs blocking work on the GUI thread.**
   Hashing reads the whole file into memory
   (`src/cvs/CvImportService.cpp:39-44`), and hashing, copy, and SQL work are
   synchronous inside the QML invocation. `saving` prevents duplicate clicks
   but does not preserve UI responsiveness during a large import.

6. **The directory backend is currently a shell.**
   Companies and contacts are empty, non-persistent, and non-mutable.
   `LinkedCompanyJobsModel` and `LinkedCompanyContactsModel` also do not
   subscribe to source insertion/reset changes, unlike the CV-linked model.

7. **Selection is index-based across dynamic sorting.**
   After insertion and proxy re-sort, job selection is clamped by numeric
   index rather than preserved by stable record ID
   (`src/jobs/JobApplicationsController.cpp:225-227`, `250-257`).
   The selected record can silently change.

8. **Qt Test is still a production configure dependency.**
   `find_package` requests `Test` even when `JOBTRACKER_BUILD_TESTS` is off
   (`CMakeLists.txt:14`).

9. **Display strings remain API values.**
   Status/filter/sort behavior compares strings such as `All`, `Linked Jobs`,
   and `Last Modified` in both QML and C++. This remains fragile for
   localization and contract evolution.

10. **Validation ownership is duplicated.**
    `ValidationService` validates existing selected jobs, while
    `AddJobService` independently implements Add Job validation. The rules
    can drift unless one domain-level validation path becomes canonical.

## Test Inventory

Six CTest executables are registered:

- `JobTrackerCommonTests`
- `JobTrackerJobsTests`
- `JobTrackerCvTests`
- `JobTrackerDashboardTests`
- `JobTrackerDirectoryTests`
- `JobTrackerStorageTests`

Evidence: `CMakeLists.txt:186-293`.

The new storage suite covers:

- initial migration and reopen;
- foreign-key activation;
- CV copying and SHA-256 deduplication;
- job persistence across database reopen;
- rollback and copied-file cleanup after an insert failure;
- invalid-input rejection;
- controller success publication.

Evidence: `tests/storage/StorageAndAddJobTest.cpp:49-229`.

Notable gaps:

- no build or runtime proof for the current uncommitted persistence work;
- no startup test around `AppBootstrap`;
- no test that newly inserted jobs/CVs appear in dashboard recent rows;
- no direct test for `CvLibraryController::recordCvUse`;
- no favorite-persistence or stored-file-open test;
- no company-ID persistence/linkage test;
- directory tests now verify only role names and empty state
  (`tests/directory/DirectoryControllerTest.cpp:35-71`);
- no unsupported-extension, non-local URL, unreadable-file, or large-file
  import tests;
- no orphan-file reconciliation or newer-schema rejection test;
- no source insertion/reset refresh tests for company-linked models.

## CMake And Build Registration

The persistence group is registered at `CMakeLists.txt:39-54` and included in
the application and persistence-dependent tests. Qt SQL is linked to the
application and relevant tests.

All current production and test translation units are registered. The current
build system exposes one avoidable dependency: `Qt6::Test` is discovered
unconditionally even for a production-only configuration.

## Changes Since The July 7 Artifact

| July 7 claim | Current working-tree status |
|---|---|
| `main.cpp` creates the application and handles exceptions | Stale: moved to `App::start()` |
| Jobs, CVs, companies, and contacts are seeded production models | Stale: jobs/CVs load from SQLite; companies/contacts are empty |
| No durable storage exists | Stale: SQLite, migration, repositories, and managed CV files exist |
| No create/save backend contract exists | Stale: Add Job is implemented and QML-connected |
| Dashboard metrics are construction-time snapshots | Stale: refreshed on job insertion/reset |
| Five test executables exist | Stale: six are registered |
| Company relationships can drift between independent seed sets | Superseded: persisted jobs cannot retain any company ID |
| `openCv()` emits only a filename | Still current |
| Qt Test is required when tests are disabled | Still current |
| CV controller indentation is inconsistent | Still current |
| A stray `//t` and seed typo exist | No longer current |

## Recommended Direction

1. Stabilize and build/test the current persistence slice before adding
   another feature layer.
2. Implement stored CV opening and persist favorite updates.
3. Define canonical company identity and add `company_id` through a versioned
   migration before reviving company-job linkage.
4. Give recent models an explicit durable timestamp order or insert new rows
   at the front.
5. Add company/contact repositories only with their actual product workflow;
   until then, treat those pages explicitly as empty placeholders.
6. Move CV hashing/copying and transactional creation off the GUI thread if
   imported documents can be large.
7. Add reconciliation for orphaned stored files.
8. Preserve selection by stable ID across inserts, filters, and proxy sorts.
9. Expand tests around new cross-model updates and persistence contracts.

## Verification Status

Verified statically against the current source, tests, CMake registration,
Git working-tree state, previous research artifact, and relevant QML call
sites.

The requested `cpp_researcher` subagent completed an independent read-only
pass. The lead Codex reconciled its output with direct source inspection and
saved this final artifact.

No build, CTest run, or GUI launch was performed because this was a research
task and the research workflow does not request runtime verification.
