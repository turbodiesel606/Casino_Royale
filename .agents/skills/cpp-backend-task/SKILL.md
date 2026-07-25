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
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/coding-style.md`
5. `For-Agent/Docs/testing.md`
6. `For-Agent/Docs/build.md`

Also read `For-Agent/Docs/qml-style.md` and `For-Agent/Docs/qml-to-cpp-extraction.md` when the work changes a QML-facing contract or moves behavior out of QML.

## Workflow

1. Inspect `git status`, the affected diff, and relevant recent artifacts before editing. Use artifacts as context only and verify current facts against the source.
2. Start from the smallest affected model, controller, service, repository, storage class, utility, or startup surface. Use `cpp-code-research` first when ownership, data flow, or implementation direction is unclear.
3. Define ownership, lifetime, public and QML-facing contracts, error behavior, thread boundaries, storage or transaction boundaries, and required tests before implementation.
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
