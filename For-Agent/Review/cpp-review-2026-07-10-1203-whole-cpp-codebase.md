# C++ Review: Current Whole Codebase
Created: 2026-07-10 12:03 local time (Asia/Baku)

## Reviewed Scope

- All 62 current C++ headers and sources under `src/`.
- All 6 test translation units and the support header under `tests/`.
- C++ and test registration in `CMakeLists.txt`.
- Startup wiring, storage, repositories, models, controllers, services, and relevant QML-facing contracts.
- Current Git status and diff.
- Required project instructions, the `cpp-code-review` skill, the requested read-only `cpp_reviewer` subagent, and the newest relevant research/review artifacts.

The working tree was clean. This is a static review of commit `f05cde2`, not an uncommitted diff. Existing artifacts and the subagent report were used only as leads; the findings below were reconciled against the current source.

## Findings

### P2: CV favorite changes are presented as durable state but are never persisted

The schema stores `is_favorite`, and `CvRepository` reads and inserts it, but the favorite action only mutates `CvListModel` memory. `CvRepository` has no update operation, and `CvLibraryController` has no repository dependency. Every favorite change therefore disappears after restart.

- `src/storage/SchemaMigrator.cpp:31-44`
- `src/cvs/CvRepository.h:14-16`
- `src/cvs/CvRepository.cpp:22-32`
- `src/cvs/CvListModel.cpp:125-135`
- `src/cvs/CvLibraryController.cpp:251-261`
- `qml/pages/CvLibraryPage.qml:104-118`

Add a repository update and a reopen test proving that a changed favorite value survives database reconstruction.

### P2: Newly created jobs and CVs can be absent from both recent dashboard lists

Repositories load rows newest-first, but runtime creation appends new rows at the end of the source models. Both recent models expose source rows `0..4` without ordering independently. Once five records exist, a newly created job or newly imported CV can remain outside the visible recent list until restart reloads database order.

- `src/jobs/JobRepository.cpp:29-35`
- `src/cvs/CvRepository.cpp:60-64`
- `src/jobs/JobApplicationListModel.cpp:148-153`
- `src/cvs/CvListModel.cpp:103-109`
- `src/dashboard/DashboardRecentApplicationsModel.cpp:25-49`
- `src/dashboard/DashboardRecentCvsModel.cpp:29-57`
- `qml/pages/DashboardPage.qml:76-85`
- `tests/dashboard/DashboardControllerTest.cpp:61-84`

Define recency using a durable timestamp and keep the order identical after startup and incremental insertion. Add tests that insert after at least five existing rows and assert the new IDs are visible.

### P2: Selection is stored as a proxy row and can silently switch entity after reordering

Job and CV controllers preserve only a numeric proxy index. Filtering, sorting, or insertion can change which entity occupies that row; refresh then clamps the old number rather than locating the previous stable ID. The highlighted row and detail panel can silently move to another record. The job-creation path is especially exposed because appending triggers dynamic proxy sorting before selection refresh.

- `src/jobs/JobApplicationsController.cpp:225-227`
- `src/jobs/JobApplicationsController.cpp:234-257`
- `src/cvs/CvLibraryController.cpp:212-232`
- `src/cvs/CvLibraryController.cpp:296-321`
- `qml/pages/JobsPage.qml:46-70`
- `qml/pages/CvLibraryPage.qml:82-101`

Company and contact controllers use the same index-clamping pattern at `src/directory/CompanyDirectoryController.cpp:145-164` and `src/directory/ContactDirectoryController.cpp:192-211`, although their production models are currently empty.

Preserve selection by stable ID while the entity remains visible. Explicitly decide whether successful creation selects the new record or retains the previous selection.

### P2: Add Job accepts hostless HTTP-like values as valid URLs

`AddJobService` checks only `QUrl::isValid()` and the scheme. Hostless values such as `https:job-posting` can satisfy those checks and be persisted despite not being usable web links. The shared `ValidationService` includes the missing host check, demonstrating rule drift between the two validation paths.

- `src/jobs/AddJobService.cpp:66-73`
- `src/common/ValidationService.cpp:19-37`
- `tests/storage/StorageAndAddJobTest.cpp:179-200`

Use one canonical URL rule and add hostless `http:` and `https:` cases to the storage workflow tests.

### P2: Persisted jobs cannot participate in the company-linked-jobs contract

`JobApplication` contains `companyId_`, and `LinkedCompanyJobsModel` joins strictly by that ID. Schema version 1 stores only `company_name`; Add Job never assigns a company ID, and `JobRepository` cannot reconstruct one. Persisted jobs therefore have an empty `companyId_` and cannot appear in a company's linked jobs.

- `src/jobs/JobApplication.h:6-13`
- `src/storage/SchemaMigrator.cpp:45-63`
- `src/jobs/AddJobService.cpp:89-103`
- `src/jobs/JobRepository.cpp:38-58`
- `src/directory/LinkedCompanyJobsModel.cpp:71-79`

Define canonical company identity and add it through a versioned migration before activating the company directory workflow.

### P2: The visible Open CV action has no completed backend behavior

`openCv()` emits `fileName_`, which is the original display filename, despite the signal parameter being named `filePath`. Imported CVs have managed relative and stored paths, but neither is resolved here. No receiver for `openCvRequested` exists in the repository, so the QML button produces no user-visible result.

- `src/cvs/CvLibraryController.h:57-68`
- `src/cvs/CvLibraryController.cpp:263-273`
- `src/cvs/CvRepository.cpp:12-33`
- `src/app/AppBootstrap.cpp:49-55`
- `qml/pages/CvLibraryPage.qml:104-118`

Resolve the managed path through the storage layer, use a platform-aware opener, surface failure, and cover the emitted contract with `QSignalSpy`.

### P3: `categorySummary` has the wrong notify dependency after CV insertion

`categorySummary` depends on all CV rows and their category counts but uses only `filtersChanged` as its notify signal. `recordCvUse()` changes the source rows and counts while emitting `cvModelChanged` and `resultSummaryChanged`, not `filtersChanged`. The QML category summary can remain stale after importing the first CV in a category.

- `src/cvs/CvLibraryController.h:16-27`
- `src/cvs/CvLibraryController.cpp:42-57`
- `src/cvs/CvLibraryController.cpp:79-104`
- `qml/pages/CvLibraryPage.qml:79-101`

Notify the property for both filter changes and source-data changes, and add coverage for `recordCvUse()`.

### P3: The selected-application validator contradicts Add Job's URL policy

Add Job treats `jobUrl` as optional, but `validateSelectedApplication()` calls `validateHttpUrl(..., true)`. A successfully persisted application with a blank URL is later reported invalid through the exposed invokable.

- `src/jobs/AddJobService.cpp:66-73`
- `src/jobs/JobApplicationsController.h:48-53`
- `src/jobs/JobApplicationsController.cpp:164-181`

Decide whether the URL is optional or required and share one rule between creation and later validation.

### P3: Runtime-only CMake configuration still requires Qt Test

Tests default to disabled, but `find_package()` always requires the `Test` component. A runtime-only environment cannot configure without test development components even though every test target is guarded by `JOBTRACKER_BUILD_TESTS`.

- `CMakeLists.txt:12-16`
- `CMakeLists.txt:170-295`

Discover `Qt6::Test` only when tests are enabled.

## Open Questions And Assumptions

- Companies and Contacts appear to be intentional empty placeholders: production bootstrap creates empty models and there is no repository or mutation path. If these screens are expected to function now, their absence is a broader incomplete-feature issue.
- Dashboard recency needs a product definition: creation time, applied date, or last update.
- Successful job creation needs an explicit selection rule: select the new application or retain the previous stable selection.
- The creation and selected-record validation APIs disagree on whether a blank job URL is valid.

## Verification Performed

- Read the required project docs, skill, and `cpp_reviewer` contract.
- Used the requested `cpp_reviewer` subagent for an independent read-only pass.
- Inspected all current C++ source/header files, tests, CMake registration, startup wiring, storage, repositories, models, controllers, services, and relevant QML call sites.
- Confirmed all production headers/sources and all test translation units are registered in CMake.
- Confirmed QML context-property names match bootstrap wiring.
- Confirmed declaration/destruction order preserves referenced model, repository, database, service, and controller lifetimes.
- Confirmed `git status` and the current diff were clean.

No build, CTest run, or GUI launch was performed. The project review workflow does not require runtime verification for a review-only task unless requested, so all behavior findings are static-analysis results pending implementation-time build and test verification.

## Missing Tests And Manual Checks

Add automated coverage for:

- favorite persistence across database reopen;
- new rows appearing in recent dashboard lists after five existing rows;
- stable selection across filtering, sorting, and insertion;
- `recordCvUse()` model updates and `categorySummary` notification;
- hostless HTTP/HTTPS URL rejection;
- optional-versus-required job URL policy;
- managed CV path resolution and open failure;
- company ID persistence and linked-job refresh;
- newer-schema rejection and startup failure behavior.

Manually verify Add Job, dashboard recency, CV favorite/restart, Open CV, and selection behavior under active filters after the corresponding fixes.

## Residual Risks

- CV hashing and copying run synchronously on the GUI thread, and hashing reads the entire file into memory at `src/cvs/CvImportService.cpp:39-44`.
- A process crash after final-file rename but before database commit can leave an orphan completed file; startup removes only `*.part` files.
- Directory linked models do not subscribe to source insert/reset/data-change signals.
- Company and contact storage/product workflows remain absent.
- Static inspection cannot prove QML runtime behavior, SQLite deployment, or Windows/Linux document-opening behavior.

## Final Recommendation

**Revise.**

No blocker-level ownership, lifetime, QML context-property, or CMake source-registration defect was found, and the startup/storage layering is coherent. The current commit should not be considered product-ready until favorite durability, recent-list ordering, stable selection, URL validation, company identity, and Open CV behavior are corrected and the full build/CTest flow is run.
