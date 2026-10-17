# JobTracker

JobTracker is a desktop job-search tracking application built with C++20, Qt 6, QML, CMake, and SQLite.

The application helps track job applications, linked CV documents, dashboard summaries, companies, and contacts from a Qt/QML desktop interface backed by C++ models, controllers, services, and repositories.

## Current Status

JobTracker is under active development.

The current production workflow supports:

- Viewing a dashboard with job and CV summary data.
- Browsing and filtering job applications.
- Adding job applications through the QML Add Job form.
- Importing local PDF, DOC, and DOCX CV files during Add Job.
- Persisting jobs, CV metadata, and job technology rows in SQLite.
- Browsing the CV library and linked application data.
- Rendering company and contact directory pages over current in-memory directory models.

Company and contact storage is not implemented yet. Their pages and controller contracts exist, but the production bootstrap currently starts them with empty models.

## Tech Stack

- C++20
- Qt 6.10.3 (required exactly on Windows and Linux)
- QML / Qt Quick
- Qt SQL
- SQLite
- CMake 3.21 or newer
- Qt Test (test-enabled builds only)

The main executable target is `JobTrackerApp`.

## Project Layout

```text
.
+-- CMakeLists.txt
+-- CMakePresets.json
+-- CMakeUserPresets.json
+-- src
|   +-- app          # Application startup and QML engine bootstrap
|   +-- common       # Shared filtering, sorting, search, and validation helpers
|   +-- cvs          # CV domain data, models, controller, repository, and import service
|   +-- dashboard    # Dashboard metrics and recent-item models
|   +-- directory    # Company/contact domain data, models, and controllers
|   +-- jobs         # Job domain data, model, controller, repository, and Add Job service
|   +-- storage      # Storage paths, SQLite connection, and schema migration
|   +-- utils        # Utility code
|   +-- main.cpp     # Minimal application entry point
+-- qml
|   +-- Main.qml
|   +-- components   # Shared QML components
|   +-- pages        # Application pages and page-level panes
+-- tests            # Qt Test suites
```

## Architecture

`src/main.cpp` delegates startup to `App::start(argc, argv)`.

`App::start()` creates the Qt application object, constructs the bootstrap layer, handles startup failures, and enters the Qt event loop.

`AppBootstrap` owns the process-level application graph. It creates the QML engine, constructs storage, repositories, services, models, and controllers, exposes the QML-facing controllers as context properties, and loads:

```text
qrc:/JobTracker/qml/Main.qml
```

The main QML shell is `qml/Main.qml`. It owns the application window, sidebar navigation, Add Job form visibility, and page switching.

Currently reachable pages are:

- Dashboard
- Jobs
- CV Library
- Companies
- Contacts
- Add/Edit Job form

The QML module URI is `JobTracker`.

## Persistence

Jobs and CVs are stored in SQLite.

The current schema stores:

- CV metadata
- Jobs
- Ordered job technology rows

CV import is handled by the C++ backend. Imported CV files are hashed, deduplicated by SHA-256, copied into managed application storage, and linked to jobs when selected during Add Job.

## Build And Windows Release

Start with [Building JobTracker](docs/BUILDING.md). The
[canonical command catalog](For-Agent/Docs/build.md) covers Windows Debug,
separate test builds, Release staging, offline Setup and Linux Debug/Release
production/test presets. Both platforms require exactly Qt 6.10.3; shared presets use `QTDIR`,
and local presets can override `CMAKE_PREFIX_PATH`. Normal builds compile/link;
development runtime preparation and Release deployment are explicit steps.

[Windows release preparation](docs/WINDOWS-RELEASE.md) describes direct staging
and one native x64 `Installer/output/JobTracker-Setup-<version>.exe`.
[Windows installer behavior](docs/WINDOWS-INSTALLER.md) explains CurrentUser
App-Local runtime, the bundled AllUsers prerequisite, same-scope manual upgrades
and data preservation. No GitHub archive or installation-time download is needed.
Switching scope requires manual uninstall with data preserved, then reinstall.
Linux builds/tests remain available but were not executed on this Windows host;
Linux packaging is out of scope.

**Publication HOLD:** current candidates are unsigned and unapproved. The
[current license issues](For-Agent/Task-Report/offline-installer/LICENSE_ISSUES.md)
preserve the [redistribution hold](packaging/windows/REDISTRIBUTION.md), including
Lottie licensing, notices/source/replacement material, external-binary provenance
and owner entitlement. Embedded QML resources are explicitly allowed by the
offline requirements. Passing local tests
does not clear these findings or establish GUI/clean-machine acceptance.
Use the [offline status index](Tasks/Offline-Installer/STATUS.md)
for dated evidence and remaining work. Nothing is uploaded automatically.

## Tests

Tests are enabled with the `JOBTRACKER_BUILD_TESTS` CMake option through the test presets.

Use the [Windows test commands](For-Agent/Docs/build.md#windows-build-with-tests)
and [testing policy](For-Agent/Docs/testing.md). Production builds do not depend
on Qt Test, test sources or deployment probes. Standalone installer suites and
the [acceptance checklist](tests/deployment/Windows-Release-Checklist.md) provide
additional evidence without launching JobTracker against personal data.

Current test areas include:

- Common filtering and validation helpers
- Job application controller behavior
- CV library controller behavior
- Dashboard controller behavior
- Directory controller behavior
- Storage and Add Job persistence behavior

## Development Notes

Keep QML focused on presentation, navigation, layout, bindings, and simple UI state.

Keep durable business logic in C++ backend classes, including validation, persistence, parsing, filtering, sorting, search, grouping, and cross-screen application state.

Important conventions:

- Use `.hpp` for C++ headers.
- Use classic `#ifndef` / `#define` include guards.
- Keep `src/main.cpp` minimal.
- Preserve Windows and Linux portability.
- Prefer small, explicit, maintainable changes.

## Known Gaps

Current known limitations include:

- Company and contact storage and mutation workflows are not implemented yet.
- Company linkage is incomplete at the persistence boundary.
- CV favorite changes are currently in-memory only.
- Stored CV opening is not fully implemented.
- Add Job and CV import run synchronously from the UI invocation.
- Some selection behavior is still index-based across filtering and sorting.
- Some QML option lists still use display strings that C++ interprets.

See `For-Agent/Docs/architecture/overview.md` for the maintained architecture
index and links to the focused domain notes.
