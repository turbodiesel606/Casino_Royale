---
name: qml-code-review
description: Review JobTracker QML/UI changes for correctness, visual regressions, component boundaries, QML/C++ contract usage, business logic leakage, CMake QML registration, and manual verification gaps. Use for QML files under qml, QML-related CMake changes, and UI changes consuming backend models or controllers.
---

# QML Code Review

Use this skill for QML and UI review.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/qml-style.md`
4. `For-Agent/Docs/build.md`

Read `For-Agent/Docs/qml-to-cpp-extraction.md` when the change adds or modifies QML JavaScript, mock data, validation, filtering, sorting, search, cross-screen state, or backend-facing contracts.

## Review Surfaces

Inspect the changed QML files and nearby components before broad scans:

- `qml/Main.qml` for shell, navigation, and page wiring.
- `qml/components/` for reusable UI controls.
- `qml/pages/` for page-level behavior and feature panes.
- Related `src/` controllers or models when QML consumes backend state.
- `CMakeLists.txt` when QML files are added, removed, or renamed.

Use `git diff` and targeted `rg` searches for changed component names, imports, property names, signal handlers, backend object references, model roles, and QML file registration.

## Review Priorities

Lead with findings ordered by severity:

- Runtime-breaking QML references, missing imports, or unregistered files.
- Broken navigation, page wiring, bindings, signal handlers, or model role usage.
- Business logic, durable state, validation, filtering, sorting, or search added directly to QML.
- Regressions in component APIs or repeated UI patterns that should be shared.
- Text overlap, unstable dimensions, inaccessible controls, or likely visual breakage.
- Missing build or manual UI verification.
- Scope creep and global style changes not requested by the task.

Keep purely subjective design comments out unless they affect usability, consistency, or maintainability.

## Review Output

Use file and line references for findings. Include:

- Findings.
- Open questions or assumptions.
- Required manual UI checks.
- Build or runtime verification still needed.
- Residual risk.
- Final recommendation: accept, revise, or block.

Write it under `For-Agent/Review/` with a clear name such as `qml-review-YYYY-MM-DD-topic.md`.

## Boundaries

Do not fix reviewed code unless the user explicitly asks for implementation.
Do not run destructive git commands.
Run a build only when requested or when the review task explicitly includes verification.
