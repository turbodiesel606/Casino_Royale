---
name: cpp-code-research
description: Research the JobTracker C++ backend without making code changes. Use when Codex needs to map C++ classes, controllers, models, services, startup wiring, tests, CMake source registration, data flow, architecture risks, or implementation options before a C++ task.
---

# C++ Code Research

Use this skill for read-only C++ backend investigation.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/coding-style.md`
4. `For-Agent/Docs/testing.md`

Also read `For-Agent/Docs/qml-to-cpp-extraction.md` when the research involves QML-facing controllers, models, or business logic being moved out of QML.

## Entry Points

Start from the smallest relevant C++ surface:

- Startup and wiring: `src/main.cpp`, `src/app/`.
- Shared backend utilities: `src/common/`.
- Jobs domain: `src/jobs/`.
- CV domain: `src/cvs/`.
- Dashboard domain: `src/dashboard/`.
- Company/contact domain: `src/directory/`.
- Tests: `tests/`.
- Build inventory: `CMakeLists.txt`.

Use `rg` for symbols, class names, QML context properties, `Q_PROPERTY`, `Q_INVOKABLE`, model role names, tests, and CMake source registration.

## Research Output

Return concise findings with:

- Scope and files inspected.
- Current C++ ownership and data flow.
- QML-facing contracts, if any.
- Relevant tests and missing test coverage.
- Architecture or maintainability risks.
- Suggested implementation direction, without changing code.

Write it under `For-Agent/Research/` with a clear name such as `cpp-research-YYYY-MM-DD-topic.md`.

## Boundaries

Do not edit production code during a research task.
Do not broaden into QML visual review unless the C++ behavior depends on QML contracts.
Do not run builds or tests unless the user asks for verification or the research depends on current build state.
