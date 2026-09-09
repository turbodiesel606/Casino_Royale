# Routing And Boundary Tests

Use these cases only when changing `cpp-code-research` selection or instructions. They validate routing decisions and observable boundaries, not exact wording.

## Should Use This Skill

| Request | Expected mode and boundary |
| --- | --- |
| "Trace Add Job from `createApplication()` through SQLite, read only." | End-to-end workflow; include QML/controller/worker/service/repository/storage/model publication and stop without edits. |
| "Which direct project functions does `AddJobWorker::Executor::process()` call? Do not recurse." | Bounded direct-call research; list only direct project-defined callees and distinguish Qt/standard-library boundaries. |
| "Where is the CV import SQL connection created and destroyed?" | Bounded Qt/threading/storage research; prove object ownership, affinity, and connection lifecycle. |
| "Find duplicate validation responsibilities in the C++ backend." | Architecture and duplication mode; load `duplication-analysis.md`. |
| "Create a C++ implementation plan for contact persistence, but do not edit." | Implementation-planning mode; include reuse, contracts, files, tests, CMake, docs, verification, and stop before edits. |
| "Map `SchemaMigrator` production registration and matching tests." | Bounded storage/CMake/test research; distinguish registration, test source, and executed-test evidence. |

## Should Route Elsewhere

| Request | Expected route |
| --- | --- |
| "Implement contact persistence." | `cpp-backend-task`. |
| "Review my current C++ diff for regressions." | `cpp-code-review`. |
| "Fix this CMake configure failure." | `cmake-build-debug`. |
| "Teach me the entire backend from scratch." | `learn-cpp-codebase`. |
| "Review the visual spacing and component hierarchy of this QML page." | QML research or review skill, depending whether changes are being reviewed. |
| "Use review-code-for-human to explain Add Job completely." | `review-code-for-human`, because it was explicitly requested. |

## Validation Checklist

- Positive cases select the skill and the correct mode without broadening scope.
- Negative cases route to the more specific project skill.
- Direct-call requests do not recursively trace nested callees.
- Ordinary research does not load duplication guidance or trigger tests.
- Research performs no edits, builds, CTest runs, or GUI launches unless separately authorized.
- Implementation planning stops before code changes and produces a handoff suitable for `cpp-backend-task`.
