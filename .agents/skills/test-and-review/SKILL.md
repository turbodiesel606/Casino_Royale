---
name: test-and-review
description: Review and validate JobTracker changes before final response. Use when Codex needs an implementation review, risk check, build/test decision, final verification pass, scope audit, or concise summary of changed files, manual checks, limitations, and follow-up ideas.
---

# Test And Review

Use this skill as the integrated incremental final review and validation pass.

## Incremental Final Verification

Start from the final diff, changed files, and verification evidence. Reuse the instructions, architecture context, source relationships, and contracts already established during implementation. Do not restart repository research, reread unchanged documentation, or retrace understood dependencies merely because implementation ended.

Expand only when the final diff introduces a new dependency, an implementation assumption changed, an unexpected build or test result occurs, a new risk becomes visible, or a concrete verification question requires broader evidence.

## Review Checklist

1. Inspect the final diff and changed files. Confirm the change stays within the user's scope and that no unrelated files, comments, or behavior changed.
2. Check whether `AGENTS.md`, docs, skills, or subagent instructions changed the expected workflow only when those files are part of the final diff or the implementation exposed a workflow conflict.
3. Check `For-Agent/Docs/artifacts.md` only when review artifacts were created or reused.
4. Verify the final diff against the applicable architecture documents already established for the task. Open another document, or reread one, only when the incremental expansion rules require it; use the overview only when routing is unclear or the review is explicitly system-wide.
5. Verify the test dependency remains strictly `tests -> production`: no production source, target, API, normal build path, or deployment step may depend on test code, Qt Test, fixtures, mocks, test-only settings, or test targets, and production must not be reshaped solely for tests.
6. Verify affected QML-facing properties, commands, signals, and model roles remain compatible with their consumers.
7. Confirm added, removed, or renamed C++, QML, and test files are registered correctly in `CMakeLists.txt`.
8. When testing is relevant, apply `For-Agent/Docs/testing.md` for test quality, initial subsystem-to-test selection, and criteria for expanding coverage. Confirm tests exercise the changed contract and can detect its defect; retain proportionate before/after regression evidence when feasible. Do not leave risky behavior unverified merely to reduce execution time or usage.
9. After the implementation is stable, perform one documentation-impact check from the final changed behavior and contracts. Inspect only directly affected guidance, update it or record that no update is required, and do not repeat the check unless a later edit changes the impact.
10. After code, QML, CMake, resource, storage, or runtime-behavior changes,
    complete the smallest sufficient production build unless verification is
    impossible. For routine Windows and Linux tasks, build Debug through
    `JobTrackerApp_dev_runtime` to include application/runtime deployment, and
    build and run required tests in a separate Debug test preset. Broaden
    production targets or test coverage for shared infrastructure, cross-target
    APIs, build-system behavior, several modules, merge/release readiness, or
    a diagnosed failure; routine expansion stays within Debug. Release builds
    and Release tests require an explicit user request, including after Debug
    failures. Follow the [routine workflow](../../../For-Agent/Docs/build.md#routine-codex-workflow)
    for configuration selection and command forms, and use `cmake-build-debug`
    for execution. Do not repeat an unchanged successful run without a concrete
    reason.
11. Include applicable QML/UI runtime and visual evidence in this final pass before reporting completion. Apply `For-Agent/Docs/qml-style.md` for focused inspection when a suitable environment is available. Reuse sufficiently current evidence for the affected behavior; do not repeat a GUI launch or visual inspection merely because the UI workflow also mentions it. When unavailable, identify the specific verification gap and provide focused manual actions and expected results. Keep build success distinct from runtime and visual verification. Report `passed`, `failed`, `not run`, and `no matching tests` under `testing.md`; do not claim unverified behavior works.
12. Complete the report below, including any required validation that could not be performed and why.

## Full Independent Review

Completion alone is not a reason to invoke a full independent reviewer. Use one
when concurrency or synchronization, worker/runtime behavior, ownership or
lifetime, storage or transaction semantics, architecture boundaries, shared
infrastructure, broad multi-subsystem behavior, significant public or internal
contracts, a complex refactor, merge/release readiness, or an explicit user
request makes independent verification materially useful. Follow the delegation
policy in `AGENTS.md`.

## Review Stance

For code review requests, lead with concrete findings ordered by severity.

For implementation tasks, keep the final response concise and focused on what changed, why, and what was verified.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless build behavior changed.

Do not claim code works unless it was verified.

## Completion Report

After making changes, report concisely:

1. What changed and why this solution fits the scope.
2. Files added, modified, or removed.
3. Verification performed, actual build/test results, and any checks not run with reasons.
4. Documentation updated, or confirmation that the focused impact check required no update.
5. Manual checks, residual risks, limitations, and unverified areas.
6. Improvement ideas only when relevant.

The task is complete when the scoped implementation and required validation are
finished, with any genuine blocker or verification limitation stated accurately.
