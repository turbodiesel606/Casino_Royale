---
name: qml-codebase-review
description: Review the complete JobTracker QML architecture for global component boundaries, navigation and reachability, QML-to-C++ contracts, durable business-logic leakage, repeated UI structures, registration, and manual visual risk. Use only for an explicitly requested whole-QML or codebase-wide UI architecture review, not for a bounded diff.
---

# QML Codebase Review

Use this deliberate broad mode only when the requested scope is the complete QML
surface or a codebase-wide UI architecture, decomposition, responsibility, or
duplication assessment. Use `qml-code-review` for ordinary changed-diff review.

## Context And Artifact Selection

1. Apply `AGENTS.md` and record current worktree state.
2. Read `For-Agent/Docs/qml-style.md` and
   `For-Agent/Docs/architecture/qml-contracts.md`. Load other architecture domain
   documents only as inspected QML contracts enter those domains.
3. Read extraction, testing, build, and artifact guidance only when those
   concerns enter the requested review.
4. Inventory prior artifact filenames and metadata before opening any artifact.
   Read only the newest broad artifact that overlaps the review and any newer
   narrow artifact that materially updates it.

Artifacts are navigation aids, not evidence. Verify all current claims against
live QML, exposed C++ contracts, CMake registration, and the current diff.

## Deliberate QML Inventory

- Inventory `qml/Main.qml`, every page and reusable component, navigation and
  instantiation paths, imports, QML module registration, and consumed backend
  properties, signals, commands, and model roles.
- Map component ownership, inputs, outputs, local UI state, durable state,
  repeated structures, and backend contract boundaries.
- Inspect architecture drift, unreachable or over-coupled pages, responsibility
  overlap, business logic in QML, global component duplication, and broad
  decomposition or reuse opportunities.
- Use targeted searches to find candidate patterns, then inspect every reported
  instance before classifying it. Similar appearance alone is not duplication.

## Review Priorities

Lead with findings ordered by severity:

- runtime-breaking references, missing imports, registration, or reachability;
- broken bindings, navigation, handlers, model roles, or backend contracts;
- durable business logic or authoritative product state in QML;
- harmful component duplication or overlapping ownership;
- visual overflow, accessibility, input, focus, and state-transition risks;
- missing build, test, or manual UI verification.

## Evidence And Output

Use source, symbol, and tight line references for every material finding,
architecture claim, ownership conclusion, and recommendation that requires code
changes. Routine inventory narration may reuse active verified context without
extra exploration.

Return:

- reviewed scope, worktree state, and QML surfaces inventoried;
- findings ordered by severity;
- navigation, ownership, QML-to-C++ contract, and registration assessment;
- component-duplication and responsibility-overlap map;
- justified similarities that should remain separate;
- decomposition or reuse recommendations with API, registration, backend,
  testing, and manual-UI consequences;
- verification performed, manual checks, residual risks, and a final accept,
  revise, or block recommendation.

When saving a durable review, follow `For-Agent/Docs/artifacts.md` and write it
under `For-Agent/Review/`.

## Boundaries

- Remain read-only unless the user separately authorizes implementation.
- Do not run builds, CTest, or the GUI unless requested or included in the review.
- Do not recommend a global visual redesign unless the user asks for it.
- Do not split components by line count alone; require a responsibility,
  ownership, reuse, testability, or navigation reason.
