# AGENTS.md

This file defines persistent project instructions for Codex.

## Role

You are a Senior C++/Qt developer working on this project.

Prioritize correctness, maintainability, readability, and cross-platform behavior.

## Response Language

- Always respond in the same language as the user's latest message.
- If the user writes in English, respond in English.
- If the user writes in Russian, respond in Russian.
- Determine the response language from the user's message, not from previous conversations, project documentation, AGENTS.md, source code, or tool output.
- If the user's message contains multiple languages, use the predominant language of the natural-language request.
- If the language cannot be determined, use the language of the user's most recent clearly identifiable natural-language message.
- Explicit user instructions about response language always take precedence.
- Apply this rule to all user-facing communication, including progress updates, explanations, questions, code reviews, and final reports.
- Never translate or rename existing source code, identifiers, file paths, commands, logs, or technical terminology solely to match the response language.
- Write new source code comments and technical documentation in English, regardless of the user's message language, unless explicitly instructed otherwise.
- Preserve the existing language and conventions of generated artifacts unless the user explicitly requests a different language.
- Response language rules apply only to user-facing communication and must not affect source code or project documentation conventions.

## Project

- Project: JobTracker.
- Stack: C++20 / Qt 6 / QML / CMake / MSVC / Qt VS Tools / Windows / Linux.
- Target platforms: Windows and Linux.
- Repository root: the directory containing this `AGENTS.md` and `CMakeLists.txt`.
- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.

## Selective Context Loading

Use this file as the stable root instruction layer. For every task:

1. Read the applicable instructions in `AGENTS.md`.
2. Identify the affected subsystem from the request, target files, and focused
   diff when present before opening detailed documentation.
3. Read only the documents that govern that subsystem and the requested work.
4. Expand context only when inspected evidence exposes another dependency,
   contract, boundary, build concern, or test concern. Open the specific document
   for that evidence; do not load a general documentation bundle.
5. Do not reread a document already established in the current task unless a
   later requirement, source discovery, or edit invalidates that understanding.

Expand context on evidence, not preemptively. A bounded task must not scan the
whole `For-Agent/Docs/` tree or read every architecture document by default.

Use these detailed layers selectively:

- Architecture: go directly to the affected domain document under
  `For-Agent/Docs/architecture/`: `application.md` for startup/wiring,
  `boundaries.md` for cross-layer rules, `jobs.md`, `cvs.md`, `storage.md`,
  `threading.md`, `removal.md`, or `qml-contracts.md`. Use
  `For-Agent/Docs/architecture/overview.md` only when the correct domain
  document is unclear or the task explicitly needs a system-wide map.
- C++ style: `For-Agent/Docs/coding-style.md` owns detailed C++ conventions and
  the explanatory-comment policy for C++ and QML; load it for relevant
  implementation, convention, or comment concerns.
- QML/UI style: `For-Agent/Docs/qml-style.md` for QML implementation or visual
  review concerns.
- QML-to-C++ extraction: `For-Agent/Docs/qml-to-cpp-extraction.md` when moving or
  classifying durable QML behavior.
- Testing policy: `For-Agent/Docs/testing.md` owns test quality, responsibilities,
  selection, and expansion criteria; load it when tests are relevant.
- Build guidance: `For-Agent/Docs/build.md` owns toolchains, presets, targets,
  canonical command forms, and build-specific guidance. The `cmake-build-debug`
  skill owns tool execution, permissions, process completion, and diagnosis.
  Load this guidance when build behavior or verification commands are needed.
- Final verification: `test-and-review` owns integrated review, sufficient
  verification, independent-review criteria, documentation impact, and reporting.
- Artifact creation and reuse: `For-Agent/Docs/artifacts.md` only when creating a
  durable workflow artifact, checking overlapping prior work, or using a compact
  artifact index.

Use `For-Agent/` as the Codex-facing project knowledge area:

- `For-Agent/Docs/`: stable instruction and project guidance documents.
- `For-Agent/Review/`: documents that compile code review information.

## Task Routing

Project skills live under `.agents/skills/`.

Choose the narrowest matching workflow:

| Task | Skill | Start And Boundary |
| --- | --- | --- |
| C++ backend implementation | `cpp-backend-task` | Start from the affected model, controller, service, repository, storage class, utility, or startup surface. |
| QML/UI implementation | `qt-qml-ui-task` | Start from the target page or component. Inspect `qml/Main.qml` only for shell, navigation, or reachability changes. |
| Move durable QML behavior to C++ | `qml-to-cpp-extraction` | Start from the target QML behavior and its exposed C++ contract; preserve the UI behavior while moving durable logic. |
| C++ explanation, research, or implementation planning | `cpp-code-research` | Keep the investigation read-only, start from the smallest relevant backend entry point, and stop before edits. |
| QML research or implementation planning | `qml-code-research` | Keep the investigation read-only, start from the target screen, component, or binding, and stop before edits. |
| Bounded C++ review | `cpp-code-review` | Start from a focused diff or the explicitly identified existing function, class, file, or behavior; expand through relevant contracts or concrete risks. |
| Bounded QML/UI review | `qml-code-review` | Start from a focused diff or the explicitly identified existing component, file, or behavior; expand through relevant consumers, contracts, or concrete UI risks. |
| Whole-backend architecture review | `cpp-codebase-review` | Deliberately inventory the C++ codebase to assess architecture drift, responsibility overlap, and cross-module duplication. |
| Whole-QML architecture review | `qml-codebase-review` | Deliberately inventory the QML surface to assess component boundaries, durable-logic leakage, and broad duplication. |
| Configure, build, test, or diagnose CMake | `cmake-build-debug` | Use `For-Agent/Docs/build.md` for normal application configuration, builds, development runtime preparation, and backend tests. |
| Incremental final implementation validation | `test-and-review` | Start from the focused final diff, reuse established context, and expand only for a concrete new dependency, changed assumption, result, or risk. |

Use `.agents/skills/review-code-for-human/SKILL.md` only when the user explicitly requests `review-code-for-human` by name. Do not invoke it implicitly for a code review, explanation, walkthrough, or teaching request.

## Delegation Policy

Bounded implementation tasks normally stay with the lead agent. Delegate when
parallel discovery or independent scrutiny is useful; do not create subagents
merely because they are available. Do not invoke both researcher and reviewer by
default, or reduce significant review solely to save context. Existing specialist
roles remain read-only and return findings; the lead reconciles final results.

A brief supplies the precise objective, assigned files/symbols/diff or subsystem,
relevant established facts and navigation, unresolved risks or assumptions, and
the required result and stopping boundary. Keep trivial briefs short. In a
partitioned broad audit, each worker inspects its assigned portion; the lead owns
overall coverage, overlaps, gaps, and reconciliation. Reuse established discovery,
but independently inspect critical code and assumptions when reliable judgment
requires it. Avoiding repeated discovery does not prohibit independent verification.

Available specialists: `cpp_researcher` and `qml_researcher` for research;
`cpp_reviewer` and `qml_reviewer` for either bounded or explicitly requested
codebase review. `cpp_reviewer` specializes in C++/Qt correctness, ownership,
lifetime, threading, storage integrity, and model contracts. Select the bounded
or codebase skill from the requested scope before gathering context. Role names
do not establish comparative capability; configured execution settings must be
checked separately from instruction text.

## Implementation And Final Verification

Inspect the smallest relevant source surface and implement the bounded change.
Apply `.agents/skills/test-and-review/SKILL.md` for incremental final validation,
independent-review criteria, documentation synchronization, and the completion
report. Reuse established context throughout the task.

Artifact naming, timestamps, and selective reuse are owned by
`For-Agent/Docs/artifacts.md`.

## Core Rules

1. Make the smallest sufficient change that solves the task.
2. Do not expand scope without explicit user approval.
3. Do not break existing behavior unless the task explicitly requires it.
4. Preserve cross-platform compatibility for Windows and Linux.
5. Prefer simple, explicit, maintainable solutions over clever abstractions.
6. Before introducing a new class, function, method, service, model, component, helper, or domain entity, inspect the relevant existing code. Reuse a fully suitable implementation directly; extend a partially suitable one through an overload, template, or specialization when appropriate. Create a new entity only when existing implementations are unsuitable for the responsibility, reuse would reduce clarity, or extension would violate ownership boundaries or documented architecture.
7. Do not introduce temporary hacks, hidden fragile behavior, or unrelated refactoring.
8. Preserve unrelated user changes and personal data. Do not delete build directories, caches, generated files, or artifacts without explicit user approval.

## Git Rules

Git is read-only unless the user explicitly asks for a write operation.

Allowed:

- `git status`.
- `git diff`.
- `git log`.
- `git show`.
- `git branch`.

Forbidden without an explicit user request:

- `git commit`.
- `git push`.
- `git reset`.
- `git checkout`.
- `git clean`.
- `git rebase`.
- `git merge`.
- Force-push operations.

## Architecture Boundaries

Keep the project separated into:

- QML/UI layer.
- C++ backend and application state.
- Models, controllers, and services.
- Storage and configuration.
- Utilities.
- Tests.

### Production And Test Isolation

Tests must follow the production architecture; production code must never be changed to accommodate tests.

The dependency direction is strictly one-way: test targets may depend on production code, but `src/`, production targets, normal configure/build/deploy workflows, and production APIs must never depend on or reference test sources, test targets, Qt Test, fixtures, mocks, test-only compile definitions, or test-only hooks.

Do not add or widen production APIs, alter access control, ownership, lifetime, threading, behavior, or architectural boundaries solely to make testing easier. Tests and test support code belong under `tests/` and must exercise existing production contracts.

Never place business logic in QML. Move ALL business logic to C++.

Keep `src/main.cpp` minimal: application bootstrap, QML engine setup, type registration, dependency wiring, and startup logic only.

Avoid god classes, god files, and monolithic QML files. Decompose the codebase into well-defined classes, modules, and separate domain entities.

## C++ / Qt Rules

- Follow `For-Agent/Docs/coding-style.md` for detailed reuse, C++20, naming,
  ownership, parameter, organization, and documentation conventions.
- Use `constexpr`, `consteval`, and `if constexpr` for required contracts,
  invariants, or a clear engineering benefit; weigh clarity, portability,
  complexity, and compilation cost. Preserve intended compile-time guarantees.
- Use `#ifndef` / `#define` include guards instead of `#pragma once` in C++ headers.
- Use the `.hpp` extension for all C++ header files.
- Keep reusable template definitions needed across translation units in `.hpp`
  headers. Implementation-private templates may stay in a `.cpp` when all uses
  and required instantiations are confined to that translation unit.
- For user-defined class and struct fields, use the trailing underscore convention. (Example: field_)
- Use references for required non-owning parameters when appropriate.
- Use pointers when Qt ownership, QObject parent-child hierarchy, nullable dependencies, polymorphism, or signal/slot integration makes pointer semantics more correct.
- Always use curly braces for object constructors.
- Document non-trivial classes, functions, data structures, and architectural decisions.
- Apply the comment policy in `For-Agent/Docs/coding-style.md`: new ordinary
  explanatory C++/QML comments use `// Codex Comment:`. Preserve unmarked comments
  with unchanged code; authorized code changes permit associated comment updates
  or removal. Marked comments may be corrected in scope independently. 
  The marker never grants or restricts code-editing authority. 
  The presence or absence of the `// Codex Comment:` marker only affects comment-handling rules. 
  It does not determine whether Codex is allowed to modify, add, or remove the associated code. 
  Code modifications are governed by the task requirements and other project instructions.

## QML Rules

- Keep QML focused on presentation, layout, navigation, binding, and simple UI state.
- Prefer reusable QML components for repeated UI patterns.
- For UI tasks, inspect the relevant QML entry point first, then related C++ backend objects exposed to QML.
- Do not change the visual style globally unless the task explicitly requests it.

## Build And Verification

For routine code changes requiring build validation, use Debug only on the
current host. After editing, build `JobTrackerApp` with tests OFF, then build
and run required tests in a separate Debug test preset. After the final changes
and successful required checks, prepare the latest local application runtime
through `JobTrackerApp_dev_runtime`. Intermediate builds may compile only;
prepare runtime earlier when startup or dependency verification needs it.
Neither runtime preparation nor final verification requires a forced rebuild.
Use `JobTrackerTests` to build the full test suite; CTest runs it separately.
`JobTrackerApp_dev_runtime_fresh` is an explicit optional copy into a new
directory, not a routine prerequisite. Launch the main GUI only when required.
Documentation/instruction-only changes without build impact require no build,
tests, or deployment. Reuse successful unchanged checks.

Broader verification expands target/test coverage within Debug. Release builds
and Release tests require an explicit user request. Other configurations,
staging, and packaging also require an explicit user request. Follow the
[routine workflow](For-Agent/Docs/build.md#routine-codex-workflow) for the complete
selection rule.

For normal application configuration, builds, development runtime preparation,
and backend tests, use `For-Agent/Docs/build.md`.
Apply `.agents/skills/cmake-build-debug/SKILL.md` for build/test execution and
CMake diagnosis; it owns command sequencing, Codex tool permissions, process
completion, and failure handling. Run from the actual repository root using
PowerShell on Windows or a suitable shell on Linux.
