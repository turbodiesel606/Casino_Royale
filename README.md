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
- Qt 6.5 or newer
- QML / Qt Quick
- Qt SQL
- SQLite
- CMake 3.21 or newer
- Qt Test

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

## Build

Run all project commands from the repository root:

```powershell
cd D:\Project_CV\Root
```

Configure the normal Windows debug build:

```powershell
cmake --preset windows-debug-local
```

Build:

```powershell
cmake --build --preset windows-debug-local
```

The local Windows presets expect Qt to be available at:

```text
D:/Qt_install/6.9.1/msvc2022_64
```

Shared non-local presets also exist in `CMakePresets.json`, including Windows and Linux debug presets.

## Tests

Tests are enabled with the `JOBTRACKER_BUILD_TESTS` CMake option through the test presets.

Configure the Windows debug test build:

```powershell
cmake --preset windows-debug-tests-local
```

Build the tests:

```powershell
cmake --build --preset windows-debug-tests-local
```

Run the tests:

```powershell
ctest --preset windows-debug-tests-local
```

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
