---
name: cpp-backend-task
description: Work on JobTracker C++ backend tasks. Use when Codex needs to add or modify C++ models, services, controllers, storage, parsing, algorithms, application state, QObject integrations, QML-exposed objects, or startup wiring outside simple QML-only UI changes.
---

# C++ Backend Task

Use this skill for C++ backend work in JobTracker.

## Required Context

Read these files before implementation:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/coding-style.md`
4. `For-Agent/Docs/testing.md`
5. `For-Agent/Docs/build.md`

Also read `For-Agent/Docs/qml-style.md` and `For-Agent/Docs/qml-to-cpp-extraction.md` when the work changes a QML-facing contract or moves behavior out of QML.
Use qt-qml-ui-task when implementation also changes QML files.

## C++ Implementation Rules

1. Consolidate C++ functions, classes, and methods that duplicate the same logic, responsibility, and invariants into one canonical function, class, or method. Use overloads, inheritance (without virtuals), templates, and template specializations when they are necessary and technically appropriate to expose the canonical implementation without duplicating it.
2. Use existing C++ classes, functions, and methods when solving a problem. Reuse a fully suitable entity directly. If an entity is only partially suitable, extend it through an overload, override, template, or specialization when appropriate. Create a new entity only when the existing entities are not suitable for the responsibility.
3. Follow established modern C++ best practices for correctness, type safety, ownership, lifetime and resource management, exception safety, clarity, maintainability, portability, and testability. Apply the concrete conventions in `AGENTS.md`, `For-Agent/Docs/architecture.md`, and `For-Agent/Docs/coding-style.md`.
4. Keep the test dependency one-way. Do not add or widen production APIs, expose internals, weaken access control, add test-only hooks or compile definitions, or alter production ownership, lifetime, threading, behavior, or architecture solely for tests. Keep fixtures, mocks, fakes, and helpers under `tests/`.

## Workflow

1. Inspect `git status` and existing diffs for files in scope. Identify and preserve pre-existing changes.
2. Start from the smallest affected model, controller, service, repository, storage class, utility, or startup surface. Use `cpp-code-research` first when ownership, data flow, or implementation direction is unclear.
3. Define ownership, lifetime, public and QML-facing contracts, error behavior, thread boundaries, and storage or transaction boundaries from production requirements, then identify the tests required for those contracts.
4. Make the smallest sufficient change. Follow the architecture and C++ style guidance, preserve unrelated work, and do not change comments for logic that did not change.
5. Update `CMakeLists.txt` source and test registration when C++ files or test targets are added, removed, or renamed.
6. Add or update tests for changed business logic, storage, parsing, algorithms, Qt model behavior, signals, or other high-risk contracts.
7. Review `For-Agent/Docs/` and update only the guidance affected by the implementation. State when no documentation update is required.
8. Use `cmake-build-debug` for applicable build and test verification, then use `test-and-review` for the final scope, architecture, QML contract, documentation, verification, and risk pass.

Treat `For-Agent/Docs/build.md` as the source of truth for build and test commands. Do not substitute undocumented commands.

## Boundaries

Keep `src/main.cpp` minimal and place non-trivial construction or QML wiring in focused application or bootstrap classes.

Keep QML-facing controllers and models small. Delegate durable behavior to services, repositories, and storage classes according to `For-Agent/Docs/architecture.md`.

Do not broaden a backend task into unrelated refactoring, visual changes, or cleanup.
