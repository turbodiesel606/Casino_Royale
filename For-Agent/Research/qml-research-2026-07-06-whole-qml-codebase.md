# QML Research: Whole Codebase, 2026-07-06

Status: completed.

Scope: read-only QML architecture research after the QML-to-C++ migration pass reports. This artifact consolidates the lead Codex pass and the requested `qml_researcher` subagent pass.

No QML, C++, CMake, build, or test behavior was changed.

## Context Used

- `AGENTS.md`
- `For-Agent/Docs/architecture.md`
- `For-Agent/Docs/qml-style.md`
- `For-Agent/Docs/qml-to-cpp-extraction.md`
- `.agents/skills/qml-codebase-research/SKILL.md`
- `.agents/skills/qml-code-research/SKILL.md`
- `For-Agent/Task-Report/pass-7-cleanup-and-review.md`
- `CMakeLists.txt`
- `qml/Main.qml`
- `qml/components/*.qml`
- `qml/pages/*.qml`
- QML-facing backend contracts in `src/app`, `src/jobs`, `src/cvs`, `src/dashboard`, and `src/directory`
- `qml_researcher` subagent result from `019f384e-ae76-7320-b598-fe11a166ec5d`

## Inventory

Current QML inventory: 24 files.

- 1 app entry point: `qml/Main.qml`
- 6 shared components: `qml/components/NeonCard.qml`, `Panel.qml`, `PrimaryButton.qml`, `Sidebar.qml`, `SidebarButton.qml`, `StatusChip.qml`
- 17 page/pane files under `qml/pages`

Largest QML files by current physical line count:

| Lines | File |
|---:|---|
| 869 | `qml/pages/JobDescriptionPane.qml` |
| 696 | `qml/pages/JobsApplicationsPane.qml` |
| 588 | `qml/pages/CvLibraryBrowserPane.qml` |
| 553 | `qml/pages/ContactsPage.qml` |
| 450 | `qml/pages/JobFormPage.qml` |
| 443 | `qml/pages/CompaniesPage.qml` |
| 418 | `qml/pages/CvLibraryPreviewPanel.qml` |
| 240 | `qml/pages/DashboardActivityPanels.qml` |
| 226 | `qml/pages/DashboardRecentCvsPanel.qml` |
| 164 | `qml/pages/DashboardPage.qml` |

`CMakeLists.txt:94-123` registers every current `qml/**/*.qml` file in `qt_add_qml_module`; a direct compare between `rg --files qml` and the CMake QML file list produced no mismatches.

## Current Ownership

`qml/Main.qml` owns the app shell: `ApplicationWindow`, sidebar, `StackLayout`, current page index, previous page index, and job form visibility (`qml/Main.qml:7-19`, `qml/Main.qml:21-30`, `qml/Main.qml:36-60`). This is appropriate QML-owned navigation state.

`qml/components/Sidebar.qml` owns sidebar labels/icons and emits `navigate(index)` and `addJob()` (`qml/components/Sidebar.qml:11-16`, `qml/components/Sidebar.qml:50-80`, `qml/components/Sidebar.qml:94-99`).

`qml/pages/DashboardPage.qml` is a composition shell over backend-driven dashboard panes. It consumes `dashboardController.statsModel`, `funnelModel`, `recentApplicationsModel`, and `recentCvsModel` (`qml/pages/DashboardPage.qml:70-86`). Its custom scrollbar math is presentation-only QML behavior (`qml/pages/DashboardPage.qml:116-158`).

`qml/pages/JobsPage.qml` owns the jobs list/detail mode switch through `descriptionMode` (`qml/pages/JobsPage.qml:11`, `qml/pages/JobsPage.qml:71-83`). It passes C++ model/state into `JobsApplicationsPane` and `JobDescriptionPane` (`qml/pages/JobsPage.qml:44-83`).

`qml/pages/CvLibraryPage.qml` is a composition shell over `CvLibraryBrowserPane` and `CvLibraryPreviewPanel`. It delegates search, filters, sort, favorite, open, and selection to `cvLibraryController` (`qml/pages/CvLibraryPage.qml:60-118`).

`qml/pages/CompaniesPage.qml` and `qml/pages/ContactsPage.qml` still combine top-level page layout, filter rows, list/table rendering, detail preview, and local row components in one file each (`qml/pages/CompaniesPage.qml:13-279`, `qml/pages/CompaniesPage.qml:281-443`, `qml/pages/ContactsPage.qml:13-370`, `qml/pages/ContactsPage.qml:373-553`).

`qml/pages/JobFormPage.qml` is currently a visual form surface. It emits `saveRequested` and `cancelRequested`, but `Main.qml` only closes the form on save/cancel (`qml/pages/JobFormPage.qml:9-10`, `qml/pages/JobFormPage.qml:100-104`, `qml/pages/JobFormPage.qml:425-429`, `qml/Main.qml:57-60`).

## Backend Contracts Used By QML

`src/app/AppBootstrap.cpp:40-44` exposes these context properties:

- `jobApplicationsController`
- `cvLibraryController`
- `dashboardController`
- `companyDirectoryController`
- `contactDirectoryController`

The QML-facing controller contracts are now broad enough for the implemented screens:

- `JobApplicationsController`: applications model, selected application map, search text, status filter, result summary, selection, filter clearing, selected-application validation (`src/jobs/JobApplicationsController.h:15-42`).
- `CvLibraryController`: CV model, linked applications model, category summary, selected CV map, search/category/language/sort filters, clear filters, favorite toggle, open CV command (`src/cvs/CvLibraryController.h:16-54`).
- `DashboardController`: stats, funnel, recent applications, and recent CV models (`src/dashboard/DashboardController.h:16-27`).
- `CompanyDirectoryController`: company model, linked jobs/contacts models, selected company map, search/sort, selection, clear filters (`src/directory/CompanyDirectoryController.h:16-44`).
- `ContactDirectoryController`: contact model, interaction history model, selected contact map, search/company/channel/sort filters, selection, clear filters (`src/directory/ContactDirectoryController.h:15-47`).

The implemented durable search/filter/sort paths are mostly already in C++:

- Jobs search/status filtering and result summary: `src/jobs/JobApplicationsController.cpp:90-138`.
- CV search/category/language/sort filtering: `src/cvs/CvLibraryController.cpp:142-224`.
- Company search/sort filtering: `src/directory/CompanyDirectoryController.cpp:87-134`.
- Contact search/company/channel/sort filtering: `src/directory/ContactDirectoryController.cpp:95-184`.

## UI State That Should Stay In QML

Keep these in QML:

- Shell navigation and job-form visibility in `Main.qml`.
- `JobsPage.descriptionMode` list/detail toggle.
- Layout constants, table column geometry, panel sizes, hover state, selected-row styling, and custom scroll visuals.
- Display-only arrays used as local layout metadata, such as jobs table columns and preview labels, as long as they do not become persisted product settings.
- Small component styling in `Panel`, `StatusChip`, `PrimaryButton`, `SidebarButton`, and `CvSearchField`.
- Dashboard scrollbar drag math, because it affects presentation only.

Do not move pure visual state into C++ just to reduce QML line count.

## Durable Behavior And Contract Gaps

### Needed Now

1. Define a real backend contract before treating Add/Edit Job as implemented.

   `JobFormPage.qml` does not bind entered form values to a backend create/update command. `saveRequested` only closes the form through `Main.qml`. A future implementation should expose a small C++ create/update contract, validation result state, and failure reporting before the UI claims durable save behavior.

2. Resolve the sidebar/page routing gap.

   `Sidebar.qml` lists `Settings` as the sixth navigation item (`qml/components/Sidebar.qml:15`), but the delegate only navigates when `index < 5` (`qml/components/Sidebar.qml:77-80`). `Main.qml` uses stack index 5 for `JobFormPage` when `jobFormVisible` is true (`qml/Main.qml:50-60`). `SettingsPage.qml`, `ProfilePage.qml`, and `MemoryPage.qml` are registered in CMake but are not reachable from the main shell.

3. Move hard-coded domain option lists out of QML when the filters become durable product contracts.

   Current examples:

   - Contact company filter options are hard-coded in QML: `["All Companies", "KDAB", "TechSoft", "Vision Systems"]` (`qml/pages/ContactsPage.qml:79-84`).
   - Contact channel and sort option strings are hard-coded in QML while C++ interprets those strings (`qml/pages/ContactsPage.qml:102-129`, `src/directory/ContactDirectoryController.cpp:125-164`).
   - Company sort modes are hard-coded in QML while C++ interprets those strings (`qml/pages/CompaniesPage.qml:56-61`, `src/directory/CompanyDirectoryController.cpp:102-120`).
   - CV sort/language modes are hard-coded in QML while C++ interprets those strings (`qml/pages/CvLibraryBrowserPane.qml:107-118`, `qml/pages/CvLibraryBrowserPane.qml:366-371`, `src/cvs/CvLibraryController.cpp:172-207`).
   - Job status filter options are hard-coded in QML while C++ interprets those strings (`qml/pages/JobsApplicationsPane.qml:25`, `src/jobs/JobApplicationsController.cpp:105-122`).

   This is acceptable for a prototype, but it is a contract smell once user data or localization is introduced.

4. Split the largest QML panes before adding more behavior.

   `JobDescriptionPane.qml` and `JobsApplicationsPane.qml` are still large enough to slow safe edits. Splitting them is not cosmetic; it reduces risk for future add/edit, validation, and detail-view work.

### Optional Later

- Dashboard global search is visual only (`qml/pages/DashboardPage.qml:45-56`). Move it to backend/app state only when global search is actually implemented.
- Jobs pagination is static visual data (`qml/pages/JobsApplicationsPane.qml:351-357`). Make it backend-owned only when pagination becomes functional.
- The `CvLibraryBrowserPane.qml` "Used in Jobs" filter is visual-only with `options: ["Any"]` and no outgoing signal (`qml/pages/CvLibraryBrowserPane.qml:121-126`). Add a controller contract when this filter becomes real.
- `Sidebar.qml` has an `Add CV` quick action with no handler (`qml/components/Sidebar.qml:108-120`). Wire it only when the destination workflow exists.
- Placeholder pages contain inline display data: `MemoryPage.qml:39-45`, `ProfilePage.qml:27-32`, `SettingsPage.qml:27-32`. Keep them as placeholder UI until those pages become product features.

## Refactoring Recommendations

### Needed Now

1. Extract shared form primitives.

   `JobFormPage.qml` and `JobDescriptionPane.qml` both define local `FieldLabel`, `FormField`, `FormArea`, and `TagChip` components (`qml/pages/JobFormPage.qml:18-62`, `qml/pages/JobDescriptionPane.qml:25-65`). A small shared form component set under `qml/components/` would reduce duplication and keep future validation/error styling consistent.

2. Split `JobDescriptionPane.qml`.

   Suggested boundaries:

   - `JobDescriptionHeaderTabs` for the top list/detail tabs.
   - `JobDetailsEditPanel` for the left form-like detail area.
   - `JobDetailsPreviewPanel` for the right preview.
   - `JobDetailsRelatedActionsPanel` or smaller local cards for requirements/tech stack/contacts.

   Keep the `selectedApplication` map as the input for the first split. Do not introduce new C++ behavior in the same pass unless the user explicitly asks for it.

3. Split `JobsApplicationsPane.qml`.

   Suggested boundaries:

   - `JobsFilterBar`
   - `JobsApplicationsTable`
   - `JobsPaginationBar`
   - `JobApplicationPreviewPanel`

   Keep `JobsPage.qml` as the screen coordinator that wires controller state/signals to these panes.

4. Split `ContactsPage.qml` before expanding contact workflows.

   Suggested boundaries:

   - `ContactsFilterBar`
   - `ContactsTable`
   - `ContactPreviewPanel`
   - `ContactRow`
   - `ContactLine`
   - `InteractionRow`

   The contact page has backend contracts already, so this can be a QML-only decomposition pass if no new behavior is added.

### Optional Later

- Extract `FilterCombo` from `CvLibraryBrowserPane.qml:159-208` if the same control is reused for Companies and Contacts.
- Extract `CompanyRow`, `LinkedJobRow`, and `LinkedContactRow` from `CompaniesPage.qml:281-443` when company workflows grow.
- Extract `IconButton` and `PdfIcon` variants shared by CV browser/preview panes (`qml/pages/CvLibraryBrowserPane.qml:210-225`, `qml/pages/CvLibraryBrowserPane.qml:552-588`, `qml/pages/CvLibraryPreviewPanel.qml:248-300`).
- Consider a small table/header component only after two or more table screens need synchronized behavior, not just matching visuals.

### Do Not Refactor Now

- Do not globally redesign the visual system.
- Do not move navigation, hover, selected-row styling, custom scrollbar visuals, or fixed layout metadata into C++.
- Do not split already-small components such as `Panel.qml`, `StatusChip.qml`, `PrimaryButton.qml`, `SidebarButton.qml`, or `CvSearchField.qml`.
- Do not turn every page-local display array into a backend model unless it becomes durable behavior, user-configurable state, localization data, or a tested contract.

## Files That Should Stay Mostly As They Are

- `qml/Main.qml`: small and focused, except for the routing gap around Settings/Profile/Memory and JobForm stack ownership.
- `qml/pages/DashboardPage.qml`: acceptable composition shell with presentation-only scrollbar logic.
- `qml/pages/DashboardStatsRow.qml`, `DashboardActivityPanels.qml`, `DashboardRecentCvsPanel.qml`: already backend-model-driven after the prior cleanup pass.
- `qml/pages/CvLibraryPage.qml`: good screen coordinator over browser and preview panes.
- `qml/pages/CvSearchField.qml`: focused reusable control.
- `qml/components/Panel.qml`, `StatusChip.qml`, `PrimaryButton.qml`, `SidebarButton.qml`: focused shared components.
- `CMakeLists.txt`: QML registration is explicit and currently complete.

## Suggested Next Passes

1. QML navigation cleanup pass.

   Decide whether `SettingsPage`, `ProfilePage`, and `MemoryPage` should be reachable now. If yes, make their routing explicit and stop using `StackLayout` index 5 as both sidebar destination and job-form surface. If not, remove or hide the unreachable navigation affordance.

2. Shared form controls pass.

   Extract `FieldLabel`, `FormField`, `FormArea`, and `TagChip` into focused components and update `JobFormPage.qml` and `JobDescriptionPane.qml`. This is a contained QML-only pass with visual regression risk.

3. Jobs pane decomposition pass.

   Split `JobsApplicationsPane.qml` and `JobDescriptionPane.qml` without changing backend contracts.

4. Model-driven option lists pass.

   Add controller-provided option lists for statuses, company/contact filters, channels, sort modes, CV languages, and CV sort modes. This should include C++ tests because QML would start depending on backend option contracts.

5. Add/Edit job backend contract pass.

   Add create/update draft handling, validation feedback, and save error reporting before the Add Job UI is treated as complete.

## Verification Performed

This was a research-only task. No build was run because no production source or build configuration was changed.

Read-only verification performed:

- Listed all QML files with `rg --files qml`.
- Counted current QML file lengths.
- Compared `rg --files qml` against the `qt_add_qml_module` file list in `CMakeLists.txt`; no mismatches were reported.
- Searched QML for JavaScript functions, inline arrays, model usage, repeated local components, and backend controller references.
- Inspected the QML-facing C++ controller headers and controller implementations relevant to search/filter/sort/selection contracts.
- Cross-checked lead findings against the requested `qml_researcher` subagent output.

## Manual Checks For Future Changes

After future QML edits, manually check:

- App starts.
- Sidebar navigation works.
- Settings/Profile/Memory routing matches the intended product decision.
- Add Job opens, cancel closes, and save behavior matches the implemented backend contract.
- Dashboard renders stats, funnel, recent applications, and recent CVs.
- Jobs list/detail toggle works and selected application data updates.
- CV Library search, category/language filters, sort, selection, favorite toggle, and open action still behave as intended.
- Companies search/sort/selection and linked jobs/contacts still render.
- Contacts search/company/channel/sort filters, selection, and interaction history still render.
- Minimum window size `1180x720` has no obvious overlap or clipped controls.

## Risks And Limitations

- The report is based on static source inspection, not a live UI run.
- Some glyphs/icons render as Unicode symbols in QML; the PowerShell console may display mojibake, so visual correctness should be checked in the running app rather than inferred from terminal output.
- The recommendation to move option lists to C++ is about durable product contracts, not an instruction to move presentation labels immediately.
- Placeholder pages and placeholder buttons may be intentional prototype surfaces; treat them as gaps only when the product expects those workflows to work.
