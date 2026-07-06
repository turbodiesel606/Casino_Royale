# Pass 1. Inventory And Contracts

Status: completed for the current QML codebase.

Scope: inventory and contract definition only. No QML behavior, C++ backend code, CMake setup, bootstrap code, tests, or visual styling was changed in this pass.

## Files Inspected

- `qml/Main.qml`
- `qml/components/Sidebar.qml`
- `qml/components/Panel.qml`
- `qml/components/StatusChip.qml`
- `qml/pages/DashboardPage.qml`
- `qml/pages/DashboardStatsRow.qml`
- `qml/pages/DashboardActivityPanels.qml`
- `qml/pages/DashboardRecentCvsPanel.qml`
- `qml/pages/JobsPage.qml`
- `qml/pages/JobsApplicationsPane.qml`
- `qml/pages/JobDescriptionPane.qml`
- `qml/pages/JobFormPage.qml`
- `qml/pages/CvLibraryPage.qml`
- `qml/pages/CvSearchField.qml`
- `qml/pages/CvLibraryBrowserPane.qml`
- `qml/pages/CvLibraryPreviewPanel.qml`
- `qml/pages/CompaniesPage.qml`
- `qml/pages/ContactsPage.qml`
- `src/main.cpp`
- `CMakeLists.txt`

Current backend exposure: none. `src/main.cpp` only creates `QGuiApplication`, creates `QQmlApplicationEngine`, connects `QQmlApplicationEngine::objectCreationFailed`, and loads `qrc:/JobTracker/qml/Main.qml`.

## QML Logic That Should Stay In QML

- Main navigation state in `qml/Main.qml`: `currentPage`, `previousPage`, `jobFormVisible`, `openJobForm()`, and `closeJobForm()`.
- Sidebar hover, selected item styling, and visual navigation delegates in `qml/components/Sidebar.qml`.
- Page-local visual selection when it is only a highlighted row or card.
- Dashboard custom scrollbar geometry and drag math in `qml/pages/DashboardPage.qml`; this is presentation behavior tied to the Flickable.
- Layout constants, colors, spacing, panel sizes, component composition, and local visual components such as `FieldLabel`, `FormField`, `FormArea`, `TagChip`, `IconButton`, `PdfIcon`, and table/header text components.
- Display-only color mapping may temporarily remain in QML, but status/category color should eventually come from stable backend enum or display metadata when statuses and categories become real domain values.

## Migration Candidates

### Needed First

1. Job applications domain.
   - Current QML owners: `qml/pages/JobsPage.qml`, `qml/pages/JobsApplicationsPane.qml`, `qml/pages/JobDescriptionPane.qml`, `qml/pages/JobFormPage.qml`, and dashboard recent application rows.
   - Current QML-owned behavior/data: application records, selected application details, preview detail rows, status text, next-step text, CV used by each vacancy, notes, job description fields, requirements, tech stack, and hard-coded application counts.
   - Reason: this is the central product entity and already connects company, contact, CV, status, notes, and next-step concepts.

2. CV library and CV linkage.
   - Current QML owners: `qml/pages/CvLibraryPage.qml`, `qml/pages/CvLibraryBrowserPane.qml`, `qml/pages/CvLibraryPreviewPanel.qml`, `qml/pages/DashboardRecentCvsPanel.qml`, and job application preview/form screens.
   - Current QML-owned behavior/data: CV records, category/language metadata, last-modified text, file size, used-in-jobs counts, linked application cards, and duplicated category color mapping.
   - Reason: the app must preserve visibility of which CV was used for which vacancy.

3. Dashboard summary data.
   - Current QML owners: `qml/pages/DashboardStatsRow.qml`, `qml/pages/DashboardActivityPanels.qml`, and `qml/pages/DashboardRecentCvsPanel.qml`.
   - Current QML-owned behavior/data: totals, status percentages, recent applications, recent CVs, and status/category display rules.
   - Reason: dashboard values should be derived from the same application and CV backend state, not duplicated mock arrays.

### Needed After The First Domain

4. Companies.
   - Current QML owner: `qml/pages/CompaniesPage.qml`.
   - Current QML-owned behavior/data: company records, website, open-job counts, contact counts, last activity, descriptions, notes, linked jobs, and employees.

5. Contacts.
   - Current QML owner: `qml/pages/ContactsPage.qml`.
   - Current QML-owned behavior/data: contact records, related company/job, channel availability, notes, linked application, and interaction history.

6. Filtering, search, sorting, pagination, and validation.
   - Current QML placeholders: search fields, filter buttons, sort labels, table footer counts, page-size controls, category/language filters, and form fields.
   - Current state: mostly visual placeholders with little real filtering. When made functional, these rules should be implemented in C++ services or controller state, not QML JavaScript.

## First Backend Contract: Job Applications

Create this slice before migrating behavior in Pass 3.

### Domain Types

- `JobApplication`
  - `id`
  - `companyId`
  - `companyName`
  - `companyInitials`
  - `companyAccent`
  - `jobTitle`
  - `jobUrl`
  - `workFormat`
  - `city`
  - `salary`
  - `status`
  - `appliedDate`
  - `nextStep`
  - `cvId`
  - `cvFileName`
  - `description`
  - `requirements`
  - `techStack`
  - `notes`
  - `contactIds`

- `ApplicationStatus`
  - `Applied`
  - `Interview`
  - `TestTask`
  - `Offer`
  - `Rejected`
  - later statuses as required by real data.

### Model

`JobApplicationListModel` should derive from `QAbstractListModel`.

Required roles:

- `id`
- `companyId`
- `companyName`
- `companyInitials`
- `companyAccent`
- `jobTitle`
- `cvId`
- `cvFileName`
- `dateLabel`
- `appliedDate`
- `status`
- `statusLabel`
- `statusAccent`
- `nextStep`
- `workFormat`
- `salary`
- `description`
- `requirements`
- `techStack`
- `notes`

The existing QML table should stop consuming index-based arrays such as `modelData[4]` after migration. Delegates should bind to named roles.

### QML-Facing Controller

`JobApplicationsController` should derive from `QObject`.

Readable properties:

- `QAbstractItemModel *applicationsModel`
- `int selectedApplicationIndex`
- `QString selectedApplicationId`
- `QVariantMap selectedApplication`
- `QString searchText`
- `QString statusFilter`
- `QString resultSummary`

Commands:

- `selectApplication(int index)`
- `setSearchText(const QString &text)`
- `setStatusFilter(const QString &status)`
- `clearFilters()`
- `createDraft()`
- `updateDraftField(const QString &fieldName, const QVariant &value)`
- `saveDraft()`
- `discardDraft()`
- `markNextStep(const QString &applicationId, const QString &nextStep)`

Signals:

- `applicationsModelChanged()`
- `selectedApplicationChanged()`
- `filtersChanged()`
- `resultSummaryChanged()`
- `draftChanged()`
- `saveSucceeded(QString applicationId)`
- `saveFailed(QString message)`

Initial migration rule: keep the `JobsPage` list/detail view mode in QML, but move selected application data lookup into the controller. QML can keep a local boolean for showing the description pane.

## Second Backend Contract: CV Library And Linkage

Create this slice before Pass 4 migration.

### Domain Types

- `CvDocument`
  - `id`
  - `fileName`
  - `title`
  - `category`
  - `language`
  - `lastModified`
  - `fileSize`
  - `description`
  - `linkedApplicationIds`
  - `isFavorite`

- `CvCategory`
  - `id`
  - `name`
  - `accent`

### Model

`CvListModel` should derive from `QAbstractListModel`.

Required roles:

- `id`
- `fileName`
- `title`
- `category`
- `categoryAccent`
- `language`
- `languageAccent`
- `lastModifiedLabel`
- `fileSizeLabel`
- `description`
- `linkedApplicationCount`
- `isFavorite`

### QML-Facing Controller

`CvLibraryController` should derive from `QObject`.

Readable properties:

- `QAbstractItemModel *cvModel`
- `QAbstractItemModel *linkedApplicationsModel`
- `int selectedCvIndex`
- `QString selectedCvId`
- `QVariantMap selectedCv`
- `QString searchText`
- `QString categoryFilter`
- `QString languageFilter`
- `QString sortMode`
- `QString resultSummary`

Commands:

- `selectCv(int index)`
- `setSearchText(const QString &text)`
- `setCategoryFilter(const QString &category)`
- `setLanguageFilter(const QString &language)`
- `setSortMode(const QString &sortMode)`
- `clearFilters()`
- `openCv(const QString &cvId)`
- `toggleFavorite(const QString &cvId)`

Signals:

- `cvModelChanged()`
- `linkedApplicationsModelChanged()`
- `selectedCvChanged()`
- `filtersChanged()`
- `resultSummaryChanged()`
- `openCvRequested(QString filePath)`
- `operationFailed(QString message)`

Linkage rule: job applications should store a stable `cvId`; QML should display `cvFileName` or linked CV data through model roles, not by duplicating text in separate arrays.

## Third Backend Contract: Dashboard Read Models

Create this after the job applications and CV library models exist.

`DashboardController` should derive dashboard data from backend state instead of owning separate records.

Readable properties/models:

- `QAbstractItemModel *statsModel`
- `QAbstractItemModel *funnelModel`
- `QAbstractItemModel *recentApplicationsModel`
- `QAbstractItemModel *recentCvsModel`

Required stat/funnel roles:

- `title`
- `value`
- `note`
- `ratio`
- `accent`

Required recent application roles:

- `id`
- `jobTitle`
- `companyName`
- `statusLabel`
- `statusAccent`
- `appliedDateLabel`

Required recent CV roles:

- `id`
- `fileName`
- `category`
- `categoryAccent`
- `language`
- `languageAccent`
- `lastModifiedLabel`
- `linkedApplicationCountLabel`

## Later Contracts

### Companies

`CompanyDirectoryController` with `CompanyListModel`.

Required company roles:

- `id`
- `name`
- `website`
- `logoText`
- `logoAccent`
- `openJobCount`
- `contactCount`
- `lastActivityLabel`
- `description`
- `notes`

The selected company detail should expose linked jobs and contacts as models or stable role lists, not inline QML string arrays.

### Contacts

`ContactDirectoryController` with `ContactListModel`.

Required contact roles:

- `id`
- `displayName`
- `initials`
- `roleTitle`
- `companyId`
- `companyName`
- `relatedApplicationId`
- `relatedApplicationTitle`
- `email`
- `telegram`
- `linkedin`
- `lastContactLabel`
- `notes`

Interaction history should become a model with roles such as `type`, `title`, `timestampLabel`, and `notes`.

## Validation And Service Candidates

Add services only when the first real behavior needs them. The first likely services are:

- `JobApplicationValidationService`: required title/company/status/CV rules, URL validation, and date validation.
- `JobApplicationFilterService`: search, status filter, sort, and pagination rules.
- `CvLibraryFilterService`: search, category/language filters, sort by last modified, and linked-job count filters.
- `RelationshipService`: application-to-CV, company-to-application, contact-to-company, and contact-to-application relationships.

These services should be testable without QML.

## Pass 2 And Pass 3 Starting Point

- Pass 2 should focus only on bootstrap exception safety and application wiring if the user approves it.
- Pass 3 should migrate job applications first, using `JobApplication`, `JobApplicationListModel`, and `JobApplicationsController`.
- Do not migrate CV library, dashboard, companies, contacts, search/filter/sort, or validation before the user explicitly asks for later passes.

## Verification Notes

This pass changed documentation only. Per `For-Agent/Docs/build.md`, no C++ build or CTest run is required for documentation-only changes that do not affect build behavior.

Manual check for the next pass: confirm that the first implementation slice should be job applications before creating C++ files.
