---
name: qml-code-research
description: Answer JobTracker QML questions through read-only, source-backed research. Use for bounded screen, component, navigation, or binding traces; QML-to-C++ contract mapping; UI-versus-durable ownership; local component or whole-QML decomposition; or implementation planning. Do not use for implementation, code review, backend-only research, build debugging, or visual redesign.
---

# QML Code Research

Research the smallest relevant JobTracker QML surface and return evidence-backed findings without changing project files.

## Routing

- Use `qt-qml-ui-task` when the user asks to implement or modify QML, navigation, layout, resources, styling, or UI behavior.
- Use `qml-code-review` for a focused diff or review of an explicitly identified existing component or behavior, even without a diff. Use `qml-codebase-review` for a whole-QML audit.
- Use `qml-to-cpp-extraction` when the user authorizes moving durable QML behavior into C++.
- Use `cpp-code-research` when the requested behavior is backend-only or requires tracing beyond the QML-facing C++ contract.
- Use `cmake-build-debug` when the primary task is QML module configuration, compilation, deployment, CTest, or build-failure diagnosis.
- [references/trigger-tests.md](references/trigger-tests.md) contains optional workflow-maintenance scenarios and a comparison method. Consult it when validating routing changes; ordinary tasks must not load or execute it automatically.

## Context Selection

1. Apply `AGENTS.md`, then identify the affected screen, component, navigation
   surface, or QML-facing backend contract.
2. Select relevant sections of `For-Agent/Docs/qml-style.md` from the actual
   question, property, and its consumers: **Logic Boundaries** for ownership and
   binding contracts; **Navigation And Structure** for navigation or decomposition;
   **Components** for component reuse and shared controls; **Button Interaction
   Palette** for button states; **Selection Controls** for checkbox/selection
   states; **Verification** when assessing runtime or visual evidence. A binding
   that controls a button or checkbox still requires the applicable component,
   palette, and selection rules. Follow consumers before narrowing the selection;
   retain broader context, including the complete guide if needed, when ambiguous.
3. Read only the matching documents under `For-Agent/Docs/architecture/` when
   the research reaches a QML/backend contract or another architecture boundary.
   Start with `qml-contracts.md` for model/property/signal contracts and add a
   domain document only when the traced behavior enters that domain. Use the
   overview only when the correct document is unclear.
4. Read `For-Agent/Docs/qml-to-cpp-extraction.md` only when the research involves
   mock data, validation, filtering, sorting, search, grouping, parsing, storage,
   cross-screen state, or other durable behavior in QML.
5. Read `For-Agent/Docs/testing.md` only when test strategy, concrete coverage,
   test registration, or a high-risk backend contract is part of the research.
6. Read `For-Agent/Docs/build.md` only when the user requests current build or
   runtime verification or the research concerns QML/CMake registration.

Expand context only when the inspected binding, handler, model role, C++ contract,
registration, or runtime concern crosses the current boundary. Do not preload the
complete architecture, build, and testing documentation.

## Scope Gate

Before broad searching:

1. Classify the request as one or more of:
   - bounded screen, component, navigation, binding, or event-flow research;
   - QML-to-C++ contract research;
   - UI-only versus durable-behavior ownership research;
   - architecture or decomposition research;
   - implementation planning without edits.
2. State the included boundary and intentional omissions. Preserve a narrow screen, component, or direct-contract boundary without turning it into a whole-QML audit.
3. When Git metadata exists, inspect `git status --short --untracked-files=all` and focused diffs for in-scope files. Otherwise inspect the files directly. Account for pre-existing work.
4. Start from the smallest relevant entry point. Inventory the complete QML surface only when the requested scope requires it.

## Search And Stopping Policy

- Prefer exact component, property, signal, handler, model-role, context-property,
  import, and QML-registration searches over directory-wide reading.
- Inspect the target component first, then its direct instantiators, handlers,
  bindings, and exposed backend contract. Follow only the edges needed to answer
  the request or resolve a concrete navigation, ownership, reachability,
  durable-state, backend-contract, build, or test risk.
- Stop when the material conclusion is grounded in current QML and the relevant
  direct contracts. Do not continue exploring solely to narrate routine facts or
  attach lines to context already established in the active task.
- Do not repeat discovery already completed by the lead. A second pass over the
  same scope is appropriate only when independent research was requested or a
  disputed or high-risk claim materially needs it.
- If prior research or review artifacts may help, inventory names and metadata
  first. Read only the newest artifact whose scope overlaps the question, plus a
  newer narrow artifact when it materially updates that scope. Artifacts are
  context aids; verify conclusions against live source.

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
3. Verify related QML files in `qt_add_qml_module` when registration affects the question or a proposed split; source existence alone is not registration evidence.
4. Verify actual instantiation and navigation reachability when relevant, separately from registration. A registered page may still be unreachable. Do not build a global navigation map merely to answer a local binding question.
5. Cite the file and line for every non-trivial claim, risk, missing contract, and implementation recommendation.
6. Distinguish relevant evidence levels when they affect the conclusion:
   - live source inspection;
   - current file inspection and any available worktree/diff evidence;
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

For a decomposition, component-boundary, duplication, or maintainability request,
read [references/decomposition-analysis.md](references/decomposition-analysis.md)
and select its local or whole-QML scope. A local question starts at the named
component, direct instantiators/consumers, and affected contracts; inspect its
registration, reuse opportunities, interaction, and visual invariants. Expand
only for concrete dependencies or risks. Whole-QML requests retain the complete
inventory, navigation mapping, and cross-component analysis. A bounded event-flow
or contract question that does not concern decomposition need not load this reference.

## Research Output

Match the output to the requested result. For a narrow factual question, normally
return the answer, supporting file/symbol/line locations, and material uncertainty
or evidence limitations. Do not inspect unrelated registration, navigation, or
architecture solely to fill a template.

For a trace or broader investigation, include relevant scope, ownership, flow,
contracts, risks, and missing coverage. Include implementation direction when a
recommendation, plan, or options were requested, or when necessary to answer the
question. Use a verification matrix when multiple relevant evidence types need
to be distinguished; a short static-only statement is enough for a directly
verifiable source question. Verify material claims regardless of output length.

Keep findings compact. Quote or reproduce only the smallest QML or C++ fragments
needed to make a conclusion understandable; use file, symbol, and line references
instead of copying large implementation bodies.

Add only the sections required by the selected mode:

- For architecture or decomposition research, use the local or whole-QML deliverable defined in `references/decomposition-analysis.md`.
- For QML-to-C++ ownership research, classify each relevant function, inline array, state mutation, or computed value as UI-only, durable, or mixed and define the smallest QML-facing contract needed after extraction.
- For implementation planning, include:
  - existing pages, components, controllers, models, and contracts to reuse or extend;
  - files and QML or C++ symbols expected to change;
  - proposed component ownership, properties, required properties, signals, model roles, and state boundaries;
  - QML module registration and backend-wiring impact;
  - required C++ tests, documentation impact, build verification, and manual UI checks;
  - ordered steps, risks, dependencies, and intentional omissions.

Stop before edits after an implementation-planning request. Hand the verified research to `qt-qml-ui-task` or `qml-to-cpp-extraction` only when the user authorizes implementation.

## Boundaries

- Do not edit QML, C++, CMake, tests, or documentation during research.
- Do not recommend a global visual redesign unless the user asks for it.
- Do not treat presentation-only behavior as a backend migration candidate.
- Do not propose a split or new component from line count alone; inspect responsibilities, ownership, APIs, repetition, and existing reusable components first.
- Do not run builds, CTest, or the GUI unless the user requests verification or the research genuinely depends on current execution state.
