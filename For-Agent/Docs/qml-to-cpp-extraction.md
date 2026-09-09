# QML To C++ Extraction

Use this guide when moving durable behavior out of QML and into the JobTracker C++ backend.

## Goal

QML should describe presentation, layout, navigation, bindings, and simple view state.

C++ should own durable product behavior:

- Application state that must survive across screens or sessions.
- Validation rules and computed domain state.
- Filtering, sorting, grouping, and search rules that affect product behavior.
- Parsing, storage, import, export, and file-system access.
- Data models consumed by more than one QML view.
- Operations that require durable ownership outside one view or stable cross-platform behavior.

## What Can Stay In QML

Keep logic in QML when it is only presentation behavior:

- Selected tab, current page, expanded row, hover, focus, and transient popup state.
- Local visual formatting that does not change domain meaning.
- Layout decisions, animations, and component composition.
- Small display-only adapters for already prepared backend values.

Mock data can stay in QML only for short-lived prototypes. Move it behind C++ models before treating the screen as product behavior.

## Extraction Workflow

1. Inspect the QML owner first, then related pages and components.
2. Classify each JavaScript function, inline array, property binding, and state mutation as UI-only or durable behavior.
3. Define the QML/C++ contract before moving code:
   - values QML reads;
   - commands QML can invoke;
   - signals QML reacts to;
   - model roles QML delegates consume.
4. Move durable rules into focused C++ classes:
   - model classes for lists and table-like data;
   - controller or view-model classes for QML-facing commands and state;
   - services for domain operations that should not know about QML;
   - storage classes for persistence and file access.
5. Expose only the smallest useful API to QML.
6. Replace QML business logic with bindings, signal handlers, and calls into the exposed C++ API.
7. Add or update tests for validation, parsing, algorithms, storage, model roles, and signal behavior.
8. Run the applicable build and tests from `D:\Project_CV\Root`.

Define the extraction boundary and production API from product responsibilities. Do not move behavior, expose internals, add commands or signals, or widen a QML/C++ contract solely to accommodate tests; tests must exercise the production contract that the feature requires.

## C++ Backend Shape

Prefer this separation:

- Domain/value types: plain C++ or Qt value types with clear ownership.
- Services: non-QML business operations, validation, parsing, persistence coordination, and algorithms.
- QML-facing controllers: `QObject` classes exposing properties, invokable commands, and signals.
- Models: `QAbstractListModel` or related Qt model types for lists consumed by QML delegates.
- Bootstrap/application wiring: construction and dependency wiring outside `src/main.cpp` when startup grows.

Avoid placing storage, parsing, validation, or cross-screen state directly in QML-facing controllers when a service or model would keep the boundary cleaner.

When implementing the extracted C++ behavior, apply the canonical reuse,
generalization, ownership, and quality rules in `AGENTS.md` and
`For-Agent/Docs/coding-style.md`. Inspect the relevant existing backend entities
before adding a new owner, and generalize only when responsibility and invariants
genuinely match.

When extraction creates or splits C++ headers, use the `.hpp` extension and `#ifndef` / `#define` include guards instead of `#pragma once`.

When extraction adds or changes object construction, use curly braces for constructors.

## QML Exposure Rules

Use `Q_PROPERTY` for state that QML reads or binds to.

Use `Q_INVOKABLE` or slots for user actions that QML triggers.

Emit notify signals whenever exposed state changes.

Use `QAbstractListModel` roles for repeated delegate data instead of exposing parallel arrays.

Prefer explicit roles with stable names. Keep role names aligned with QML delegate property names.

Use pointers when Qt ownership, QObject parent-child hierarchy, nullability, polymorphism, signal/slot integration, or model/view APIs make pointer semantics correct.

Prefer required references for non-owning, non-QObject dependencies.

Avoid exposing broad service objects directly to QML. Expose a small controller or model API that matches the screen contract.

## Startup Wiring

Keep `src/main.cpp` limited to application bootstrap, QML engine setup, type registration, dependency wiring, and startup logic.

When wiring grows beyond simple setup, move object construction into a focused bootstrap or application class and keep QML registration explicit.

## Testing Expectations

Add or update tests when extraction moves:

- Validation rules.
- Filtering, sorting, grouping, search, or computed state.
- Storage, parsing, import, export, or file-system behavior.
- Model role names, row counts, data values, or reset/update behavior.
- Signals or property notifications that QML depends on.

Use Qt Test for QObject, signal, and model behavior. Use `QSignalSpy` when verifying emitted signals.

Keep test fixtures, mocks, fakes, and helpers under `tests/`. Production C++ and QML-facing APIs must not reference or depend on them.

## Manual Checks

After extraction, manually check the affected screens when visual automation is unavailable:

- App starts and the target screen opens.
- Existing navigation still works.
- Data shown before extraction still appears.
- User actions still update the UI.
- Empty, error, and loading states still render correctly when applicable.
- Text still fits and no controls overlap at supported window sizes.
