# Routing And Boundary Tests

Use these cases only when changing `qml-code-research` selection or instructions. They validate routing decisions and observable boundaries, not exact wording.

## Should Use This Skill

| Request | Expected mode and boundary |
| --- | --- |
| "Map which pages are reachable from `Main.qml` and which are only registered." | Navigation and reachability research; distinguish source, registration, instantiation, and reachability without edits. |
| "Trace the Add Job button from its QML handler to the controller contract, read only." | Bounded event and QML-to-C++ contract trace; stop at the exposed controller unless deeper backend behavior is required. |
| "Which properties, signals, and model roles does `CvLibraryPage.qml` depend on?" | Bounded component-contract research with source-and-line evidence. |
| "Which QML files still need splitting?" | Architecture and decomposition mode; load `decomposition-analysis.md` and classify each recommendation by priority. |
| "Find business logic still owned by QML, but do not migrate it." | Ownership research; classify relevant behavior as UI-only, durable, or mixed and define candidate backend contracts. |
| "Create a QML implementation plan for a reusable jobs filter panel, but do not edit." | Implementation-planning mode; include reuse, component API, backend contract, registration, verification, risks, and stop before edits. |
| "Is this new page registered and reachable from the sidebar?" | Bounded registration and reachability research; inspect CMake and actual navigation separately. |

## Should Route Elsewhere

| Request | Expected route |
| --- | --- |
| "Implement the reusable jobs filter panel." | `qt-qml-ui-task`. |
| "Review my current QML diff for runtime and visual regressions." | `qml-code-review`. |
| "Move this QML validation and filtering into C++." | `qml-to-cpp-extraction`. |
| "Trace the worker thread and SQLite transaction behind Add Job." | `cpp-code-research`. |
| "Fix the missing QML module build error." | `cmake-build-debug`. |
| "Redesign every screen with a different visual style." | `qt-qml-ui-task`, after the requested design scope is established. |

## Validation Checklist

- Positive cases select this skill and the correct mode without broadening scope.
- Negative cases route to the more specific project skill.
- Narrow research does not load decomposition guidance or inventory the entire QML tree.
- Reports distinguish source existence, QML registration, instantiation, navigation reachability, build success, and manual runtime observation.
- QML-to-C++ traces distinguish bindings, direct calls, signal emissions, signal handlers, and backend notifications.
- Research performs no edits, builds, CTest runs, GUI launches, artifact writes, or subagent delegation unless separately authorized.
- Implementation planning stops before code changes and produces a handoff suitable for `qt-qml-ui-task` or `qml-to-cpp-extraction`.
- No routing or documentation references remain for the retired duplicate QML research skill.
