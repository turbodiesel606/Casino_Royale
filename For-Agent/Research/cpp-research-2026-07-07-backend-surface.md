# C++ Research: Backend Surface, 2026-07-07

Scope: read-only research of the whole current JobTracker C++ backend surface.
This artifact consolidates the lead Codex pass and the requested
`cpp_researcher` subagent pass.

No production C++, QML, CMake, build, or test behavior was changed by this
research task.

## Context Used

- `AGENTS.md`
- `.agents/skills/cpp-code-research/SKILL.md`
- `.codex/agents/cpp-researcher.toml`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- Existing QML research artifact:
  `For-Agent/Research/qml-research-2026-07-06-whole-qml-codebase.md`
- Existing task reports under `For-Agent/Task-Report/`, especially the
  QML-to-C++ migration pass reports.

## Files And Areas Inspected

- Startup and wiring: `src/main.cpp`, `src/app/AppBootstrap.h`,
  `src/app/AppBootstrap.cpp`
- Shared backend utilities: `src/common/RoleFilterProxyModel.*`,
  `src/common/ValidationService.*`, `src/common/ValidationResult.h`
- Jobs domain: `src/jobs/JobApplication.h`,
  `src/jobs/JobApplicationListModel.*`,
  `src/jobs/JobApplicationsController.*`
- CV domain: `src/cvs/CvDocument.h`, `src/cvs/CvListModel.*`,
  `src/cvs/LinkedApplicationListModel.*`,
  `src/cvs/CvLibraryController.*`
- Dashboard domain: `src/dashboard/DashboardMetric.h`,
  `src/dashboard/DashboardMetricListModel.*`,
  `src/dashboard/DashboardRecentApplicationsModel.*`,
  `src/dashboard/DashboardRecentCvsModel.*`,
  `src/dashboard/DashboardController.*`
- Company/contact domain: `src/directory/Company.h`,
  `src/directory/CompanyListModel.*`,
  `src/directory/Contact.h`, `src/directory/ContactListModel.*`,
  `src/directory/ContactInteractionListModel.*`,
  `src/directory/LinkedCompanyJobsModel.*`,
  `src/directory/LinkedCompanyContactsModel.*`,
  `src/directory/CompanyDirectoryController.*`,
  `src/directory/ContactDirectoryController.*`
- Tests: `tests/common`, `tests/jobs`, `tests/cvs`, `tests/dashboard`,
  `tests/directory`
- Build registration: `CMakeLists.txt`, `CMakePresets.json`
- QML contract call sites only where needed:
  `qml/pages/JobsPage.qml`, `CvLibraryPage.qml`, `DashboardPage.qml`,
  `CompaniesPage.qml`, `ContactsPage.qml`, and supporting page panes.

There is no current `include/` directory. The current C++ header/source surface
lives under `src/`.

## Current Inventory

Current C++ shape by source area:

- `src/app`: startup bootstrap and QML engine wiring.
- `src/common`: shared filtering, sorting, search, and validation helpers.
- `src/jobs`: job application value type, seeded list model, and
  QML-facing jobs controller.
- `src/cvs`: CV value type, seeded list model, linked job lookup model, and
  QML-facing CV library controller.
- `src/dashboard`: read-only dashboard metric models and recent-item adapter
  models derived from job/CV source models.
- `src/directory`: company and contact value types, seeded list models,
  linked company job/contact models, contact interaction history model, and
  QML-facing directory controllers.
- `tests`: five Qt Test executables covering common helpers and the four
  current product/domain controller areas.

## Startup And Ownership

`src/main.cpp` is intentionally thin. It creates `QGuiApplication`,
constructs `AppBootstrap`, calls `run()`, catches startup exceptions, logs via
`qCritical()`, and returns `EXIT_FAILURE` on failure (`src/main.cpp:9-22`).

`AppBootstrap` owns the long-lived backend objects and the
`QQmlApplicationEngine` (`src/app/AppBootstrap.h:25-32`). It wires backend
objects in constructor order:

- `JobApplicationsController` owns the canonical seeded job model.
- `CvLibraryController` receives the job source model for linked applications.
- `DashboardController` receives the job source model and CV source model.
- `ContactListModel` is owned separately so both directory controllers can use
  one contact source.
- `CompanyDirectoryController` receives the job source model and contact model.
- `ContactDirectoryController` receives the contact model.

`AppBootstrap::configureContextProperties()` exposes five QML context
properties (`src/app/AppBootstrap.cpp:38-44`):

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

`AppBootstrap::loadMainQml()` loads `qrc:/JobTracker/qml/Main.qml` and throws
if the engine has no root objects (`src/app/AppBootstrap.cpp:47-53`).

## Shared Utility Layer

`RoleFilterProxyModel` is a reusable `QSortFilterProxyModel` wrapper for:

- tokenized case-insensitive search across configured roles;
- exact role filters;
- one required-non-empty role filter;
- role-based sorting with numeric, English `MMM d, yyyy` date, then
  case-folded locale-aware string comparison.

Important locations:

- Public API: `src/common/RoleFilterProxyModel.h:14-21`
- Filtering and sorting implementation:
  `src/common/RoleFilterProxyModel.cpp:24-173`

`ValidationService` currently provides small shared validation rules:

- required-field validation for `QVariantMap` values;
- HTTP/HTTPS URL validation through `QUrl`.

Important locations:

- API: `src/common/ValidationService.h:7-11`
- Implementation: `src/common/ValidationService.cpp:5-40`

## Jobs Domain

`JobApplication` is a value type with the current job-application display and
domain-ish fields (`src/jobs/JobApplication.h:6-28`).

`JobApplicationListModel` owns seeded in-memory `QVector<JobApplication>` data
and exposes QML roles for identity, company, job title, URL, location, salary,
status, dates, CV link, description, requirements, tech stack, and notes
(`src/jobs/JobApplicationListModel.h:13-35`). The seed data is embedded in
`makeSeedApplications()` (`src/jobs/JobApplicationListModel.cpp:64-78`).

`JobApplicationsController` exposes:

- `applicationsModel`, `applicationCount`, selected index/id/map;
- `searchText`, `statusFilter`, `resultSummary`;
- invokable selection, search, status filtering, clear filters, and selected
  application validation.

Important locations:

- Contract: `src/jobs/JobApplicationsController.h:15-42`
- Search/status filtering and summary:
  `src/jobs/JobApplicationsController.cpp:70-138`
- Validation: `src/jobs/JobApplicationsController.cpp:140-153`
- Selected-application map:
  `src/jobs/JobApplicationsController.cpp:185-209`

QML consumes this contract primarily in `qml/pages/JobsPage.qml` and
`qml/pages/JobsApplicationsPane.qml`.

## CV Domain

`CvDocument` is a value type for CV metadata and linked application ids
(`src/cvs/CvDocument.h:6-20`).

`CvListModel` owns seeded in-memory `QVector<CvDocument>` data and exposes QML
roles for identity, file name, title, category, language, last modified, size,
description, linked application counts, and favorite state
(`src/cvs/CvListModel.h:13-27`). The seed data is embedded in
`makeSeedCvs()` (`src/cvs/CvListModel.cpp:42-98`).

`LinkedApplicationListModel` derives rows from `JobApplicationListModel` by
matching each application's `cvId` against the selected CV id. It exposes a
small linked-job role set and rebuilds via model reset when the selected CV
changes (`src/cvs/LinkedApplicationListModel.cpp:57-83`).

`CvLibraryController` exposes:

- `cvModel`, `linkedApplicationsModel`, `categorySummary`;
- selected CV index/id/map, filters, sort mode, and result summary;
- invokable selection, search, category/language filtering, sorting, clear
  filters, favorite toggling, and open-CV request emission.

Important locations:

- Contract: `src/cvs/CvLibraryController.h:16-63`
- Search/filter/sort: `src/cvs/CvLibraryController.cpp:142-224`
- Favorite toggle and open request:
  `src/cvs/CvLibraryController.cpp:226-249`
- Selected-CV map: `src/cvs/CvLibraryController.cpp:251-268`
- Linked applications update:
  `src/cvs/CvLibraryController.cpp:299-302`

QML consumes this contract primarily in `qml/pages/CvLibraryPage.qml`,
`qml/pages/CvLibraryBrowserPane.qml`, and
`qml/pages/CvLibraryPreviewPanel.qml`.

## Dashboard Domain

`DashboardController` is read-only and derives dashboard models from the
canonical job and CV models supplied by `AppBootstrap`
(`src/dashboard/DashboardController.h:16-27`).

It owns:

- `DashboardMetricListModel statsModel_`
- `DashboardMetricListModel funnelModel_`
- `DashboardRecentApplicationsModel recentApplicationsModel_`
- `DashboardRecentCvsModel recentCvsModel_`

Stats/funnel metrics are computed at construction from job status counts
(`src/dashboard/DashboardController.cpp:36-99`). Recent application and recent
CV models keep references to the source models and expose the first five rows
through dashboard-specific roles
(`src/dashboard/DashboardRecentApplicationsModel.cpp:17-57`,
`src/dashboard/DashboardRecentCvsModel.cpp:17-63`).

QML consumes these models in `qml/pages/DashboardPage.qml`,
`qml/pages/DashboardStatsRow.qml`, `qml/pages/DashboardActivityPanels.qml`,
and `qml/pages/DashboardRecentCvsPanel.qml`.

## Company And Contact Domain

`Company` is a value type for company display fields and stored counts
(`src/directory/Company.h:5-17`). `CompanyListModel` owns seeded in-memory
company rows and exposes identity, website, logo, open-job count, contact
count, last activity, description, and notes roles
(`src/directory/CompanyListModel.h:13-26`). The seed data is embedded in
`makeSeedCompanies()` (`src/directory/CompanyListModel.cpp:38-95`).

`Contact` and `ContactInteraction` are value types for contact details and
per-contact interaction history (`src/directory/Contact.h:6-30`).
`ContactListModel` owns seeded contacts and exposes roles for identity,
display name, company linkage, related application linkage, channels, last
contact, and notes (`src/directory/ContactListModel.h:13-27`). The seed data
is embedded in `makeSeedContacts()`
(`src/directory/ContactListModel.cpp:53-146`).

`LinkedCompanyJobsModel` derives rows from `JobApplicationListModel` by
matching `companyId` (`src/directory/LinkedCompanyJobsModel.cpp:54-80`).
`LinkedCompanyContactsModel` derives rows from `ContactListModel` by matching
`companyId` (`src/directory/LinkedCompanyContactsModel.cpp:63-89`).
`ContactInteractionListModel` owns the selected contact's interaction rows via
model reset (`src/directory/ContactInteractionListModel.cpp:49-55`).

`CompanyDirectoryController` exposes:

- `companyModel`, linked jobs, linked contacts;
- selected company index/id/map, search text, sort mode, and result summary;
- invokable selection, search, sorting, and clear filters.

Important locations:

- Contract: `src/directory/CompanyDirectoryController.h:16-50`
- Search/sort: `src/directory/CompanyDirectoryController.cpp:87-134`
- Linked model update:
  `src/directory/CompanyDirectoryController.cpp:180-184`

`ContactDirectoryController` exposes:

- `contactModel`, interaction history;
- selected contact index/id/map, search text, company filter, channel filter,
  sort mode, and result summary;
- invokable selection, search, filtering, sorting, and clear filters.

Important locations:

- Contract: `src/directory/ContactDirectoryController.h:15-53`
- Search/filter/sort: `src/directory/ContactDirectoryController.cpp:95-184`
- Interaction history update:
  `src/directory/ContactDirectoryController.cpp:234-238`

QML consumes these contracts in `qml/pages/CompaniesPage.qml` and
`qml/pages/ContactsPage.qml`.

## Cross-Domain Data Flow

Current canonical source models are:

- jobs: `JobApplicationListModel`
- CVs: `CvListModel`
- companies: `CompanyListModel`
- contacts: `ContactListModel`

Cross-domain joins are currently ID-string based:

- CV linked applications: `CvDocument::linkedApplicationIds_` and selected
  `cvId` are matched against `JobApplication::cvId_`.
- Company linked jobs: selected company id is matched against
  `JobApplication::companyId_`.
- Company linked contacts: selected company id is matched against
  `Contact::companyId_`.
- Contact related application: contacts store
  `relatedApplicationId_` and `relatedApplicationTitle_`, but there is no
  controller-level join to the job model yet.

This is simple and works for the immutable seed state. Once editing, deletion,
storage, or import/export exists, the relationships should move toward a
single canonical data/service layer so counts, IDs, and derived lists cannot
drift independently.

## QML-Facing Contract Summary

Current context properties:

- `jobApplicationsController`: consumed by Jobs pages.
- `cvLibraryController`: consumed by CV Library pages.
- `dashboardController`: consumed by Dashboard pages.
- `companyDirectoryController`: consumed by Companies page.
- `contactDirectoryController`: consumed by Contacts page.

Current C++-owned durable behavior exposed to QML:

- job selection, search, status filter, summary, validation;
- CV selection, search, category/language filters, sort, favorite toggle,
  linked job lookup, open-CV request signal;
- dashboard stats/funnel/recent read models;
- company selection, search, sort, linked job/contact lookup;
- contact selection, search, company/channel filters, sort, interaction
  history lookup;
- shared role-based filtering/search/sort and basic validation.

Current QML-visible gaps:

- `JobFormPage.qml` has `saveRequested` and `cancelRequested` signals, but no
  C++ create/update/save contract exists yet
  (`qml/pages/JobFormPage.qml:9-10`, `qml/pages/JobFormPage.qml:104`,
  `qml/pages/JobFormPage.qml:429`).
- Add/edit/delete controls remain visual or placeholder-only according to the
  prior pass reports.
- Dashboard global search is visual only.
- Option lists for filters/sort modes are still mostly hard-coded in QML while
  C++ interprets the selected strings.

## CMake And Build Registration

`CMakeLists.txt` groups source files by domain:

- `JOBTRACKER_COMMON_SOURCES`: `CMakeLists.txt:22-28`
- `JOBTRACKER_JOB_SOURCES`: `CMakeLists.txt:30-36`
- `JOBTRACKER_CV_SOURCES`: `CMakeLists.txt:38-46`
- `JOBTRACKER_DASHBOARD_SOURCES`: `CMakeLists.txt:48-58`
- `JOBTRACKER_DIRECTORY_SOURCES`: `CMakeLists.txt:60-77`

`JobTrackerApp` includes all current source groups plus `src/app` and
`src/main.cpp` (`CMakeLists.txt:79-88`). The QML module is registered with URI
`JobTracker` and resource prefix `/` (`CMakeLists.txt:94-123`).

Tests are registered behind `JOBTRACKER_BUILD_TESTS`
(`CMakeLists.txt:149-251`):

- `JobTrackerCommonTests`
- `JobTrackerJobsTests`
- `JobTrackerCvTests`
- `JobTrackerDashboardTests`
- `JobTrackerDirectoryTests`

Lead verification compared current filesystem `src/**/*.h`, `src/**/*.cpp`,
and `tests/**/*.cpp` against CMake references. No missing source or test
registration was found.

Minor build-system risk: `find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Test)`
currently requires the Qt Test component even when `JOBTRACKER_BUILD_TESTS` is
off (`CMakeLists.txt:14`). If production-only configure should avoid the Test
component, gate `Qt6::Test` discovery behind `JOBTRACKER_BUILD_TESTS` later.

## Current Test Coverage

Current registered Qt Test coverage:

- `tests/common/FilterAndValidationTest.cpp`
  - role-search across configured roles;
  - exact filtering plus required-non-empty role filtering;
  - string and numeric sorting;
  - required-field and URL validation.
- `tests/jobs/JobApplicationsControllerTest.cpp`
  - job model role names;
  - seeded application rows;
  - selection and invalid selection behavior;
  - search/status filtering;
  - selected-application validation.
- `tests/cvs/CvLibraryControllerTest.cpp`
  - CV model role names;
  - seeded CV rows;
  - linked applications for selected CV;
  - favorite toggle visible through selected CV;
  - search/category/sort behavior.
- `tests/dashboard/DashboardControllerTest.cpp`
  - dashboard status aggregates;
  - funnel ratios;
  - recent application and recent CV model rows.
- `tests/directory/DirectoryControllerTest.cpp`
  - company/contact role names and seeded rows;
  - company selection with linked jobs/contacts;
  - contact selection with interaction history;
  - company search/sort;
  - contact company/channel filters and sorting.

Coverage gaps worth addressing as the backend becomes mutable:

- `CvLibraryController::openCv()` signal behavior:
  `openCvRequested` and `operationFailed` are not directly tested.
- `CvListModel::toggleFavorite()` emits `dataChanged`, but tests currently
  check selected map state rather than spying on the model signal.
- Empty-result selection behavior is not deeply covered across all controllers.
- `AppBootstrap` startup/QML-load failure behavior has no automated test.
- Cross-domain ID consistency is not tested.
- There are no storage, import/export, mutation, create/update/delete, or
  durable persistence tests because those layers do not exist yet.

## Risks And Maintainability Notes

1. Durable storage does not exist yet.
   All product records are seeded in C++ list model constructors. This is a
   good improvement over QML-owned mock arrays, but still not a persistence
   layer.

2. Several models combine domain storage and seed-data construction.
   `JobApplicationListModel`, `CvListModel`, `CompanyListModel`, and
   `ContactListModel` each own both model behavior and embedded seed fixtures.
   Once data becomes editable, move seed/demo data and persistence into a
   separate repository/storage/service layer.

3. Cross-domain relationships can drift.
   Job applications reference company ids that are not present in the current
   five-company seed list, such as `company-nexora`, `company-byteworks`,
   `company-innotech`, `company-platforma`, and `company-devsolutions`
   (`src/jobs/JobApplicationListModel.cpp:72-76`,
   `src/directory/CompanyListModel.cpp:38-95`).

4. Company counts are stored instead of derived.
   `Company::openJobCount_` and `Company::contactCount_` can become stale once
   jobs or contacts are mutable (`src/directory/Company.h:12-13`).

5. Dashboard stats/funnel are computed once at construction.
   `DashboardController` initializes `statsModel_` and `funnelModel_` from the
   job model in its constructor (`src/dashboard/DashboardController.cpp:103-108`).
   That is fine for immutable seed data, but future mutable jobs will need
   refresh behavior driven by model changes or a service/controller update path.

6. QML/C++ option strings are duplicated.
   Sort/filter options are still largely hard-coded in QML while C++ compares
   strings such as `All`, `Linked Jobs`, `Contacts`, `Company`, `Last Contact`,
   and channel names. This is acceptable for a small prototype, but a future
   product contract should expose option lists or stable enum-like values from
   C++.

7. `CvLibraryController::openCv()` emits a file name, not an opened document.
   The controller emits `openCvRequested(cv->fileName_)`
   (`src/cvs/CvLibraryController.cpp:238-249`). There is no file path
   resolution, platform opener, or storage-backed CV document path yet.

8. `src/app/AppBootstrap.cpp` has a stray `//t` comment at line 9.
   This is harmless but should be removed in a cleanup pass.

9. `src/jobs/JobApplicationListModel.cpp` has a seed-data typo:
   `Qt/QML Engiqneer` at line 74.

10. `src/cvs/CvLibraryController.cpp` uses tab indentation in several blocks,
    while most project C++ uses spaces. This is a style consistency issue, not
    a behavioral risk.

## Suggested Next C++ Directions

1. Introduce a storage/repository/service layer before implementing add/edit,
   delete, import/export, or persistence. Keep QML-facing controllers thin.

2. Add a real Add/Edit Job C++ contract:
   - draft state or command object;
   - create/update command;
   - validation result exposure;
   - operation failure signal or error property;
   - tests for success, validation errors, and unchanged/cancel paths.

3. Make cross-domain relationships canonical:
   - derive company open-job/contact counts from job/contact source data;
   - validate all seed/demo IDs;
   - avoid duplicating related titles when IDs can be resolved.

4. Make dashboard read models refreshable when source models become mutable:
   - connect to model reset/rows/data change signals;
   - rebuild stats/funnel models or expose resettable read models;
   - test refresh behavior with `QSignalSpy`.

5. Move durable option lists into C++ once they are product contracts:
   - job statuses;
   - CV categories/languages/sort modes;
   - company/contact sort modes;
   - contact channels.

6. Add focused tests for current uncovered contracts:
   - `openCvRequested` and `operationFailed`;
   - `CvListModel::dataChanged` on favorite toggle;
   - empty filter-result selection behavior;
   - cross-domain seed consistency.

7. Consider gating Qt Test discovery in CMake behind
   `JOBTRACKER_BUILD_TESTS` if production-only configure should not require
   the Qt Test component.

## Verification Performed For This Research

- Read required instruction layers and C++ research skill.
- Used the requested `cpp_researcher` subagent for an independent read-only
  C++ backend pass.
- Inspected all current C++ directories under `src/`.
- Inspected test registration and current tests under `tests/`.
- Inspected CMake source and test registration.
- Compared `src/**/*.h`, `src/**/*.cpp`, and `tests/**/*.cpp` against
  `CMakeLists.txt` references; no missing registrations were found.
- Searched QML only for backend controller call sites and contract usage.
- Searched for persistence/file/database APIs and found no current storage
  implementation in `src/` or `tests/`.
- Searched for placeholder/hygiene markers and found the documented `//t`
  comment plus the seed-data typo.

No build or CTest run was performed because this task only adds a research
artifact. Per `For-Agent/Docs/build.md`, documentation-only changes do not
require a C++ build unless build behavior is affected.

## Manual Checks For Future Implementation

- App starts and each page still receives its context property.
- Jobs, CVs, Dashboard, Companies, and Contacts screens render current backend
  seed data.
- Search/filter/sort behavior still matches current tests and visible UI.
- Favorite toggle updates both the selected CV preview and list role.
- Empty search/filter results do not leave stale preview state.
- Add/Edit Job behavior is not treated as durable until a C++ contract exists.

## Final Notes

This codebase is past the QML-mock stage for the main list/detail screens:
durable-ish product state now lives in C++ models and controllers, and QML is
mostly consuming exposed contracts. The next architectural step is not another
controller pile-on; it is a small data/service boundary that lets editing,
storage, relationships, and dashboard refresh behavior stay coherent.
