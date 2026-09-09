---
name: qml-code-review
description: Review a bounded JobTracker QML/UI diff for correctness, visual regressions, component boundaries touched by the change, QML/C++ contract usage, business logic leakage, QML registration, and manual verification gaps. Do not use for whole-QML architecture or global duplication review.
---

# QML Code Review

Use this skill for bounded QML and UI change review. Use
`qml-codebase-review` only when the user explicitly requests a whole-QML
architecture, decomposition, responsibility-overlap, or global duplication
review.

## Context Selection

1. Apply `AGENTS.md`, inspect the focused diff, and identify the affected QML
   surface and any backend contract it consumes.
2. Read `For-Agent/Docs/qml-style.md` for QML/UI review guidance.
3. Read only the matching documents under `For-Agent/Docs/architecture/`.
   Start with `qml-contracts.md` when properties, commands, signals, model roles,
   selection, or publication changed; add a domain document only when the diff or
   traced dependency enters that domain. Use the overview only when routing is
   unclear.
4. Read `For-Agent/Docs/artifacts.md` only when relevant prior review artifacts
   may overlap the scope or a durable review artifact will be created.
5. Read `For-Agent/Docs/testing.md` only when changed durable behavior, test
   coverage, test registration, or a high-risk backend contract is relevant.
6. Read `For-Agent/Docs/build.md` only when the review includes build/runtime
   verification, changes QML/CMake registration, or needs its commands.

Expand context only when inspected evidence crosses the current boundary. Do not
preload the complete architecture, artifact, build, and testing bundle for a
bounded QML review.

Read `For-Agent/Docs/qml-to-cpp-extraction.md` when the change adds or modifies QML JavaScript, mock data, validation, filtering, sorting, search, cross-screen state, or backend-facing contracts.

## Scope Gate

1. Start from the final diff and changed QML or CMake files.
2. Inspect the changed components first, then their direct instantiators,
   bindings, handlers, backend contracts, registration, and directly affected
   tests or manual UI paths.
3. Expand beyond that boundary only when a concrete finding or risk requires
   broader evidence. State each material expansion and why it is necessary.
4. Do not inventory the whole QML tree or build a global component-duplication
   map during bounded review.
5. If prior review artifacts may overlap, inventory filenames and metadata
   first. Read only the newest broad artifact relevant to the changed surface
   and any newer narrow artifact that materially updates it. Do not reread an
   artifact in the same review without a concrete reason.

Artifacts are context only. Current QML, exposed C++ contracts, and the final
diff are authoritative. Use file and line evidence for material findings,
architecture or ownership claims, and recommendations that require code changes.
Do not expand repository exploration solely to attach lines to routine narration
already established in the active context.

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

When saving a durable review artifact, write the final review under `For-Agent/Review/` with a clear name such as `qml-review-YYYY-MM-DD-HHMM-topic.md`. Put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

Use this artifact shape:

- Title and reviewed scope.
- Inputs inspected.
- Findings ordered by severity, with file and line references.
- Build or runtime verification performed.
- Manual UI checks still needed.
- Missing tests or unverified behavior.
- Residual risks.
- Final recommendation.

## Boundaries

Do not fix reviewed code unless the user explicitly asks for implementation.
Do not run destructive git commands.
Run a build only when requested or when the review task explicitly includes verification.
Route an explicitly requested whole-QML architecture, decomposition, or global
duplication review to `qml-codebase-review`.
