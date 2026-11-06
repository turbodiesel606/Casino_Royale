---
name: qml-codebase-review
description: Review the complete JobTracker QML architecture for global component boundaries, navigation and reachability, backend contracts, durable business-logic leakage, repeated UI structures, registration, and visual risk. Use for an explicitly requested whole-QML audit or an assigned partition of it, not a bounded diff or one existing component.
---

# QML Codebase Review

Use this deliberate broad mode only when the requested scope is the complete QML
surface or a codebase-wide UI architecture, decomposition, responsibility, or
duplication assessment. Use `qml-code-review` for a bounded diff or existing component.

For a delegated partition of a broad audit, apply the inventory and scrutiny below
to the assigned portion and necessary boundary contracts. Reuse established
navigation; independently verify critical code and assumptions. Report uncovered
boundaries to the lead, who owns overall coverage, overlaps, gaps, and reconciliation.
Each worker need not repeat the whole-QML inventory or unrelated navigation mapping.

## Context And Artifact Selection

1. Apply `AGENTS.md`. Inspect status and scoped diffs when Git metadata exists. Otherwise inspect the current files directly.
2. Read `For-Agent/Docs/qml-style.md` and
   `For-Agent/Docs/architecture/qml-contracts.md`. Load other architecture domain
   documents only as inspected QML contracts enter those domains.
3. Read extraction, testing, build, and artifact guidance only when those
   concerns enter the requested review.
4. Inventory prior artifact filenames and metadata before opening any artifact.
   Read only the newest broad artifact that overlaps the review and any newer
   narrow artifact that materially updates it.

Artifacts are navigation aids, not evidence. Verify all current claims against
live QML, exposed C++ contracts, CMake registration, and any relevant current diff.

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

Apply the shared [review evidence standard](../test-and-review/SKILL.md#review-stance)
when classifying findings, risks, and design suggestions.

Use source, symbol, and tight line references for every material finding,
architecture claim, ownership conclusion, and recommendation that requires code
changes. Routine inventory narration may reuse active verified context without
extra exploration.

Return:

- reviewed scope, available worktree evidence, and QML surfaces inventoried;
- findings ordered by severity;
- navigation, ownership, QML-to-C++ contract, and registration assessment;
- component-duplication and responsibility-overlap map;
- justified similarities that should remain separate;
- decomposition or reuse recommendations with API, registration, backend,
  testing, and manual-UI consequences;
- verification performed, manual checks, and residual risks.

Give an acceptance recommendation only when reviewing an actual acceptance decision.

When saving a durable review, follow `For-Agent/Docs/artifacts.md` and write it
under `For-Agent/Review/`.

## Boundaries

- Remain read-only unless the user separately authorizes implementation.
- Do not run builds, CTest, or the GUI unless requested or included in the review.
- Do not recommend a global visual redesign unless the user asks for it.
- Do not split components by line count alone; require a responsibility,
  ownership, reuse, testability, or navigation reason.
