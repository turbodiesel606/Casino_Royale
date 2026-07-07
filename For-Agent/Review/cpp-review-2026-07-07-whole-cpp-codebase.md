# C++ Review: Whole C++ Codebase

Date: 2026-07-07

Status: completed.

Reviewed scope:

- `src/main.cpp`
- `src/app/*.h`, `src/app/*.cpp`
- `src/common/*.h`, `src/common/*.cpp`
- `src/jobs/*.h`, `src/jobs/*.cpp`
- `src/cvs/*.h`, `src/cvs/*.cpp`
- `src/dashboard/*.h`, `src/dashboard/*.cpp`
- `src/directory/*.h`, `src/directory/*.cpp`
- `tests/**/*.cpp`
- C++ and test registration in `CMakeLists.txt`
- QML call sites only where needed to validate C++ contracts consumed by QML.

This review used the requested read-only `cpp_reviewer` subagent as
independent input. The lead review verified the findings against the current
source before saving this artifact.

No production C++, QML, CMake, build, or test behavior was changed.

## Inputs Inspected

- `AGENTS.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/coding-style.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `.agents/skills/cpp-code-review/SKILL.md`
- `.codex/agents/cpp-reviewer.toml`
- `For-Agent/Research/cpp-research-2026-07-07-backend-surface.md`
- `For-Agent/Review/qml-review-2026-07-07-whole-qml-codebase.md`
- Current C++ files under `src/`
- Current tests under `tests/`
- `CMakeLists.txt`
- Targeted QML contract call sites under `qml/pages`

## Findings

### P2: Sort/filter refresh can silently switch the selected entity

The CV, company, and contact controllers store selection as a proxy row index.
When sorting or filtering changes the proxy order, `refreshSelectionAfterFilterChange()`
only clamps the existing row number into the new row range. It does not preserve
the selected entity id and then find the new proxy row for that id.

- `src/cvs/CvLibraryController.cpp:187-204` changes the CV sort role.
- `src/cvs/CvLibraryController.cpp:287-295` clamps `selectedCvIndex_` after
  the proxy model changes.
- `src/cvs/CvLibraryController.cpp:277-284` derives the selected CV from the
  current proxy row.
- `src/directory/CompanyDirectoryController.cpp:102-119` changes company sort.
- `src/directory/CompanyDirectoryController.cpp:152-160` clamps
  `selectedCompanyIndex_`.
- `src/directory/ContactDirectoryController.cpp:148-165` changes contact sort.
- `src/directory/ContactDirectoryController.cpp:202-210` clamps
  `selectedContactIndex_`.

Concrete CV example: selecting row 2 currently selects `cv-general`
(`src/cvs/CvListModel.cpp:71-83`). Sorting by `Linked Jobs` sorts by
`LinkedApplicationCountRole` descending (`src/cvs/CvLibraryController.cpp:198-202`).
The same proxy row 2 can then point to `cv-embedded`
(`src/cvs/CvListModel.cpp:58-70`) instead of preserving `cv-general`.

QML binds the highlighted row and detail panel to these selected-index and
selected-map properties:

- `qml/pages/CvLibraryPage.qml:87-100`
- `qml/pages/CompaniesPage.qml:60-61`
- `qml/pages/CompaniesPage.qml:132-133`
- `qml/pages/ContactsPage.qml:128-129`
- `qml/pages/ContactsPage.qml:188-189`

The current tests exercise sorting, but only from the default selection or
without asserting that a previously selected entity remains selected after the
sort:

- `tests/cvs/CvLibraryControllerTest.cpp:114-119`
- `tests/directory/DirectoryControllerTest.cpp:132-137`
- `tests/directory/DirectoryControllerTest.cpp:155-159`

Expected behavior for these list/detail screens is to preserve the selected id
when it remains visible, and only fall back to another row when the selected
entity is filtered out.

### P2: Contact "Last Contact" sorting is lexicographic, not chronological

`ContactsPage` exposes a `Last Contact` sort option, and the controller sorts
by `LastContactLabelRole`. The source data stores relative display labels such
as `2 days ago`, `1 week ago`, and `5 days ago`. `RoleFilterProxyModel` only
parses English absolute dates in `MMM d, yyyy` format, then falls back to
case-folded locale-aware string comparison. That makes `Last Contact` sort by
label text, not actual recency.

- `qml/pages/ContactsPage.qml:124-129` exposes `Last Contact`.
- `src/directory/ContactDirectoryController.cpp:156-160` sorts by
  `ContactListModel::LastContactLabelRole`.
- `src/common/RoleFilterProxyModel.cpp:111-118` parses only absolute
  `MMM d, yyyy` dates before string fallback.
- `src/directory/ContactListModel.cpp:69`, `src/directory/ContactListModel.cpp:89`,
  `src/directory/ContactListModel.cpp:107`, and
  `src/directory/ContactListModel.cpp:125` use relative labels.
- `tests/directory/DirectoryControllerTest.cpp:155-159` covers `Company`
  sorting but not `Last Contact`.

This is a current product-behavior bug because QML presents the option as a
meaningful sort mode.

### P2: Company/job seed data has broken cross-domain IDs

Five job applications reference company ids that are not present in
`CompanyListModel`: `company-nexora`, `company-byteworks`, `company-innotech`,
`company-platforma`, and `company-devsolutions`.

- `src/jobs/JobApplicationListModel.cpp:72-76` contains those job rows.
- `src/directory/CompanyListModel.cpp:38-95` seeds only KDAB, TechSoft,
  Vision Systems, CodeCraft, and GreenWidget.
- `src/directory/LinkedCompanyJobsModel.cpp:75-79` can only derive linked jobs
  by matching a selected company id.

Those five applications are therefore unreachable from the company directory's
linked jobs model. The current directory tests only verify one happy-path
company/job link:

- `tests/directory/DirectoryControllerTest.cpp:57-79`

This is already visible with immutable seed data, and it will become more
fragile once company counts or relationships become editable.

### P3: Company zero-result summary says "Showing 1 to 0 of 0 companies"

`CompanyDirectoryController::resultSummary()` special-cases a single company,
but not zero companies. When search returns no matches, it formats the summary
as `Showing 1 to 0 of 0 companies`.

- `src/directory/CompanyDirectoryController.cpp:69-72`
- `qml/pages/CompaniesPage.qml:140` consumes `resultSummary`.

The jobs controller handles zero explicitly:

- `src/jobs/JobApplicationsController.cpp:70-77`

There is no company test for zero-result search.

### P3: "Open CV" has no in-repo receiver and emits only a file name

`CvLibraryController::openCv()` is exposed to QML and emits
`openCvRequested(QString filePath)`, but the emitted value is only
`cv->fileName_`, not a resolved path. A repo-wide search found no receiver for
`openCvRequested`, so the visible QML action currently has no completed app
behavior.

- `qml/pages/CvLibraryPage.qml:117-118` calls
  `cvLibraryController.openCv(page.selectedCv.id)`.
- `src/cvs/CvLibraryController.h:53-63` exposes `openCv()` and the
  `openCvRequested` / `operationFailed` signals.
- `src/cvs/CvLibraryController.cpp:238-248` emits `cv->fileName_`.
- `src/app/AppBootstrap.cpp:38-45` only exposes the controller as a context
  property and does not connect the signal to a document opener or error UI.

The tests do not cover either `openCvRequested` or `operationFailed`:

- `tests/cvs/CvLibraryControllerTest.cpp:26-31`

This overlaps with the QML review finding around the preview action, but the
C++ contract itself is still incomplete because it promises a `filePath` while
providing only a display file name.

### P3: Runtime-only configure still requires Qt Test

`JOBTRACKER_BUILD_TESTS` defaults to `OFF`, but `find_package()` always requires
the Qt Test component. A production/runtime-only configure can fail on an
environment with Qt Quick available but Qt Test not installed, even though no
tests are being built.

- `CMakeLists.txt:12` defines `JOBTRACKER_BUILD_TESTS` as `OFF` by default.
- `CMakeLists.txt:14` requires `Quick Test` unconditionally.
- `CMakeLists.txt:149-251` only builds and links tests when
  `JOBTRACKER_BUILD_TESTS` is enabled.

Keep `Qt6::Test` discovery and test targets behind the test option if the
project should configure without test tooling.

## Checks With No Findings

- No missing C++ source or test registration was found in `CMakeLists.txt`
  compared with the current `src/` and `tests/` file inventory.
- `src/main.cpp` remains thin and delegates startup wiring to `AppBootstrap`.
- Current QML context properties match the documented backend controllers.
- Current QML-facing role names used by the inspected C++ tests and targeted
  QML call sites were not found to be mismatched.
- Current dashboard models are acceptable for immutable seed data; refresh
  behavior should be revisited when job/CV models become mutable.

## Verification Performed

- Read the required project instruction layers and the C++ review skill.
- Read the existing C++ research artifact and QML review artifact as context.
- Used the requested `cpp_reviewer` subagent for an independent read-only
  review pass.
- Inspected all current C++ source areas under `src/`.
- Inspected all current Qt Test files under `tests/`.
- Inspected `CMakeLists.txt` source and test registration.
- Searched QML call sites only where needed to validate C++ contracts consumed
  by QML.
- Checked current `git status` and `git diff -- src tests CMakeLists.txt`.

No build or CTest run was performed. This task saved a review artifact only,
and `For-Agent/Docs/build.md` says documentation-only changes do not require a
C++ build unless build behavior is affected.

## Missing Tests, Manual Checks, And Unverified Areas

- Add tests that preserve selected CV/company/contact id across sort and filter
  changes when the selected entity remains visible.
- Add a test for contact `Last Contact` sorting once the model exposes a
  sortable timestamp or date role.
- Add a seed-consistency test that verifies every job `companyId` exists in the
  company model or is intentionally external.
- Add a company zero-result summary test.
- Add `QSignalSpy` coverage for `CvLibraryController::openCv()`,
  `openCvRequested`, and `operationFailed`.
- Manually exercise the CV Library, Companies, and Contacts pages after fixes
  to confirm the highlighted row, detail panel, linked models, and summaries
  remain coherent.

## Residual Risks

- Static review cannot prove runtime QML rendering, binding warnings, or
  platform document-opening behavior.
- Durable storage does not exist yet; seed data currently stands in for product
  state.
- Dashboard refresh behavior is a future risk once job/CV data becomes mutable.
- Option lists and backend-interpreted strings remain duplicated between QML
  and C++ in several areas, as documented in the QML review.

## Final Recommendation

Revise before treating the current C++ backend as product-ready.

The architecture is clean enough for the current prototype stage, and the
source/test registration looks coherent. The main issues to fix next are
selection identity across proxy changes, chronological contact sorting,
cross-domain seed ID consistency, company zero-result summaries, and the
unfinished Open CV contract.
