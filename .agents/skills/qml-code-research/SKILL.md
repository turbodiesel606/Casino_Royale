---
name: qml-code-research
description: Perform read-only, source-backed research of JobTracker QML before implementation. Use for bounded screen, component, navigation, or binding traces; QML-to-C++ contract mapping; UI-versus-durable ownership analysis; whole-QML decomposition; or implementation planning. Do not use for code changes, diff review, backend-only research, build debugging, or visual redesign.
---

# QML Code Research

Research the smallest relevant JobTracker QML surface and return evidence-backed findings without changing project files.

## Routing

- Use `qt-qml-ui-task` when the user asks to implement or modify QML, navigation, layout, resources, styling, or UI behavior.
- Use `qml-code-review` when the user asks to review a diff, changed QML, visual regressions, or release readiness.
- Use `qml-to-cpp-extraction` when the user authorizes moving durable QML behavior into C++.
- Use `cpp-code-research` when the requested behavior is backend-only or requires tracing beyond the QML-facing C++ contract.
- Use `cmake-build-debug` when the primary task is QML module configuration, compilation, deployment, CTest, or build-failure diagnosis.
- When changing this skill's routing or description, read [references/trigger-tests.md](references/trigger-tests.md) and exercise its cases. Do not load that file for ordinary QML research.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/qml-style.md`

Also read:

- `For-Agent/Docs/qml-to-cpp-extraction.md` when the research involves mock data, validation, filtering, sorting, search, grouping, parsing, storage, cross-screen state, or other durable behavior in QML.
- `For-Agent/Docs/build.md` when the user requests current build or runtime verification.

Before overlapping prior research, check the relevant files in `For-Agent/Research/`, `For-Agent/Review/`, and `For-Agent/Task-Report/`. Follow `For-Agent/Docs/artifacts.md` for recency and reuse. Treat artifacts as context only and reverify every current claim against the live instructions, source, worktree, and focused diff.

## Scope Gate

Before broad searching:

1. Classify the request as one or more of:
   - bounded screen, component, navigation, binding, or event-flow research;
   - QML-to-C++ contract research;
   - UI-only versus durable-behavior ownership research;
   - architecture or decomposition research;
   - implementation planning without edits.
2. State the included boundary and intentional omissions. Preserve a narrow screen, component, or direct-contract boundary without turning it into a whole-QML audit.
3. Run `git status --short --untracked-files=all`. Inspect focused diffs for in-scope files and account for pre-existing modified, deleted, or untracked work.
4. Start from the smallest relevant entry point. Inventory the complete QML surface only when the requested scope requires it.

## Entry Points

Start from the relevant QML surface:

- App shell and navigation: `qml/Main.qml`.
- Shared UI components: `qml/components/`.
- Pages and feature panes: `qml/pages/`.
- Backend exposure and QML-facing contracts: `src/app/` plus the related controller or model in `src/`.
- QML file registration: `CMakeLists.txt`.

Use targeted `rg` searches for component definitions and instantiations, imports, `property`, `required property`, aliases, signals, functions, handlers, `Connections`, bindings, model roles, context-property names, inline arrays, repeated local structures, and QML registration before broader scans.

## Evidence Workflow

1. Inspect each relevant QML definition and the call site or parent that instantiates it before inferring ownership or reachability.
2. Trace the requested direction explicitly:
   - user event to QML handler, signal, or backend command;
   - backend property, signal, or model role to QML binding, `Connections` handler, or delegate;
   - shell navigation to the page or form surface it can actually instantiate.
3. Verify related QML files in `qt_add_qml_module`; source existence alone is not registration evidence.
4. Verify actual instantiation and navigation reachability separately from registration. A registered page may still be unreachable.
5. Cite the file and line for every non-trivial claim, risk, missing contract, and implementation recommendation.
6. Distinguish these evidence levels in the output:
   - live source inspection;
   - current worktree or diff inspection;
   - QML module registration inspection;
   - static instantiation and reachability inspection;
   - related test-source inspection;
   - successful build;
   - manual runtime or visual observation.
7. If no build or GUI run occurred, label the result static-only and do not imply runtime or visual verification.
8. Record unresolved questions, conflicting evidence, and intentionally uninspected surfaces.

## QML Ownership And Backend Contract Checks

When the scope touches a QML component or QML-facing C++ object:

- Identify the component owner, creation point, lifetime, input properties, output signals, local state, and model dependencies.
- Distinguish declarative bindings, imperative JavaScript calls, QML signal emission, signal handlers, and C++ invocations. A signal notification is not a direct function call.
- Verify context-property or registered-type exposure and the consumed C++ `Q_PROPERTY`, notify signal, invokable or slot, result signal, and model role names.
- Check that the QML contract handles relevant empty, unavailable, loading, error, completion, or cancellation states.
- Keep transient presentation state in QML: navigation selection, focus, hover, expansion, popup visibility, animation, layout geometry, and display-only formatting.
- Classify validation, storage, parsing, filtering, sorting, search, grouping, cross-screen state, reusable data models, and other testable product rules as durable behavior owned by C++.
- For mixed behavior, identify the UI intent and rendering that stay in QML and the durable rule or state that belongs behind the QML-facing contract.
- Stop at the exposed controller or model boundary unless deeper backend behavior is necessary to answer the request; hand backend-only research to `cpp-code-research`.

## Architecture And Decomposition Mode

For whole-QML architecture, file-splitting, component-boundary, duplication, or maintainability research, read and follow [references/decomposition-analysis.md](references/decomposition-analysis.md). Do not load or emit its complete decomposition deliverable for a narrow screen, event-flow, or contract task.

## Research Output

Always return:

- Requested scope, stopping boundary, files inspected, and relevant worktree state.
- Current page or component ownership and the requested navigation, event, binding, or data flow.
- Source references for non-trivial claims.
- UI-only state, durable-behavior ownership, QML-to-C++ contracts, CMake registration, and reachability when applicable.
- Relevant risks, missing coverage, open questions, and intentional omissions.
- Suggested implementation direction without changing code.
- A verification matrix separating static source, diff, registration, reachability, test-source, build, and manual-runtime evidence.

Add only the sections required by the selected mode:

- For architecture or decomposition research, include the deliverable defined in `references/decomposition-analysis.md`.
- For QML-to-C++ ownership research, classify each relevant function, inline array, state mutation, or computed value as UI-only, durable, or mixed and define the smallest QML-facing contract needed after extraction.
- For implementation planning, include:
  - existing pages, components, controllers, models, and contracts to reuse or extend;
  - files and QML or C++ symbols expected to change;
  - proposed component ownership, properties, required properties, signals, model roles, and state boundaries;
  - QML module registration and backend-wiring impact;
  - required C++ tests, documentation impact, build verification, and manual UI checks;
  - ordered steps, risks, dependencies, and intentional omissions.

Stop before edits after an implementation-planning request. Hand the verified research to `qt-qml-ui-task` or `qml-to-cpp-extraction` only when the user authorizes implementation.

Create a durable research artifact only when the user explicitly requests saved or durable output. Otherwise return findings in chat without writing files. When authorized, follow `For-Agent/Docs/artifacts.md` and save under `For-Agent/Research/`.

If a read-only subagent is used at the user's request, treat its output as research input. The lead Codex remains responsible for checking the live source, compiling the final findings, and writing any authorized artifact.

## Boundaries

- Do not edit QML, C++, CMake, tests, or documentation during research.
- Do not write a research artifact unless the user explicitly authorizes durable output.
- Do not recommend a global visual redesign unless the user asks for it.
- Do not treat presentation-only behavior as a backend migration candidate.
- Do not propose a split or new component from line count alone; inspect responsibilities, ownership, APIs, repetition, and existing reusable components first.
- Do not run builds, CTest, or the GUI unless the user requests verification or the research genuinely depends on current execution state.
- Do not use subagents unless the user requests subagents, parallel research, or an independent pass.
