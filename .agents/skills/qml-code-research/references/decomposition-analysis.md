# QML Decomposition Analysis

Read this reference only for whole-QML architecture, file-splitting, component-boundary, duplication, or maintainability research.

## Inventory And Reachability

1. Start from `qml/Main.qml` and map the shell, navigation, overlays, forms, and page ownership.
2. Treat the `qt_add_qml_module` list in `CMakeLists.txt` as the registered QML inventory, then compare it with `rg --files qml -g '*.qml'`.
3. Record approximate file sizes to prioritize inspection, not to decide that a file must be split.
4. Inspect large pages and panes first, then the shared components and the parent surfaces that instantiate them.
5. Distinguish files that exist, files that are registered, components that are instantiated, and pages that are reachable through current navigation.

## Candidate Analysis

Look for:

- files that combine several independent responsibilities such as shell ownership, data presentation, tables, cards, dialogs, overlays, and local controls;
- repeated local structures or helpers that represent the same UI responsibility and contract;
- state whose owner is unclear or is passed through several unrelated component layers;
- local component APIs with too many unrelated properties, writable state that should be internal, or missing intent signals;
- durable mock data, validation, filtering, sorting, search, grouping, parsing, storage, or cross-screen behavior that belongs behind a C++ controller or model;
- extraction boundaries likely to cause visual, binding, focus, sizing, z-order, or lifecycle regressions.

For each candidate:

1. Cite the file, lines, component or local object, and its current responsibilities.
2. Identify existing shared components or parent-owned behavior that could be reused or extended before proposing a new component.
3. Define the proposed owner and boundary, including input properties, `required property` values, output signals, model roles, and state that remains internal.
4. Separate QML component extraction from QML-to-C++ business-logic extraction. A file may need one, both, or neither.
5. Record QML registration, import, parent-instantiation, backend-contract, and manual visual-verification impact.
6. State the behavior and visual invariants that the future split must preserve.

Similar syntax or a high line count is not enough to justify consolidation. Keep structures separate when their responsibility, lifecycle, state owner, visual context, or reasons to change differ.

## Priority

Classify every recommendation:

- `Needed now`: multiple responsibilities or repeated structures are actively obscuring ownership, increasing regression risk, or making the requested implementation unsafe to extend in place.
- `Optional later`: a clear boundary exists and would improve readability or reuse, but the current file remains understandable and safe for the requested work.
- `Do not refactor now`: the split would add indirection, create a one-use abstraction without a stable contract, disturb visual behavior, or expand beyond the requested scope.

## Deliverable

Include:

- Current QML structure, registration, instantiation, and reachability summary.
- Candidate table with priority, evidence, current responsibilities, proposed boundary, reusable implementation, API contract, and rationale.
- Files or structures that should remain intact and the boundary that justifies keeping them together.
- Durable behavior that should move to C++, clearly separated from presentation-only component extraction.
- CMake, backend-contract, build, and manual UI verification required for any future split.
- Visual and behavioral regression risks.
- A short conclusion stating whether further QML refactoring is needed for the requested scope.
