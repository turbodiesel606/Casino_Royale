---
name: cpp-backend-task
description: Work on JobTracker C++ backend tasks. Use when Codex needs to add or modify C++ models, services, controllers, storage, parsing, algorithms, application state, QObject integrations, QML-exposed objects, or startup wiring outside simple QML-only UI changes.
---

# C++ Backend Task

Use this skill for C++ backend work in JobTracker.

## Workflow

1. Read `For-Agent/Docs/architecture.md`, `For-Agent/Docs/coding-style.md`, and `For-Agent/Docs/testing.md`.
2. Locate the affected model, service, controller, storage, utility, or startup code.
3. Keep `src/main.cpp` minimal; move non-trivial startup logic into focused classes when needed.
4. Prefer required non-owning references for non-QObject parameters.
5. Use pointers when Qt ownership, QObject parent-child hierarchy, nullable dependencies, polymorphism, or signal/slot integration makes pointer semantics more correct.
6. Use trailing underscores for user-defined class and struct fields.
7. Keep changes scoped to the requested behavior.
8. Add or update tests when changing business logic, storage, parsing, algorithms, or high-risk behavior.

## QML Integration

When backend work changes behavior currently owned by QML, read `For-Agent/Docs/qml-style.md` and `For-Agent/Docs/qml-to-cpp-extraction.md`.

Inspect the QML owner before designing the C++ API.

Define the QML/C++ contract before implementation:

- properties QML binds to;
- commands QML invokes;
- signals QML handles;
- model roles QML delegates consume.

Prefer focused QObject controllers or Qt models for the QML-facing API, and delegate durable behavior to services or domain classes.

## Verification

Run the normal build after C++ changes unless impossible.

Run the test preset and `ctest` when the change affects logic, storage, parsing, algorithms, or high-risk behavior.

If tests are unavailable, state the manual checks needed.
