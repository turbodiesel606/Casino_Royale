# Architecture Overview

JobTracker is a Qt 6 / QML / C++ desktop application.

This file is a compact architecture index. Do not load every architecture
document for a bounded task. Start with the affected subsystem below and open an
additional document only when the inspected code or diff exposes a dependency,
contract, boundary, or risk covered there.

## System Map

The primary durable-command dependency flow is:

`QML -> controllers/models -> worker facades/executors -> services -> repositories -> storage`

Application/bootstrap code constructs, hydrates, and connects that graph.
Worker facades move job saves, standalone CV imports, and removals off the GUI
thread and return value outcomes for GUI-thread publication. Not every path uses
every layer: bootstrap reads repositories and runs managed-file recovery
directly, while `CvLibraryController` currently persists favorite changes
through `CvRepository` on the GUI thread. Tests consume production contracts
from outside the production dependency chain.

## Architecture Documents

| Task evidence | Read |
| --- | --- |
| Process startup, bootstrap construction, context properties, module inventory, or page reachability | [application.md](application.md) |
| Layer placement, dependency direction, ownership, lifetime, error translation, or production/test isolation | [boundaries.md](boundaries.md) |
| Add Job, edit/save/discard, job validation, job-save queueing, or job model publication | [jobs.md](jobs.md) |
| Add CV, CV identity/reuse/restore, import publication, or CV import cancellation | [cvs.md](cvs.md) |
| SQLite schema, migrations, repositories, managed files, transactions, or recovery | [storage.md](storage.md) |
| Worker runtime, thread affinity, queued delivery, shared mutation serialization, cancellation, or shutdown | [threading.md](threading.md) |
| Job deletion, CV archive/restore/permanent deletion, bulk removal, or linked-CV safety | [removal.md](removal.md) |
| QML/backend contracts, Qt model roles and notifications, filtering/sorting projections, or selection tracking | [qml-contracts.md](qml-contracts.md) |

For a task that clearly belongs to one row, go directly to that document. Read
this index first only when the subsystem is unclear or the task explicitly needs
a system-wide map. Cross-domain work should load the smallest combination of rows
that matches the actual dependency path; it should not trigger a complete
architecture read.

## Entry Points

- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.
- Executable target: `JobTrackerApp`.
- QML module URI: `JobTracker`.

Concrete C++ and QML file registration in `CMakeLists.txt` remains the source of
truth for the build inventory. Live production source, schema code, and tests
remain the source of truth when a maintained architecture note conflicts with
the implementation.
