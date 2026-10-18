# Routing And Boundary Tests

These are optional workflow-maintenance cases, not inputs or commands for ordinary
tasks. Use them to check routing and observable boundaries, not exact wording.
Static consistency review is preparation, not an empirical performance result.

## Should Use This Skill

| Request | Workflow and initial scope | Required evidence or output | Expand only when |
| --- | --- | --- | --- |
| "Briefly explain this function / this Qt notify mechanism." | `cpp-code-research`; named function or project usage. | Direct answer, source anchors, material limits; no mandatory matrix. | A caller, receiver, or lifetime contract is needed to explain it. |
| "Trace Add Job from `createApplication()` through SQLite, read only." | End-to-end research; controller entry. | QML/controller/worker/service/repository/storage/publication trace with source evidence; stop before edits. | The requested path crosses another contract or failure branch. |
| "Which direct project functions does `JobSaveExecutor::process()` call? Do not recurse." | Bounded direct-call research; named method. | Direct project callees, Qt/standard-library boundary, static limits. | Clarification of a direct call is necessary; do not recursively trace callees. |
| "Where is the CV import SQL connection created and destroyed?" | Qt/threading/storage research; import worker context. | Source-backed ownership, affinity, connection lifecycle. | A lifecycle edge or shared runtime owns part of the answer. |
| "Find duplicate validation responsibilities in the C++ backend." | Architecture/duplication research; relevant validation owners. | `duplication-analysis.md` deliverable, matching invariants and drift risks. | Candidate duplication crosses domains; verify every reported instance. |
| "Plan contact persistence, without edits." | Implementation planning; directory controller/service/storage boundary. | Reuse, contracts, files, tests, registration, docs, verification plan; no edits. | The proposed responsibility requires another dependency. |
| "Map `SchemaMigrator` registration and tests." | Storage/CMake/test research; migrator and registration. | Distinct registration, test-source, and executed-test evidence. | A supported migration or fixture is needed to assess coverage. |

## Should Route Elsewhere

| Request | Workflow and initial scope | Required evidence or output | Expand only when |
| --- | --- | --- | --- |
| "Implement contact persistence." | `cpp-backend-task`; directory production contract. | Scoped implementation, meaningful tests, build and final review. | Dependencies or verification risks demand it. |
| "Fix this validation bug." | `cpp-backend-task`; failing input and validator. | Observable regression coverage; before/after evidence when feasible and proportionate. | Shared validation or a boundary case is affected. |
| "Change worker cancellation and storage rollback." | `cpp-backend-task`; affected worker/service/runtime. | Ownership/order/transaction analysis, meaningful failure tests, broader relevant verification and independent review when warranted. | Shared lifecycle or storage semantics require coverage; do not stop at one mapped test. |
| "Review my current C++ diff for regressions." | `cpp-code-review`; focused diff. | Severity-ordered findings, source evidence, residual risk; no automatic saved report. | Direct contracts or a concrete finding require more code. |
| "Review this existing function; there is no diff." | `cpp-code-review`; named function. | Inspect its current contract and dependencies; actionable findings or no findings with limits. | A caller/owner/invariant is needed; no manufactured diff or global audit. |
| "Review the supplied C++ setter and observer under their stated contract." | `cpp-code-review`; supplied code and contract. | Actionable findings or no actionable findings with source evidence and limits. | Notification/receiver semantics need inspection; no production mutation. |
| "Audit the whole C++ backend for architecture drift and responsibility overlap." | `cpp-codebase-review`; complete backend inventory. | Broad coverage, source-backed findings, justified similarities, verification gaps. | Cross-module ownership and contracts require it. |
| "Fix this CMake configure failure." | `cmake-build-debug`; exact command, preset, error. | Diagnose configuration separately from build/tests; preserve execution policy. | Error evidence points to toolchain, registration, or dependency. |
| "Review spacing and hierarchy of this existing QML page." | `qml-code-review`; target page even without a diff. | Source-backed interaction/layout risks and visual-verification limits. | Direct components or backend contracts matter. |
| "Use review-code-for-human to explain Add Job completely." | Explicit `review-code-for-human`; requested workflow. | Preserve its detailed specialized deliverable. | Its workflow requires connected code; no implicit invocation elsewhere. |

## Validation Checklist

- Positive cases select the skill and the correct mode without broadening scope.
- Negative cases route to the more specific project skill.
- Direct-call requests do not recursively trace nested callees.
- Ordinary research does not load duplication guidance or trigger tests.
- Research does not edit project code or documentation as implementation work;
  implementation requests route to the appropriate workflow. Runtime verification
  follows the research skill's [Boundaries](../SKILL.md#boundaries) and active
  environment permissions.
- Implementation planning stops before code changes and produces a handoff suitable for `cpp-backend-task`.
- Explanations retain the requested depth; bounded reviews without a diff and
  explicitly requested broad audits retain their respective scope.

## Optional Review Quality Cases

The routing cases above check workflow selection, not independent defect
detection. For review-quality evaluation, use the synthetic C++ and QML/backend
[fixtures and evaluator rubric](review-quality-cases.md), including correct
controls. Pass only the neutral reviewer inputs identified there to the reviewer;
keep evaluator expectations separate.

## Optional Before/After Comparison

1. Use comparable repository baselines and environments, changing only the
   instruction version under comparison. Record unavoidable differences.
2. Keep model and reasoning effort the same. Run independent fresh sessions for
   each version/case so old instructions or conclusions do not carry over.
3. Assess correctness, completeness, regressions, verification quality, and
   required human corrections first, using the expected boundaries above and
   the fixture rubric when evaluating review quality.
4. Then compare elapsed time, unnecessary work, and available usage data,
   including delegated work. State when a usage metric is unavailable.
5. Separate measured results from assumptions. Static scenario validation alone
   establishes neither improvement, percentage savings, nor optimality.

Run this comparison only when requested as evaluation work. Maintaining these
cases requires static inspection only; it does not require benchmarks,
evaluation infrastructure, or empirical comparison runs.
