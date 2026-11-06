---
name: qml-to-cpp-extraction
description: Identify and migrate JobTracker QML business logic into C++ backend classes, models, controllers, services, or storage while preserving UI behavior. Use when Codex needs to extract JavaScript functions, mock data, durable state, validation, filtering, sorting, parsing, storage, or QML-owned product logic into C++.
---

# QML To C++ Extraction

Use this skill when moving durable behavior from QML into the JobTracker C++ backend.

## Workflow

1. Apply `AGENTS.md`, then start from the QML owner of the behavior and identify
   the affected product subsystem.
2. Read `For-Agent/Docs/qml-to-cpp-extraction.md`,
   `For-Agent/Docs/qml-style.md`, and `For-Agent/Docs/coding-style.md`.
3. Read `For-Agent/Docs/architecture/boundaries.md` and
   `For-Agent/Docs/architecture/qml-contracts.md`. Add only the affected domain
   documents under `For-Agent/Docs/architecture/` when the traced behavior
   reaches their contracts; use the overview only if routing is unclear.
4. Read `For-Agent/Docs/testing.md` when the extracted behavior requires test
   strategy, test registration, or coverage changes. Read
   `For-Agent/Docs/build.md` only when CMake/build behavior is involved or its
   commands are needed for verification.
5. Inspect connected pages, components, and C++ objects exposed to QML. Expand
   documentation context only when this trace exposes another boundary.
6. Classify current QML logic as UI-only state or durable product behavior.
7. Before editing, define the production contract and apply the
   [behavioral equivalence guidance](../../../For-Agent/Docs/qml-to-cpp-extraction.md#behavioral-equivalence).
   Trace producers and consumers and choose observable examples that distinguish
   authorized changes from behavior to preserve.
8. Move durable rules into focused backend owners. Keep the QML-facing API as
   small as the product requires. Do not expose internals or widen it solely for tests.
9. Keep QML as bindings, layout, navigation, presentation, and simple UI state.
10. Keep `src/main.cpp` limited to bootstrap, QML engine setup, registration, dependency wiring, and startup logic.
11. Update CMake file lists when adding C++ or QML files.
12. Reuse existing checks for the chosen examples and add coverage where a gap remains. Keep test support under `tests/`.

## Verification

After the move, compare the chosen examples with their expected outcomes, inspect the changed files (and status/diffs when Git metadata exists) and complete one incremental `test-and-review` pass using the context already established during extraction.

Use `cmake-build-debug` for required build/tests. `test-and-review` owns integrated verification, build expansion, and independent-review criteria; `For-Agent/Docs/testing.md` owns test selection, quality, and expansion for the extracted contracts.

Follow `For-Agent/Docs/qml-style.md` for relevant runtime/visual inspection in a
suitable available environment and for concrete manual checks when unavailable.

After applicable verification and any review fixes, synchronize directly affected
descriptions under [Architecture Documentation](../../../AGENTS.md#architecture-documentation).
Revisit only evidence or descriptions invalidated by a later change.
