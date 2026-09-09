---
name: cpp-backend-task
description: Work on JobTracker C++ backend tasks. Use when Codex needs to add or modify C++ models, services, controllers, storage, parsing, algorithms, application state, QObject integrations, QML-exposed objects, or startup wiring outside simple QML-only UI changes.
---

# C++ Backend Task

Use this skill for C++ backend work in JobTracker.

## Context Selection

1. Apply `AGENTS.md`, then identify the affected backend subsystem from the
   request, target code, and focused diff.
2. Read only the matching domain documents under
   `For-Agent/Docs/architecture/`. Use
   `For-Agent/Docs/architecture/overview.md` only when the correct domain document
   is unclear.
3. Read `For-Agent/Docs/coding-style.md` for C++ implementation conventions.
4. Read `For-Agent/Docs/testing.md` when the change affects business logic,
   storage, parsing, algorithms, Qt model behavior, signals, test registration,
   or another high-risk contract.
5. Read `For-Agent/Docs/build.md` only when CMake/build behavior is in scope or
   when build/test commands are needed for verification.

Read `For-Agent/Docs/qml-style.md` and
`For-Agent/Docs/qml-to-cpp-extraction.md` only when the work changes a
QML-facing contract or moves behavior out of QML. Expand to another document only
after the inspected implementation exposes the corresponding dependency or
boundary. Do not preload the complete architecture, build, and testing bundle.
Use qt-qml-ui-task when implementation also changes QML files.

## C++ Implementation Rules

1. Consolidate C++ functions, classes, and methods that duplicate the same logic, responsibility, and invariants into one canonical function, class, or method. Use overloads, inheritance (without virtuals), templates, and template specializations when they are necessary and technically appropriate to expose the canonical implementation without duplicating it.
2. Use existing C++ classes, functions, and methods when solving a problem. Reuse a fully suitable entity directly. If an entity is only partially suitable, extend it through an overload, override, template, or specialization when appropriate. Create a new entity only when the existing entities are not suitable for the responsibility.
3. Follow established modern C++ best practices for correctness, type safety, ownership, lifetime and resource management, exception safety, clarity, maintainability, portability, and testability. Apply the concrete conventions in `AGENTS.md`, the selected architecture domain documents, and `For-Agent/Docs/coding-style.md`.
4. Keep the test dependency one-way. Do not add or widen production APIs, expose internals, weaken access control, add test-only hooks or compile definitions, or alter production ownership, lifetime, threading, behavior, or architecture solely for tests. Keep fixtures, mocks, fakes, and helpers under `tests/`.

## Workflow

1. Inspect `git status` and existing diffs for files in scope. Identify and preserve pre-existing changes.
2. Start from the smallest affected model, controller, service, repository, storage class, utility, or startup surface. Use `cpp-code-research` first when ownership, data flow, or implementation direction is unclear.
3. Define ownership, lifetime, public and QML-facing contracts, error behavior, thread boundaries, and storage or transaction boundaries from production requirements, then identify the tests required for those contracts.
4. Make the smallest sufficient change. Follow the architecture and C++ style guidance, preserve unrelated work, and do not change comments for logic that did not change.
5. Update `CMakeLists.txt` source and test registration when C++ files or test targets are added, removed, or renamed.
6. Add or update tests for changed business logic, storage, parsing, algorithms, Qt model behavior, signals, or other high-risk contracts.
7. After implementation is stable, finish with one incremental final verification pass: inspect the final diff, use `cmake-build-debug` for the smallest meaningful production build and directly associated tests, then perform one documentation-impact check. Apply `test-and-review` as this integrated final checklist, not as a separate full research or architecture-review phase. Reuse established context and broaden the build, tests, documentation, or independent review only under the risk and evidence gates in `AGENTS.md`.

Treat `For-Agent/Docs/build.md` as the source of truth for build and test commands. Do not substitute undocumented commands.

## Boundaries

Keep `src/main.cpp` minimal and place non-trivial construction or QML wiring in focused application or bootstrap classes.

Keep QML-facing controllers and models small. Delegate durable behavior to services, repositories, and storage classes according to the selected architecture domain documents.

Do not broaden a backend task into unrelated refactoring, visual changes, or cleanup.
