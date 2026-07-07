# QML Review: Whole QML Codebase

Date: 2026-07-07

Status: completed.

Reviewed scope:

- `qml/Main.qml`
- `qml/components/*.qml`
- `qml/pages/*.qml`
- QML registration in `CMakeLists.txt`
- QML-facing backend contracts in `src/app`, `src/jobs`, `src/cvs`, `src/dashboard`, and `src/directory` where needed to validate controller calls, model roles, and business-logic boundaries.

This review used the requested read-only `qml_reviewer` subagent as independent input. The lead review verified the findings against the current source before saving this artifact.

No production QML, C++, CMake, build, or test behavior was changed.

## Inputs Inspected

- `AGENTS.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/build.md`
- `For-Agent/Docs/testing.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `.agents/skills/qml-code-review/SKILL.md`
- `.agents/skills/qml-codebase-research/SKILL.md`
- `.codex/agents/qml-reviewer.toml`
- `For-Agent/Research/qml-research-2026-07-06-whole-qml-codebase.md`
- `For-Agent/Research/cpp-research-2026-07-07-backend-surface.md`
- Current QML files under `qml/`
- `CMakeLists.txt`
- QML-facing backend controller and model headers/implementations under `src/`

## Findings

### P1: Add/Edit job save paths present false-success or no-op behavior

`JobFormPage` presents an Add Job form and a Save button, but the save path only closes the form. No entered values are read, validated, or sent to a C++ create/update command.

- `qml/pages/JobFormPage.qml:9-10` exposes only `cancelRequested()` and `saveRequested()`.
- `qml/pages/JobFormPage.qml:100-104` emits `page.saveRequested()` from Save.
- `qml/Main.qml:57-59` handles save with only `window.closeJobForm()`.
- `qml/pages/JobFormPage.qml:131-254` and `qml/pages/JobFormPage.qml:349-416` define the editable job fields, but they are not bound to a draft or command object.
- `src/jobs/JobApplicationsController.h:15-42` exposes selection, filtering, clearing, and selected-application validation only; there is no create/update/save contract.

The job detail pane has the same user-facing risk: fields are editable and the UI contains `Change CV`, `Discard`, `Save Changes`, `Open Link`, and reminder controls, but those controls do not send an edit/update command.

- `qml/pages/JobDescriptionPane.qml:201-299` binds editable fields to selected application values.
- `qml/pages/JobDescriptionPane.qml:342-363` defines `Change CV` without an action.
- `qml/pages/JobDescriptionPane.qml:469-508` defines `Discard` and `Save Changes` without save/discard handlers.
- `qml/pages/JobDescriptionPane.qml:803-819` defines `Open Link` as an inert visual rectangle.
- `qml/pages/JobDescriptionPane.qml:831-849` defines `Mark Next Step / Add Reminder` without an action.

This conflicts with the project boundary that durable product behavior, validation, and state mutation should live in C++. The current UI can make users believe changes were accepted while no durable behavior exists.

### P1: CV preview action is mislabeled and currently has no completed app behavior

The CV preview footer says `View all ... applications`, but it emits the preview's `openCvRequested()` signal. `CvLibraryPage` maps that signal to `cvLibraryController.openCv(page.selectedCv.id)`, and the controller only emits `openCvRequested(filePath)`. Static search found no receiver for that C++ signal.

- `qml/pages/CvLibraryPreviewPanel.qml:20-21` declares `favoriteToggled()` and `openCvRequested()`.
- `qml/pages/CvLibraryPreviewPanel.qml:228-237` labels the control as viewing linked applications but emits `root.openCvRequested()`.
- `qml/pages/CvLibraryPage.qml:117-118` calls `cvLibraryController.openCv(page.selectedCv.id)`.
- `src/cvs/CvLibraryController.cpp:238-248` emits `openCvRequested(cv->fileName_)` or `operationFailed(...)`.
- `src/cvs/CvLibraryController.h:62-63` declares `openCvRequested(QString filePath)` and `operationFailed(QString message)`.
- `src/app/AppBootstrap.cpp:38-45` exposes the controller but does not connect those signals to a document opener or error UI.

The result is a visible action with the wrong semantic wiring and no completed behavior in the app shell.

### P2: Sidebar exposes unreachable pages and inert actions

The sidebar includes `Settings`, but its click handler blocks index 5. At the same time, `Main.qml` uses stack index 5 for `JobFormPage`, not `SettingsPage`.

- `qml/components/Sidebar.qml:15` includes `Settings`.
- `qml/components/Sidebar.qml:77-80` navigates only when `index < 5`.
- `qml/Main.qml:47-60` maps stack index 5 to `JobFormPage`.
- `CMakeLists.txt:120-122` registers `MemoryPage.qml`, `ProfilePage.qml`, and `SettingsPage.qml`, but `Main.qml` never instantiates them.

The sidebar also renders an Add CV quick action without a click handler.

- `qml/components/Sidebar.qml:108-120` defines the `Add CV` button but no `onClicked`.

This is not a QML load failure, but it leaves visible navigation/action affordances that do nothing.

### P2: Companies and Contacts are likely to clip at the declared minimum width

`Main.qml` allows a minimum width of 1180 and reserves 242 px for the sidebar, leaving about 938 px for page content before margins. Companies and Contacts then reserve fixed-width right panels while their left tables request wide fixed columns.

- `qml/Main.qml:11-12` sets `minimumWidth: 1180` and `minimumHeight: 720`.
- `qml/Main.qml:36-38` reserves a 242 px sidebar.
- `qml/pages/ContactsPage.qml:169-177` defines contact table columns totaling about 1010 px before margins.
- `qml/pages/ContactsPage.qml:199-201` also reserves a 430 px right panel.
- `qml/pages/CompaniesPage.qml:113-121` defines company table columns totaling about 895 px before margins.
- `qml/pages/CompaniesPage.qml:151-153` also reserves a 455 px right panel.

Static review cannot prove final rendering, but the math is unfavorable enough that runtime checks at `1180x720` should be treated as required before accepting the UI as stable.

### P2: Backend-interpreted option contracts are still duplicated in QML strings

Several filter/sort option lists live in QML as exact display strings while C++ interprets those same strings as product behavior.

- Jobs status options: `qml/pages/JobsApplicationsPane.qml:25` and `qml/pages/JobsApplicationsPane.qml:131-136`; interpreted by `src/jobs/JobApplicationsController.cpp:110-116`.
- Company sort modes: `qml/pages/CompaniesPage.qml:56-61`; interpreted by `src/directory/CompanyDirectoryController.cpp:102-116`.
- Contact company/channel/sort options: `qml/pages/ContactsPage.qml:79-84`, `qml/pages/ContactsPage.qml:102-107`, and `qml/pages/ContactsPage.qml:124-129`; interpreted by `src/directory/ContactDirectoryController.cpp:110-162`.
- CV language/sort options: `qml/pages/CvLibraryBrowserPane.qml:107-118` and `qml/pages/CvLibraryBrowserPane.qml:366-371`; interpreted by `src/cvs/CvLibraryController.cpp:172-202`.

This is acceptable only as a prototype shortcut. Once these filters and sort modes are product contracts, C++ should expose stable option values or enum-like ids so QML does not own backend-facing behavior strings.

### P3: Some visible add/search controls are placeholders without behavior

Several visible controls are intentionally styled like product actions but have no command path yet:

- `qml/pages/CompaniesPage.qml:81-96` defines `Add Company` without an action.
- `qml/pages/ContactsPage.qml:58-73` defines `Add Contact` without an action.
- `qml/pages/DashboardPage.qml:45-56` defines a global search field that is not bound to backend/app search state.
- `qml/pages/CvLibraryBrowserPane.qml:121-126` defines a `Used in Jobs` filter with only `Any` and no outgoing signal.
- `qml/pages/JobsApplicationsPane.qml:658-675` defines `Mark Next Step / Add Reminder` without an action.

These may be acceptable placeholders, but they should either be hidden/disabled until implemented or made explicit product decisions before a product-ready pass.

### P3: Some icon/glyph strings are actual mojibake in source

This was verified by reading the file text and checking character codes, not by relying on PowerShell terminal rendering. Some icon strings contain Cyrillic and punctuation sequences that are typical mojibake rather than the intended symbols.

Examples:

- `qml/pages/JobFormPage.qml:66`
- `qml/pages/JobFormPage.qml:171`
- `qml/pages/JobFormPage.qml:331`
- `qml/pages/JobFormPage.qml:399`
- `qml/pages/JobDescriptionPane.qml:69`
- `qml/pages/JobDescriptionPane.qml:228`
- `qml/pages/JobDescriptionPane.qml:370`
- `qml/pages/JobDescriptionPane.qml:443`
- `qml/pages/JobDescriptionPane.qml:653-654`
- `qml/pages/JobDescriptionPane.qml:700`
- `qml/pages/JobDescriptionPane.qml:778`
- `qml/pages/JobDescriptionPane.qml:834`

By contrast, `qml/components/Sidebar.qml:16` and `qml/pages/JobsApplicationsPane.qml:352` contain valid Unicode symbols in the file and were not counted as defects.

## Checks With No Findings

- CMake QML registration is complete. A direct compare between `rg --files qml` and `CMakeLists.txt:99-122` found no missing or extra QML files.
- No obvious QML model role-name mismatches were found in the inspected jobs, CV, dashboard, company, or contact delegates.
- Current backend context properties used by QML are exposed in `src/app/AppBootstrap.cpp:40-44`.
- `qml/Main.qml` is still small and focused apart from the routing gap around placeholder pages and the job form.

## Open Questions

- Should `SettingsPage`, `ProfilePage`, and `MemoryPage` be reachable now, hidden until implemented, or removed from the shell until product scope is clear?
- Is Add/Edit Job intended to remain visual-only, or should the next implementation pass add the C++ create/update/validation contract?
- Should CV opening be handled through `QDesktopServices`, an app-level signal handler, or a platform-aware document service?
- Should placeholder add/reminder/global-search controls be hidden, disabled, or wired in the next UI/backend pass?

## Build Or Runtime Verification Performed

No build, CTest, or runtime UI session was run. This was a read-only review plus a saved documentation artifact, and `For-Agent/Docs/build.md` says documentation-only changes do not require a C++ build unless build behavior is affected.

Static verification performed:

- Listed all QML files with `rg --files qml`.
- Counted current QML file lengths.
- Compared current QML files against `qt_add_qml_module` registration in `CMakeLists.txt`; no mismatches were reported.
- Inspected navigation and stack wiring in `qml/Main.qml` and `qml/components/Sidebar.qml`.
- Inspected visible action controls across `qml/pages` and `qml/components`.
- Inspected QML-facing backend contracts and controller methods used by QML.
- Searched for `openCvRequested` and `operationFailed` receivers; none were found outside declarations/emissions and prior documentation.
- Verified suspected mojibake lines by character-code inspection.
- Used the requested `qml_reviewer` subagent for an independent read-only review pass.

## Required Manual UI Checks

- Launch the app and check the QML console for import, binding, and runtime warnings.
- Exercise sidebar navigation, especially Settings and Add CV.
- Open Add Job, enter values, press Save, and verify whether persistence should happen or the UI should be marked placeholder.
- Open Job Description, edit fields, use Change CV, Save Changes, Open Link, and reminder controls.
- Test CV favorite and the preview footer action.
- Resize to `1180x720` and inspect Contacts, Companies, Jobs, CV Library, Dashboard, and Job Description for clipping or overlap.
- Visually inspect the listed mojibake lines in the running UI.

## Missing Tests Or Unverified Behavior

- Add/Edit Job create/update/validation behavior has no backend contract yet, so it also has no tests.
- CV open request behavior is not connected at the app level and is not covered by a user-visible runtime check.
- Placeholder actions are not covered by tests or disabled-state assertions.
- Layout risk was assessed statically only; actual rendering needs a runtime pass.
- Glyph rendering needs a runtime pass after the mojibake strings are corrected.

## Residual Risks

- Some no-op controls may be intentional prototype placeholders, but visible enabled controls still create user-facing false affordances.
- Static review cannot prove text overlap, font fallback, or runtime binding warnings.
- The largest QML files remain maintainability risks: `JobDescriptionPane.qml`, `JobsApplicationsPane.qml`, `CvLibraryBrowserPane.qml`, `ContactsPage.qml`, `JobFormPage.qml`, and `CompaniesPage.qml`.
- Backend seed data, storage, and cross-domain consistency risks are documented in the C++ research artifact but were not expanded here unless they affected the QML contract.

## Final Recommendation

Revise before treating the QML shell as product-ready.

The QML module registration is complete and the main model-role usage looks coherent, but the visible false-success/no-op actions, incomplete CV open contract, unreachable navigation, minimum-size layout risk, backend-interpreted QML option strings, and mojibake icon strings should be addressed in follow-up passes.
