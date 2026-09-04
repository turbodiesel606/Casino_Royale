---
name: qml-to-cpp-extraction
description: Identify and migrate JobTracker QML business logic into C++ backend classes, models, controllers, services, or storage while preserving UI behavior. Use when Codex needs to extract JavaScript functions, mock data, durable state, validation, filtering, sorting, parsing, storage, or QML-owned product logic into C++.
---

# QML To C++ Extraction

Use this skill when moving durable behavior from QML into the JobTracker C++ backend.

## Workflow

1. Read `For-Agent/Docs/qml-to-cpp-extraction.md`, `For-Agent/Docs/architecture.md`, `For-Agent/Docs/qml-style.md`, `For-Agent/Docs/coding-style.md`, and `For-Agent/Docs/testing.md`.
2. Start from the QML owner of the behavior, then inspect connected pages, components, and C++ objects exposed to QML.
3. Classify current QML logic as UI-only state or durable product behavior.
4. Define the QML/C++ contract before editing:
   - readable properties;
   - invokable commands or slots;
   - notify and result signals;
   - model roles for delegates.
   Define this contract from product responsibilities. Do not expose internals, add commands or signals, or widen production APIs solely for tests.
5. Move durable behavior into focused C++ backend classes:
   - models for repeated delegate data;
   - controllers or view models for QML-facing state and commands;
   - services for validation, parsing, filtering, sorting, storage coordination, or algorithms.
6. Keep QML as bindings, layout, navigation, presentation, and simple UI state.
7. Keep `src/main.cpp` limited to bootstrap, QML engine setup, registration, dependency wiring, and startup logic.
8. Update CMake file lists when adding C++ or QML files.
9. Add or update tests for changed business logic, storage, parsing, algorithms, model roles, or signal behavior. Keep test support under `tests/` and make tests follow the production contract without reshaping it.

## Verification

Run the normal build after C++ or QML extraction unless impossible.

Run the test preset and `ctest` when the extraction affects logic, storage, parsing, algorithms, models, or high-risk behavior.

List manual UI checks when visual behavior must be confirmed by the user.
