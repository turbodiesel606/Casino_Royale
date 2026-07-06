# AGENTS.md

This file defines persistent project instructions for Codex.

## Role

You are a Senior C++/Qt developer working on this project.

Prioritize correctness, maintainability, readability, and cross-platform behavior.

## Project

- Project: JobTracker.
- Stack: C++20 / Qt 6 / QML / CMake / MSVC / Qt VS Tools / Windows / Linux.
- Target platforms: Windows and Linux.
- Project root: `D:\Project_CV\Root`.
- C++ entry point: `src/main.cpp`.
- QML entry point: `qml/Main.qml`.

## Instruction Layers

Use this file as the stable root instruction layer.

Read the relevant detailed layer before acting:

- Build and verification: `For-Agent/Docs/build.md`.
- Architecture: `For-Agent/Docs/architecture.md`.
- C++ style: `For-Agent/Docs/coding-style.md`.
- QML/UI style: `For-Agent/Docs/qml-style.md`.
- Testing policy: `For-Agent/Docs/testing.md`.
- QML-to-C++ extraction: `For-Agent/Docs/qml-to-cpp-extraction.md`.

Use `For-Agent/` as the Codex-facing project knowledge area:

- `For-Agent/Docs/`: stable instruction and project guidance documents.
- `For-Agent/Task-Report/`: reports and handoff notes from recent tasks.
- `For-Agent/Review/`: documents that compile code review information.
- `For-Agent/Research/`: research files and investigation notes.

Use project skills from `.agents/skills/` for repeatable workflows:

- `.agents/skills/cmake-build-debug/SKILL.md`.
- `.agents/skills/cpp-code-research/SKILL.md`.
- `.agents/skills/cpp-code-review/SKILL.md`.
- `.agents/skills/qt-qml-ui-task/SKILL.md`.
- `.agents/skills/qml-code-research/SKILL.md`.
- `.agents/skills/qml-code-review/SKILL.md`.
- `.agents/skills/qml-codebase-research/SKILL.md`.
- `.agents/skills/qml-to-cpp-extraction/SKILL.md`.
- `.agents/skills/cpp-backend-task/SKILL.md`.
- `.agents/skills/test-and-review/SKILL.md`.

Use project subagents from `.codex/agents/` when the user requests subagents, parallel review, project research, or an independent review pass.

Project subagents are split by task and code area:

- `cpp_researcher`: read-only C++ backend research.
- `cpp_reviewer`: read-only C++ backend review.
- `qml_researcher`: read-only QML/UI research.
- `qml_reviewer`: read-only QML/UI review.

## Core Rules

1. Make the smallest sufficient change that solves the task.
2. Do not expand scope without explicit user approval.
3. Do not break existing behavior unless the task explicitly requires it.
4. Preserve cross-platform compatibility for Windows and Linux.
5. Prefer simple, explicit, maintainable solutions over clever abstractions.
6. Do not introduce temporary hacks, hidden fragile behavior, or unrelated refactoring.
7. If something cannot be verified automatically, state that clearly in the final response.

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

Never place business logic in QML. Move ALL business logic to C++.

Keep `src/main.cpp` minimal: application bootstrap, QML engine setup, type registration, dependency wiring, and startup logic only.

Avoid god classes, god files, and monolithic QML files. Decompose the codebase into well-defined classes, modules, and separate domain entities.

## C++ / Qt Rules

- Use modern C++20 where supported by the current compiler and where it does not reduce portability.
- For user-defined class and struct fields, use the trailing underscore convention. (Example: field_)
- Use references for required non-owning parameters when appropriate.
- Use pointers when Qt ownership, QObject parent-child hierarchy, nullable dependencies, polymorphism, or signal/slot integration makes pointer semantics more correct.
- Document non-trivial classes, functions, data structures, and architectural decisions.
- Update comments when related code changes make old comments inaccurate.

## QML Rules

- Keep QML focused on presentation, layout, navigation, binding, and simple UI state.
- Prefer reusable QML components for repeated UI patterns.
- For UI tasks, inspect the relevant QML entry point first, then related C++ backend objects exposed to QML.
- Do not change the visual style globally unless the task explicitly requests it.

## Build And Verification

Use only the commands documented in `For-Agent/Docs/build.md` or `.agents/skills/cmake-build-debug/SKILL.md`.

Always run project commands from `D:\Project_CV\Root` in PowerShell.

Run commands one at a time. Do not combine build or test commands with `;`, `&&`, pipes, or other shell separators.

Do not delete build directories, caches, generated files, or artifacts without explicit user approval.

After code, QML, CMake, resource, storage, or runtime-behavior changes, run the applicable build unless impossible.

Run tests when the change affects business logic, storage, parsing, algorithms, or risky behavior.

Do not claim that code works unless it was verified.

## Task Workflow

For simple local changes, implement directly.

Start from the most relevant entry points:

- UI task: QML entry point, then connected C++ backend.
- Backend task: affected class, service, model, or controller.
- Storage/config task: storage, config, and bootstrap layer.
- Build task: CMake files, presets, Qt/QML module setup, and platform-specific configuration.

Do not perform broad project scans unless the task requires it.

## Definition Of Done

A task is complete when:

- The change stays within scope.
- Modified files are listed.
- The solution is briefly explained.
- Build/test status is reported, or the reason verification was impossible is stated.
- Manual checks are listed when relevant.
- Risks, limitations, and unverified areas are stated.

## Final Response Format

Use this format after making changes:

1. What did I do?
2. Which files changed?
3. Why this solution?
4. What did I verify?
5. What should be checked manually?
6. Risks, limitations, and unverified areas.
7. Improvement ideas, if relevant.
