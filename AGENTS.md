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
- Artifact creation and reuse: `For-Agent/Docs/artifacts.md`.
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
- `.agents/skills/learn-cpp-codebase/SKILL.md`.
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

## Research And Review Workflow

Use `learn-cpp-codebase` for a progressive, read-only teaching tour of the complete C++ backend from project inventory through startup, domains, workflows, Qt contracts, storage, tests, and final synthesis.

Use `cpp-code-research` or `qml-code-research` for investigation before implementation.

Use `cpp-backend-task` as the entry point for C++ backend implementation. Complete applicable build and test verification through `cmake-build-debug`, then use `test-and-review` for the final scope, documentation, verification, and risk pass.

Use `cpp-code-review` or `qml-code-review` for review after changes or when the user asks for a review.

Use `.codex/agents/` subagents only when the user requests subagents, parallel research, or independent review. Subagents are read-only and return findings; the lead Codex compiles, verifies, and saves final artifacts when needed.

Save durable research artifacts under `For-Agent/Research/`.

Save durable review artifacts under `For-Agent/Review/`.

When creating durable research or review artifacts, include the creation date and time in the filename using `YYYY-MM-DD-HHMM`, and put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

Before repeating broad research or review, check the relevant artifact folders in `For-Agent/Research/`, `For-Agent/Review/`, and `For-Agent/Task-Report/`. Use the most recent relevant artifact by timestamp as context, then verify current facts against the actual source, instructions, and diff. Determine recency from the filename timestamp first; for legacy date-only artifacts, inspect the beginning of the file for a `Created:` timestamp, and if no time exists, treat the artifact as the earliest one for that date.

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
- Use `#ifndef` / `#define` include guards instead of `#pragma once` in C++ headers.
- Use the `.hpp` extension for all C++ header files.
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

Use only the commands documented in `For-Agent/Docs/build.md` or `.agents/skills/cmake-build-debug/SKILL.md`.

Always run project commands from `D:\Project_CV\Root` in PowerShell.

Run commands one at a time. Do not combine build or test commands with `;`, `&&`, pipes, or other shell separators.

Always run every `cmake --build ...` command with elevated access from the first attempt. Do not first attempt a build with restricted or sandboxed access.

Wait for each build process to finish and return its final exit code before continuing or reporting its status. If the command runner yields a still-running build, keep waiting on that same process until it completes. Do not treat missing output as completion, and do not start a duplicate build while the original process is still running.

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

## Documentation Synchronization

After changes to architecture, code, build configuration, testing workflow, or runtime behavior, review `For-Agent/Docs/` and update only the documentation files affected by the change. Do not make unrelated documentation edits. If no documentation is affected, state that the review was completed and no update was required.

## Definition Of Done

A task is complete when:

- The change stays within scope.
- Modified files are listed.
- The solution is briefly explained.
- Build/test status is reported, or the reason verification was impossible is stated.
- Relevant `For-Agent/Docs/` guidance was updated, or its review confirmed that no update was needed.
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
