---
name: cpp-code-research
description: Perform read-only, source-backed research of the JobTracker C++ backend before implementation. Use for bounded symbol or workflow tracing, Qt ownership and threading, QML contracts, storage, CMake and test mapping, implementation planning, or architecture and duplication analysis. Do not use for code changes, diff review, build debugging, or progressive teaching.
---

# C++ Code Research

Research the smallest relevant JobTracker C++ surface and return evidence-backed findings without changing production code.

## Routing

- Use `cpp-backend-task` when the user asks to implement or modify C++.
- Use `cpp-code-review` when the user asks to review a diff, changed code, or release readiness.
- Use `cmake-build-debug` when the primary task is configuration, compilation, CTest, or build-failure diagnosis.
- Use `learn-cpp-codebase` for a progressive teaching tour of the complete backend.
- Use `qml-code-research` for QML structure or visual ownership; inspect QML here only to verify a C++ contract boundary.
- When changing this skill's routing or description, read [references/trigger-tests.md](references/trigger-tests.md) and exercise its cases. Do not load that file for ordinary backend research.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/coding-style.md`
4. `For-Agent/Docs/testing.md`

Also read:

- `For-Agent/Docs/qml-to-cpp-extraction.md` when the research involves QML-facing controllers, models, or durable behavior being moved out of QML.
- `For-Agent/Docs/build.md` when the user requests current build or test verification.

## Scope Gate

Before broad searching:

1. Classify the request as one or more of:
   - bounded symbol or direct-call research;
   - end-to-end backend workflow research;
   - architecture, maintainability, or duplication research;
   - implementation planning without edits.
2. State the included boundary and intentional omissions. Honor a direct-call-only request without recursively tracing nested callees.
3. Run `git status --short --untracked-files=all`. Inspect focused diffs for in-scope files and account for pre-existing modified, deleted, or untracked work.
4. Start from the smallest relevant entry point. Do not inventory the whole repository unless the requested scope requires it.

## Entry Points

- Startup and dependency wiring: `src/main.cpp`, `src/app/`.
- Shared backend utilities: `src/common/`.
- Storage, SQL helpers, transactions, and schema migrations: `src/storage/`.
- Jobs and Add Job: `src/jobs/`.
- CVs, managed files, and Add CV: `src/cvs/`.
- Dashboard read models: `src/dashboard/`.
- Company and contact domains: `src/directory/`.
- Matching tests and reusable fixtures: `tests/`, especially the relevant domain folder and `tests/support/`.
- Production and test registration: `CMakeLists.txt`.
- C++ exposure and consumption boundary, only when relevant: `qml/`.

Use targeted `rg` searches for definitions, callers, constructors, connections, context properties, `Q_PROPERTY`, `Q_INVOKABLE`, model roles, QML handlers, tests, and CMake registration before broader scans.

## Evidence Workflow

1. Inspect each relevant project-defined implementation before inferring ownership or behavior.
2. Trace the requested direction explicitly: caller to callee, signal sender to connected receiver, QML command to backend, or storage operation back to model publication.
3. Verify that participating production and test files are registered in `CMakeLists.txt`; file existence alone is not registration evidence.
4. Cite the file and line for every non-trivial claim, risk, missing contract, and implementation recommendation.
5. Distinguish these evidence levels in the output:
   - live source inspection;
   - current worktree or diff inspection;
   - CMake registration inspection;
   - test-source inspection;
   - successful build;
   - CTest result;
   - manual runtime observation.
6. If no build, CTest, or GUI run occurred, label the result static-only and do not imply runtime verification.
7. Record unresolved questions, conflicting evidence, and intentionally uninspected surfaces.

## Qt, QML, And Concurrency Checks

When the scope touches QObjects, controllers, models, workers, asynchronous flows, or Qt SQL:

- Identify each QObject owner, lifetime, and thread affinity. Moving a value does not move a QObject or change its affinity.
- Distinguish direct function calls from signal notification. Identify the connected receiver, QML handler, and connection type when they affect behavior.
- Verify that QML-facing controllers and model mutations stay on the GUI thread and that model begin/end and notify contracts match the behavior being researched.
- Trace facade and executor responsibilities, queued value delivery, active and waiting work, cancellation identity, late-result rejection, shutdown order, and object destruction.
- Identify the thread where every `QSqlDatabase` connection and active query is created, used, closed, and removed.
- Trace transaction ownership and any filesystem staging, cleanup, rollback compensation, or crash-recovery limitation.
- Verify QML exposure in bootstrap, actual QML consumption, stable model roles, properties, invokables, signals, and handlers without broadening into visual review.

## Persistence And Test Checks

When storage is in scope:

- Trace schema initialization and every supported migration path that can reach the affected state.
- Verify repository query and domain-value mapping in both read and write directions.
- Identify the service or operation that owns transaction boundaries.
- Inspect matching storage tests and `tests/support/` fixtures instead of inferring coverage from suite names.

When tests are in scope, map each relevant invariant to a concrete test function or identify the exact missing coverage. Keep test-source inspection separate from an executed CTest result.

## Architecture And Duplication Mode

For architecture, maintainability, duplication, or overlapping-responsibility research, read and follow [references/duplication-analysis.md](references/duplication-analysis.md). Do not load or emit its full duplication deliverable for a narrow symbol or workflow task unless duplication becomes necessary to answer the request.

## Research Output

Always return:

- Requested scope, stopping boundary, and files inspected.
- Current ownership and data flow relevant to that scope.
- Source references for non-trivial claims.
- QML-facing, Qt model, threading, storage, test, and CMake contracts when applicable.
- Relevant risks, missing coverage, open questions, and intentional omissions.
- Suggested implementation direction without changing code.
- Verification matrix separating static inspection, build, CTest, and manual runtime evidence.

Add only the sections required by the selected mode:

- For architecture or duplication research, include the deliverable defined in `references/duplication-analysis.md`.
- For implementation planning, include:
  - the existing entity to reuse or extend;
  - files and symbols expected to change;
  - ownership, lifetime, public and QML contracts, error behavior, threading, storage, and transaction decisions;
  - required tests, CMake registration, and documentation impact;
  - later build, CTest, and manual verification;
  - ordered steps, risks, dependencies, and intentional omissions.

Stop before edits after an implementation-planning request. Hand the verified research to `cpp-backend-task` only when the user authorizes implementation.

If a read-only subagent is used at the user's request, treat its output as research input. The lead Codex remains responsible for checking the live source and compiling the final findings.

## Boundaries

- Do not edit production code, CMake, tests, QML, or documentation during research.
- Do not broaden into QML visual review unless the C++ behavior depends on the QML contract.
- Do not run builds, CTest, or the GUI unless the user requests verification or the research genuinely depends on current execution state.
- Do not recommend a new abstraction until the relevant existing implementation has been inspected and found insufficient.
- Do not treat similar syntax, names, or fields as shared responsibility without matching invariants and reasons to change.
