# Architecture

JobTracker is a Qt 6 / QML / C++ desktop application.

## Entry Points

- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.
- Executable target: `JobTrackerApp`.
- QML module URI: `JobTracker`.

## Current Shape

`src/main.cpp` creates the application, initializes `QQmlApplicationEngine`, connects object creation failure handling, and loads `qrc:/JobTracker/qml/Main.qml`.

`qml/Main.qml` owns the main `ApplicationWindow`, sidebar shell, and `StackLayout` navigation.

QML pages live under `qml/pages`.

Shared QML components live under `qml/components`.

Keep concrete QML file registration in `CMakeLists.txt` as the source of truth for the current file inventory.

## Boundaries

Keep QML focused on presentation, layout, navigation, binding, and simple UI state.

Place durable business logic, storage, parsing, algorithms, and application state in C++ backend classes when they are introduced.

Use `docs/qml-to-cpp-extraction.md` when moving durable behavior from QML into C++.

Keep `src/main.cpp` minimal. Move non-trivial startup wiring into dedicated bootstrap/application classes when the startup surface grows.

Avoid global visual rewrites unless the task explicitly asks for a broader design change.

Avoid large monolithic pages. Extract reusable QML components when repetition becomes meaningful.

## Backend Shape

Organize backend code by responsibility:

- Domain and value types for durable product data.
- Models for list or table data consumed by QML delegates.
- Controllers or view models for QML-facing properties, commands, and signals.
- Services for validation, parsing, algorithms, and business operations.
- Storage and configuration classes for persistence and platform-aware file access.
- Bootstrap/application classes for startup wiring that no longer belongs in `src/main.cpp`.

QML-facing controllers should expose a small screen contract and delegate non-trivial behavior to services or models.
