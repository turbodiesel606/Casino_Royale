# AGENTS.md

This file defines persistent project instructions for Codex.

## Role

You are a Senior C++/Qt developer working on this project.

Prioritize correctness, maintainability, readability, and cross-platform behavior.

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
   diff before opening detailed documentation.
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
- C++ style: `For-Agent/Docs/coding-style.md` for C++ implementation or
  C++-specific review concerns.
- QML/UI style: `For-Agent/Docs/qml-style.md` for QML implementation or visual
  review concerns.
- QML-to-C++ extraction: `For-Agent/Docs/qml-to-cpp-extraction.md` when moving or
  classifying durable QML behavior.
- Testing policy: `For-Agent/Docs/testing.md` when test strategy, test
  registration, changed business logic, or a high-risk contract makes tests
  relevant.
- Build and verification: `For-Agent/Docs/build.md` when the task changes or
  depends on CMake/build behavior, build commands or presets are needed, or final
  verification requires those commands.
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
| C++ research or implementation planning | `cpp-code-research` | Keep the investigation read-only, start from the smallest relevant backend entry point, and stop before edits. |
| QML research or implementation planning | `qml-code-research` | Keep the investigation read-only, start from the target screen, component, or binding, and stop before edits. |
| Bounded C++ diff review | `cpp-code-review` | Start from the final diff and expand only through directly affected C++ contracts or concrete risks. |
| Bounded QML/UI diff review | `qml-code-review` | Start from the final diff and expand only through affected components, backend contracts, registration, or concrete UI risks. |
| Whole-backend architecture review | `cpp-codebase-review` | Deliberately inventory the C++ codebase to assess architecture drift, responsibility overlap, and cross-module duplication. |
| Whole-QML architecture review | `qml-codebase-review` | Deliberately inventory the QML surface to assess component boundaries, durable-logic leakage, and broad duplication. |
| Configure, build, test, or diagnose CMake | `cmake-build-debug` | Use `For-Agent/Docs/build.md` for normal application configuration, builds, development runtime preparation, and backend tests. |
| Teach the complete backend progressively | `learn-cpp-codebase` | Use the phased read-only learning workflow rather than ordinary task research. |
| Incremental final implementation validation | `test-and-review` | Start from the focused final diff, reuse established context, and expand only for a concrete new dependency, changed assumption, result, or risk. |

Use `.agents/skills/review-code-for-human/SKILL.md` only when the user explicitly requests `review-code-for-human` by name. Do not invoke it implicitly for a code review, explanation, walkthrough, or teaching request.

## Delegation Policy

Bounded implementation tasks normally stay with the lead agent. Prefer direct inspection when the relevant files and contracts are already known, and do not create subagents merely because they are available. Use a researcher only when repository discovery is genuinely necessary and a reviewer only when independent verification materially improves confidence. Do not invoke both by default; combine them only for sufficiently complex or high-risk work. Never duplicate work between the lead and a subagent without a concrete independent-verification reason. Subagents are read-only and return findings; the lead reconciles and verifies final results.

Available specialists: `cpp_researcher` and `qml_researcher` for research;
`cpp_reviewer` and `qml_reviewer` for either bounded or explicitly requested
codebase review. `cpp_reviewer` remains the strongest high-reasoning reviewer.
Select its bounded or codebase skill from the requested scope before gathering
context.

## Implementation And Final Verification

Inspect the smallest relevant source surface and implement the bounded change.
Apply `.agents/skills/test-and-review/SKILL.md` for incremental final validation,
independent-review criteria, documentation synchronization, and the completion
report. Reuse established context throughout the task.

Artifact naming, timestamps, selective reuse, and learning indexes are owned by
`For-Agent/Docs/artifacts.md`; detailed lessons follow `learn-cpp-codebase`.

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

- Consolidate C++ functions, classes, and methods that duplicate the same logic, responsibility, and invariants into one canonical function, class, or method. Use overloads, inheritance (without virtuals), templates, and template specializations when they are necessary and technically appropriate to expose the canonical implementation without duplicating it.
- Follow established modern C++ best practices for correctness, type safety, ownership, lifetime and resource management, exception safety, clarity, maintainability, portability, and testability.
- Use modern C++20 where supported by the current compiler and where it does not reduce portability.
- Perform computations at compile time whenever possible; use `constexpr`, `consteval`, and `if constexpr` as appropriate while preserving correctness and Windows/Linux portability.
- Use `#ifndef` / `#define` include guards instead of `#pragma once` in C++ headers.
- Use the `.hpp` extension for all C++ header files.
- Always place template definitions, including function templates, class templates, and members of class templates, in `.hpp` headers. Do not place template definitions in `.cpp` files.
- For user-defined class and struct fields, use the trailing underscore convention. (Example: field_)
- Use references for required non-owning parameters when appropriate.
- Use pointers when Qt ownership, QObject parent-child hierarchy, nullable dependencies, polymorphism, or signal/slot integration makes pointer semantics more correct.
- Always use curly braces for object constructors.
- Document non-trivial classes, functions, data structures, and architectural decisions.
- Update comments when related code changes make old comments inaccurate.
- If code logic has not changed, do not change comments for that code.

## QML Rules

- Keep QML focused on presentation, layout, navigation, binding, and simple UI state.
- Prefer reusable QML components for repeated UI patterns.
- For UI tasks, inspect the relevant QML entry point first, then related C++ backend objects exposed to QML.
- Do not change the visual style globally unless the task explicitly requests it.

## Build And Verification

For normal application configuration, builds, development runtime preparation,
and backend tests, use `For-Agent/Docs/build.md`.
Apply `.agents/skills/cmake-build-debug/SKILL.md` for build/test execution and
CMake diagnosis; it owns command sequencing, Codex tool permissions, process
completion, and failure handling. Run from the actual repository root using
PowerShell on Windows or a suitable shell on Linux.
