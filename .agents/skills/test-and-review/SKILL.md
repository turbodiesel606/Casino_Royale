---
name: test-and-review
description: Review and validate JobTracker changes before final response. Use when Codex needs an implementation review, risk check, build/test decision, final verification pass, scope audit, or concise summary of changed files, manual checks, limitations, and follow-up ideas.
---

# Test And Review

Use this skill as the integrated incremental final review and validation pass.

## Incremental Final Verification

Start from the user's request, the acceptance conditions, the changed files, and current verification evidence. With Git metadata, inspect status and focused diffs. Otherwise inspect the files directly. Reuse the instructions, architecture context, source relationships, and contracts already established during implementation. Do not restart repository research, reread unchanged documentation, or retrace understood dependencies merely because implementation ended.

Expand only when the final changes introduce a new dependency, an implementation assumption changed, an unexpected build or test result occurs, a new risk becomes visible, or a concrete verification question requires broader evidence.

## Review Checklist

1. Inspect the changed files and any available diff. Confirm the change stays within the user's scope and that no unrelated files, comments, or behavior changed.
2. Check whether `AGENTS.md`, docs, skills, or subagent instructions changed the expected workflow only when those files changed or the implementation exposed a workflow conflict.
3. Check `For-Agent/Docs/artifacts.md` only when review artifacts were created or reused.
4. Identify the evidence needed for each requested outcome, including any requirement absent from the diff. Use architecture descriptions to locate current code and tests, then assess behavior against the user's request and explicit project policies.
5. Verify the test dependency remains strictly `tests -> production`: no production source, target, API, normal build path, or deployment step may depend on test code, Qt Test, fixtures, mocks, test-only settings, or test targets, and production must not be reshaped solely for tests.
6. Verify affected QML-facing properties, commands, signals, and model roles remain compatible with their consumers.
7. Confirm added, removed, or renamed C++, QML, and test files are registered correctly in `CMakeLists.txt`.
8. When testing is relevant, apply `For-Agent/Docs/testing.md` for test quality, initial subsystem-to-test selection, and criteria for expanding coverage. Read only the relevant section of [testing-reference.md](../../../For-Agent/Docs/testing-reference.md) when a specific suite, fixture, log, or manual scenario needs detail. Confirm tests exercise the changed contract and can detect its defect; retain proportionate before/after regression evidence when feasible. Do not leave risky behavior unverified merely to reduce execution time or usage.
9. After code, QML, CMake, resource, storage, or runtime-behavior changes,
    complete the smallest sufficient production build unless verification is
    impossible. Follow the [routine workflow](../../../For-Agent/Docs/build.md#routine-codex-workflow)
    for shared action selection and verification order, then the relevant
    [Windows](../../../For-Agent/Docs/build-windows.md) or
    [Linux](../../../For-Agent/Docs/build-linux.md) reference for commands/runtime
    details; use `cmake-build-debug` for execution. Broaden production targets or test coverage for shared infrastructure, cross-target
    APIs, build-system behavior, several modules, merge/release readiness, or
    a diagnosed failure. Do not repeat an unchanged successful run without a
    concrete reason.
10. Include applicable QML/UI runtime and visual evidence in this final pass before reporting completion. Apply `For-Agent/Docs/qml-style.md` for focused inspection when a suitable environment is available. Reuse sufficiently current evidence for the affected behavior; do not repeat a GUI launch or visual inspection merely because the UI workflow also mentions it. When unavailable, identify the specific verification gap and provide focused manual actions and expected results. Keep build success distinct from runtime and visual verification. Report `passed`, `failed`, `not run`, and `no matching tests` under [test outcomes](../../../For-Agent/Docs/testing.md#test-outcomes); do not claim unverified behavior works.
11. For each changed behavior, check the [logging policy](../../../For-Agent/Docs/architecture/logging.md#policy-for-new-and-changed-behavior).
    Verify event ownership, severity, payload privacy, and the diagnostic against the actual business outcome. Apply the policy's producer and catalog checks to the logging changes in scope. If existing logging suffices or no additional event is useful, say why in the final report.
12. Apply the [independent review criteria](#independent-review), reconcile findings, and verify fixes. Revisit any acceptance evidence affected by those fixes.
13. After applicable verification and review fixes, make the final documentation-impact check under [Architecture Documentation](../../../AGENTS.md#architecture-documentation). Update directly affected descriptions, check consistency and references, and record any verification gap. Revisit only what later changes invalidate.
14. Check the final result against every requested outcome. In the [report](#completion-report), give the evidence for fulfilled conditions and identify anything incomplete, blocked, or unverified.

## Independent Review

Use an independent reviewer when concurrency or synchronization, worker/runtime
behavior, ownership or lifetime, storage or transaction semantics, architecture
boundaries, shared infrastructure, broad multi-subsystem behavior, significant
public or internal contracts, a complex refactor, merge/release readiness, or
an explicit user request makes independent verification materially useful.
Ordinary completion alone does not warrant delegation. Review the affected
final contracts. Use a whole-codebase audit only when that scope was requested.
Follow the [delegation policy](../../../AGENTS.md#delegation-policy).

The lead reconciles every material finding. Fix it within the authorized task
and verify the fix, reject it with supporting evidence, or disclose it as
unresolved or outside the authorized scope. If a later implementation change
invalidates a conclusion or verification result, revisit that part of the review
and the affected checks. Retain conclusions whose evidence still applies.

## Review Stance

For bounded and codebase reviews of C++ and QML, lead with findings ordered by
severity. For a confirmed correctness defect, identify the trigger, trace the
relevant execution path, cite supporting current source locations, name the
violated user requirement, explicit project policy, or demonstrable behavioral
contract, and explain the observable consequence. A sound static argument
suffices without a runtime reproducer.

If the causal chain is incomplete, report an open question or unverified risk.
Separate confirmed defects, unverified risks, and optional design improvements.
A conflict solely with descriptive architecture is a documentation inconsistency.
"No actionable findings" is a valid result. Recommend accept, revise, or block
only when reviewing an actual acceptance decision.

For documentation-only, workflow-only, or instruction-only changes without build
impact, do not compile, run tests, or prepare runtime.

Do not claim code works unless it was verified.

## Completion Report

For ordinary implementation work, report the following where applicable. Use
compact prose for small tasks and omit empty sections.

1. State which requested outcomes were delivered and the evidence for them.
2. Identify material changes and the affected files.
3. Describe verification actually performed and its results. Distinguish the presence of an implementation, inspected source, executed checks, and behavior those checks verified. A successful build alone cannot establish every requested outcome.
4. Summarize synchronization of directly affected documentation when relevant.
5. Disclose anything incomplete, blocked, or unverified, with the reason.
6. Give concrete manual actions and expected results only when needed. Keep suggested checks separate from work already performed.

The task is complete when the scoped implementation and required validation are
finished, with any genuine blocker or verification limitation stated accurately.
