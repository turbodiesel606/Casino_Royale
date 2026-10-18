# Routing And Boundary Tests

These are optional workflow-maintenance cases, not inputs or commands for ordinary
tasks. Use them to check routing and observable boundaries, not exact wording.
Static consistency review is preparation, not an empirical performance result.

## Should Use This Skill

| Request | Workflow and initial scope | Required evidence or output | Expand only when |
| --- | --- | --- | --- |
| "Briefly explain this binding or Qt signal handler." | `qml-code-research`; named binding/handler and consumed property/signal. | Direct answer, source anchors, material limits; no mandatory matrix. | The immediate backend contract or instantiator is needed. |
| "Map pages reachable from `Main.qml` versus registered pages." | Navigation research; shell and QML registration. | Distinguish existence, registration, instantiation, and reachability. | A navigation/creation path requires another component. |
| "Trace the Add Job button to its controller contract, read only." | Bounded event trace; handler. | Source-backed signal/command flow; stop at exposed controller. | Deeper backend behavior is necessary to answer. |
| "Which properties/signals/roles does `CvLibraryPage.qml` consume?" | Bounded contract research; named page. | Source anchors and affected backend contracts; no full navigation inventory. | A delegated component consumes part of the contract. |
| "Should this one component be split?" | Local decomposition; target plus direct instantiators/consumers. | Properties, signals, bindings, state/ownership, registration, reuse, visual/interaction invariants, justified recommendation. | A concrete dependency or risk crosses that boundary; no complete `Main.qml` map prerequisite. |
| "Analyze decomposition of the whole QML tree." | Whole-QML decomposition; complete inventory and shell map. | Registration, navigation, cross-component candidates and priorities from `decomposition-analysis.md`. | The requested whole surface and its consumed contracts require it. |
| "Find business logic still owned by QML, without migration." | Ownership research; requested surface, whole QML only if requested. | UI-only/durable/mixed classification and candidate C++ contract. | A state owner or consumer determines responsibility. |
| "Plan a reusable jobs filter panel, without edits." | Implementation planning; current page/filter and reusable controls. | Reuse, API, backend contract, registration, verification, risks; stop before edits. | A proposed dependency or contract requires inspection. |
| "Is this page registered and reachable from the sidebar?" | Bounded registration/reachability research; named page and route. | CMake registration and actual creation/navigation evidence separately. | A loader or parent determines reachability. |

## Should Route Elsewhere

| Request | Workflow and initial scope | Required evidence or output | Expand only when |
| --- | --- | --- | --- |
| "Implement the reusable jobs filter panel." | `qt-qml-ui-task`; target page/control. | Scoped implementation, registration, build and relevant interaction checks. | Durable rules require C++ or shared contracts are affected. |
| "Change this button's spacing and selected appearance." | `qt-qml-ui-task`; button and direct uses. | Inspect affected visuals/states in a suitable available environment; otherwise concrete manual checks and explicit limits. | Shared styling or interaction risks require more coverage; no automatic full-app tour or ceremonial test. |
| "Review this QML diff for regressions." | `qml-code-review`; focused diff. | Findings, source evidence, runtime/visual limits; no automatic saved report. | Direct consumers or concrete risks demand it. |
| "Review this existing component; no diff is available." | `qml-code-review`; named component. | Current properties/bindings/handlers/contracts and actionable findings or no findings with limits. | Instantiators, registration, or a backend contract affect judgment. |
| "Review the supplied QML component and its backend interface." | `qml-code-review`; supplied component and exposed backend contract. | Actionable findings or no actionable findings with source evidence and limits. | Bindings, handlers, or exposure affect judgment; no source edits. |
| "Audit the whole QML surface for architecture drift and repeated responsibilities." | `qml-codebase-review`; complete QML inventory and consumed contracts. | Broad navigation, ownership, and duplication assessment with source evidence and verification gaps. | Cross-component and backend contracts require it. |
| "Move this QML validation/filtering into C++." | `qml-to-cpp-extraction`; current owner and exposed contract. | Production ownership, behavior preservation, meaningful tests, build and affected UI evidence. | Shared state/storage or cross-screen contracts require it. |
| "Trace the worker thread and SQLite transaction behind Add Job." | `cpp-code-research`; exposed command then worker/service. | Source-backed affinity/ownership/transaction flow. | Relevant lifecycle and failure paths require it. |
| "Fix the missing QML module build error." | `cmake-build-debug`; exact error and registration. | Configuration/build diagnosis, appropriate execution and result reporting. | Error evidence requires imports, Qt kit, or runtime preparation. |
| "Redesign every screen with a different visual style." | `qt-qml-ui-task`; requested global UI scope. | Broad visual design implementation and proportionate interaction/visual verification. | The explicitly broad request requires shared components and all affected screens. |

## Validation Checklist

- Positive cases select this skill and the correct mode without broadening scope.
- Negative cases route to the more specific project skill.
- Narrow event/contract research need not load decomposition guidance. Local
  decomposition uses its local section without inventorying the entire QML tree.
- Reports distinguish source existence, QML registration, instantiation, navigation reachability, build success, and manual runtime observation.
- QML-to-C++ traces distinguish bindings, direct calls, signal emissions, signal handlers, and backend notifications.
- Research does not edit project code or documentation as implementation work;
  implementation requests route to the appropriate workflow. Runtime verification
  follows the research skill's [Boundaries](../SKILL.md#boundaries) and active
  environment permissions.
- Implementation planning stops before code changes and produces a handoff suitable for `qt-qml-ui-task` or `qml-to-cpp-extraction`.
- No routing or documentation references remain for the retired duplicate QML research skill.
- Existing-component review does not require a diff or route to a whole-QML audit.

## Optional Review Quality Cases

The routing cases above check workflow selection, not independent defect
detection. Use the shared synthetic C++ and QML/backend
[fixtures and evaluator rubric](../../cpp-code-research/references/review-quality-cases.md)
for review-quality evaluation, including correct controls. Pass only the neutral
reviewer inputs identified there; keep evaluator expectations separate.

## Optional Before/After Comparison

Use the [shared comparison method](../../cpp-code-research/references/trigger-tests.md#optional-beforeafter-comparison):
comparable baselines/environments, identical model and reasoning effort,
independent fresh sessions, quality and human corrections before time/unnecessary
work/available usage including delegation. State missing metrics and separate
measurements from assumptions. Maintaining these cases requires static inspection
only; it does not require GUI launches, benchmarks, evaluation infrastructure, or
empirical comparison runs.
