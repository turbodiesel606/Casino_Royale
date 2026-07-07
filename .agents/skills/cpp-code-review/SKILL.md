---
name: cpp-code-review
description: Review JobTracker C++ backend changes for correctness, regressions, architecture boundaries, Qt model/controller behavior, tests, and verification gaps. Use for C++ code review of src, tests, CMake source registration, startup wiring, and QML-facing backend contracts.
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

Read `For-Agent/Docs/qml-to-cpp-extraction.md` when the review touches QML-facing APIs, controllers, models, validation, filtering, sorting, search, or migration of business logic out of QML.

If relevant research or review artifacts exist under `For-Agent/Research/` or `For-Agent/Review/`, read the most recent relevant artifact by timestamp before reviewing. Use artifacts as context only, not as proof. Verify every finding against `AGENTS.md`, `For-Agent/Docs/`, `git diff`, and the actual code.

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
- Missing `Q_PROPERTY` notify signals or unstable model roles consumed by QML.
- Business logic left in QML when the change claims a backend migration.
- Missing tests for validation, parsing, filtering, sorting, model roles, signals, or high-risk behavior.
- Build or CMake registration mistakes.
- Scope creep and unrelated refactors.

Keep style-only comments out unless they hide a real maintainability or behavior risk.

## Review Output

Use file and line references for findings. Include:

- Findings.
- Open questions or assumptions.
- Verification performed or still needed.
- Residual risk.
- Final recommendation: accept, revise, or block.

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
