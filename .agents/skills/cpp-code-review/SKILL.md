---
name: cpp-code-review
description: Review a bounded JobTracker C++ backend diff for correctness, regressions, ownership, lifetime, threading, data integrity, architecture boundaries touched by the change, tests, CMake registration, and verification gaps. Do not use for whole-codebase architecture or global duplication review.
---

# C++ Code Review

Use this skill for bounded C++ backend change review. Use
`cpp-codebase-review` only when the user explicitly requests a whole-backend
architecture, responsibility-overlap, or global duplication review.

## Context Selection

1. Apply `AGENTS.md`, inspect the focused diff, and identify the affected
   backend subsystem before opening detailed documentation.
2. Read only the matching domain documents under
   `For-Agent/Docs/architecture/`. Use the overview only when the correct domain
   documents are unclear.
3. Read `For-Agent/Docs/coding-style.md` for C++ implementation concerns.
4. Read `For-Agent/Docs/artifacts.md` only when relevant prior review artifacts
   may overlap the scope or a durable review artifact will be created.
5. Read `For-Agent/Docs/testing.md` when changed logic, test coverage, test
   registration, or a high-risk contract is relevant.
6. Read `For-Agent/Docs/build.md` only when the review includes build/test
   execution, changes build configuration, or needs its verification commands.

Expand to another architecture document only when the diff or a directly traced
dependency crosses that boundary. Do not preload the complete architecture,
testing, build, and artifact bundle for a bounded review.

## Scope Gate

1. Start from the final diff and changed C++ or CMake files.
2. Inspect the changed code first, then direct callers, callees, owners,
   consumers, tests, and directly affected public or QML contracts.
3. Expand beyond that boundary only when a concrete finding or risk requires
   broader evidence. State each material expansion and why it is necessary.
4. Do not inventory every module, build a whole-codebase duplication map, or
   reopen unrelated architecture areas during bounded review.
5. If prior review artifacts may overlap, inventory filenames and metadata
   first. Read only the newest broad artifact relevant to the changed subsystem
   and any newer narrow artifact that materially updates it. Do not reread an
   artifact in the same review without a concrete reason.

Artifacts are context only. Current source and the final diff are authoritative.
Use file and line evidence for material findings, architecture claims, ownership
or lifetime claims, concurrency claims, and recommendations that require code
changes. Do not expand repository exploration solely to attach lines to routine
narration already established in the active context.

## Review Surfaces

Inspect the changed C++ files and nearby code before broad scans:

- `src/app/` and `src/main.cpp` for bootstrap and dependency wiring.
- `src/common/` for shared filtering, validation, and utility behavior.
- `src/jobs/`, `src/cvs/`, `src/dashboard/`, `src/directory/` for domain code.
- `tests/` for matching Qt Test coverage.
- `CMakeLists.txt` for added or removed C++ sources and tests.

Use `git diff` and targeted `rg` searches for affected symbols, role names, properties, invokables, signal names, and test registrations.

## Review Priorities

Lead with findings ordered by severity:

- Correctness bugs and behavior regressions.
- Broken Qt ownership, lifetime, signal, or model/view contracts.
- Harmful duplication or overlapping responsibility that can cause behavior drift, inconsistent fixes, excessive maintenance cost, or unnecessary coupling.
- Missing `Q_PROPERTY` notify signals or unstable model roles consumed by QML.
- Business logic left in QML when the change claims a backend migration.
- Production code, APIs, access control, ownership, threading, behavior, or architecture changed solely to accommodate tests; reverse dependencies from production code or normal build paths to test code, Qt Test, fixtures, mocks, test-only settings, or test targets.
- Missing tests for validation, parsing, filtering, sorting, model roles, signals, or high-risk behavior.
- Build or CMake registration mistakes.
- Scope creep and unrelated refactors.

Keep style-only comments out unless they hide a real maintainability or behavior risk.

## In-Scope Duplication Review

Report duplication only when it is encountered inside the changed code or its
direct dependencies and is materially relevant to the change. Do not search the
whole codebase for duplication in this mode.

For every duplication finding:

1. Cite every affected file, symbol, and relevant line.
2. Classify it as:
   - exact duplication;
   - structural duplication;
   - semantic duplication;
   - overlapping responsibility;
   - intentional similarity.
3. Identify the canonical responsibility and the existing class, function, service, model, repository, component, or helper that should own it.
4. Explain the concrete risk:
   - behavior or validation drift;
   - fixes needing changes in several locations;
   - inconsistent error handling;
   - duplicated state or signals;
   - conflicting transaction or threading behavior;
   - unnecessary test and CMake maintenance.
5. Recommend one outcome:
   - reuse the existing implementation;
   - extend the existing implementation;
   - extract a shared implementation;
   - keep the implementations separate.
6. When recommending consolidation, describe ownership, dependency direction, public-contract, and test consequences.
7. Do not report similar-looking code as harmful duplication when the implementations have different domain meaning, ownership, lifetime, invariants, thread affinity, or reasons to change.
8. Do not recommend a new abstraction when an existing implementation can be reused or extended without weakening clarity or responsibility boundaries.

## Review Output

Use file and line references for findings. Include:

- Findings.
- Open questions or assumptions.
- Verification performed or still needed.
- Residual risk.
- Final recommendation: accept, revise, or block.

For an in-scope duplication finding, include its classification, canonical
responsibility, concrete drift risk, and recommended outcome. Do not emit a
global duplication map.

When saving a durable review artifact, write the final review under `For-Agent/Review/` with a clear name such as `cpp-review-YYYY-MM-DD-HHMM-topic.md`. Put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

Use this artifact shape:

- Title and reviewed scope.
- Inputs inspected.
- Findings ordered by severity, with file and line references.
- Verification performed.
- Missing tests, manual checks, or unverified areas.
- Residual risks.
- Final recommendation.

## Boundaries

Do not fix reviewed code unless the user explicitly asks for implementation.
Do not run destructive git commands.
Run build or tests only when requested or when the review task explicitly includes verification.
Do not recommend deduplication when the resulting abstraction would combine unrelated responsibilities, hide ownership, weaken domain types, cross thread boundaries, or create broader coupling than the duplication it removes.
Route an explicitly requested whole-backend architecture or global duplication
review to `cpp-codebase-review`.
