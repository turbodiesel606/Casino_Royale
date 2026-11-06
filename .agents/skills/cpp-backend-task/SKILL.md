---
name: cpp-backend-task
description: Work on JobTracker C++ backend tasks. Use when Codex needs to add or modify C++ models, services, controllers, storage, parsing, algorithms, application state, QObject integrations, QML-exposed objects, or startup wiring outside simple QML-only UI changes.
---

# C++ Backend Task

Use this skill for C++ backend work in JobTracker.

## Context Selection

1. Apply `AGENTS.md`, then identify the affected backend subsystem from the
   request, target code, and focused diff when available.
2. Read only the matching domain documents under
   `For-Agent/Docs/architecture/`. Use
   `For-Agent/Docs/architecture/overview.md` only when the correct domain document
   is unclear.
3. Read `For-Agent/Docs/coding-style.md` for C++ conventions. Use the
   [Best Practices index](../../../For-Agent/CPP_Qt6_Best_Practices_EN/README.md)
   to select only relevant topics, such as types for payloads, error handling
   for failure boundaries, or Qt ownership for worker lifetimes. Current
   project instructions take precedence over conflicting reference advice.
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

1. Apply `For-Agent/Docs/coding-style.md` for canonical reuse/generalization,
   C++ conventions, template placement, compile-time decisions, and comments.
   Inspect suitable existing entities before adding an owner, and preserve the
   contracts in the affected architecture documents.
2. Keep the test dependency one-way. Do not add or widen production APIs, expose internals, weaken access control, add test-only hooks or compile definitions, or alter production ownership, lifetime, threading, behavior, or architecture solely for tests. Keep fixtures, mocks, fakes, and helpers under `tests/`.

## Workflow

1. When Git metadata exists, inspect `git status` and existing diffs for files in scope; otherwise inspect those files directly. Identify and preserve pre-existing changes.
2. Start from the smallest affected model, controller, service, repository, storage class, utility, or startup surface. Use `cpp-code-research` first when ownership, data flow, or implementation direction is unclear.
3. Define ownership, lifetime, public and QML-facing contracts, error behavior, thread boundaries, and storage or transaction boundaries from production requirements, then identify the tests required for those contracts.
4. For each changed behavior, assess its outcomes under the
   [logging policy](../../../For-Agent/Docs/architecture/logging.md#policy-for-new-and-changed-behavior).
   Implement missing events at the confirming owner and verify the business
   result and diagnostic together. Make the smallest sufficient change. Preserve unrelated work and apply the canonical comment policy: use `// Codex Comment:` for added ordinary explanatory comments, preserve unmarked comments with unchanged code, update associated comments when code changes, and correct stale marked comments in scope. Comments never restrict necessary authorized code changes.
5. Update `CMakeLists.txt` source and test registration when C++ files or test targets are added, removed, or renamed.
6. Use `For-Agent/Docs/testing.md` to select meaningful regression coverage for changed business logic, storage, parsing, algorithms, Qt model behavior, signals, or high-risk contracts. Start from its verified subsystem-to-test map and expand for affected contracts and risk.
7. After implementation is stable, complete one incremental `test-and-review` pass: inspect the changed files and available diff, execute the required build/tests through `cmake-build-debug`, and check documentation impact. Reuse established context. `test-and-review` owns integrated verification and independent-review criteria; `testing.md` owns test-scope expansion.

Use [build.md](../../../For-Agent/Docs/build.md) for shared action selection and
its relevant [Windows](../../../For-Agent/Docs/build-windows.md) or
[Linux](../../../For-Agent/Docs/build-linux.md) reference for command forms.
Substitute verified presets, targets, and CTest selections; do not invent commands.

## Boundaries

Keep `src/main.cpp` minimal and place non-trivial construction or QML wiring in focused application or bootstrap classes.

Keep QML-facing controllers and models small. Delegate durable behavior to services, repositories, and storage classes according to the selected architecture domain documents.

Do not broaden a backend task into unrelated refactoring, visual changes, or cleanup.
