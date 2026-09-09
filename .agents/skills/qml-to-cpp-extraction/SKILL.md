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
7. Define the QML/C++ contract before editing:
   - readable properties;
   - invokable commands or slots;
   - notify and result signals;
   - model roles for delegates.
   Define this contract from product responsibilities. Do not expose internals, add commands or signals, or widen production APIs solely for tests.
8. Move durable behavior into focused C++ backend classes:
   - models for repeated delegate data;
   - controllers or view models for QML-facing state and commands;
   - services for validation, parsing, filtering, sorting, storage coordination, or algorithms.
9. Keep QML as bindings, layout, navigation, presentation, and simple UI state.
10. Keep `src/main.cpp` limited to bootstrap, QML engine setup, registration, dependency wiring, and startup logic.
11. Update CMake file lists when adding C++ or QML files.
12. Add or update tests for changed business logic, storage, parsing, algorithms, model roles, or signal behavior. Keep test support under `tests/` and make tests follow the production contract without reshaping it.

## Verification

After implementation is stable, inspect the final diff and complete one incremental `test-and-review` pass using the context already established during extraction.

Use `cmake-build-debug` for the smallest meaningful production build and the tests directly associated with the extracted logic, storage, parsing, algorithms, models, signals, or other high-risk behavior. Broaden the build, tests, context, or independent review only under the risk and evidence gates in `AGENTS.md`.

Perform one documentation-impact check after build and test verification. Do not repeat architecture exploration or documentation review unless the final diff or verification results expose a new dependency, changed assumption, or risk.

List manual UI checks when visual behavior must be confirmed by the user.
