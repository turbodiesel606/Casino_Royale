---
name: cpp-code-review
description: Review JobTracker C++ backend changes or whole-codebase architecture for correctness, regressions, duplication, overlapping responsibilities, architecture boundaries, Qt model/controller behavior, tests, CMake registration, and verification gaps.
---

# C++ Code Review

Use this skill for C++ backend review.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/coding-style.md`
5. `For-Agent/Docs/testing.md`
6. `For-Agent/Docs/build.md`

## Review Surfaces

For a change review, inspect the changed C++ files and nearby code before broad scans. 
For a whole-codebase architecture review, inventory every current C++ module, test translation unit, QML-facing contract, and CMake target before evaluating cross-module duplication.

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

## Duplication Review

Review duplication as an architecture concern, not merely a style concern.

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
- Duplication map grouped by canonical responsibility.
- Justified duplication that should remain unchanged.
- Consolidation recommendations ordered by maintenance risk and expected benefit.

When saving a durable review artifact, write the final review under `For-Agent/Review/` with a clear name such as `cpp-review-YYYY-MM-DD-HHMM-topic.md`. Put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

Use this artifact shape:

- Title and reviewed scope.
- Inputs inspected.
- Findings ordered by severity, with file and line references.
- Verification performed.
- Missing tests, manual checks, or unverified areas.
- Residual risks.
- Final recommendation.

If a read-only subagent is used, treat the subagent output as review input. The lead Codex should compile, verify, and save the final review artifact.

## Boundaries

Do not fix reviewed code unless the user explicitly asks for implementation.
Do not run destructive git commands.
Run build or tests only when requested or when the review task explicitly includes verification.
Do not recommend deduplication when the resulting abstraction would combine unrelated responsibilities, hide ownership, weaken domain types, cross thread boundaries, or create broader coupling than the duplication it removes.
