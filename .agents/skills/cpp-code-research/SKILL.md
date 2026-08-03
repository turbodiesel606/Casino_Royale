---
name: cpp-code-research
description: Research the JobTracker C++ backend without making code changes. Use when Codex needs to map C++ classes, controllers, models, services, startup wiring, tests, CMake source registration, data flow, architecture risks, code duplication, overlapping responsibilities, or implementation options before a C++ task.
---

# C++ Code Research

Use this skill for read-only C++ backend investigation.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/coding-style.md`
5. `For-Agent/Docs/testing.md`

Also read `For-Agent/Docs/qml-to-cpp-extraction.md` when the research involves QML-facing controllers, models, or business logic being moved out of QML.

Before repeating broad research, check `For-Agent/Research/` and relevant `For-Agent/Task-Report/` artifacts. Use the most recent relevant artifact by timestamp as context only, then verify current facts against the source.

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

## Duplication Analysis

When the research concerns architecture or maintainability:

1. Inspect exact, structural, and semantic duplication across:
   - classes, structs, functions, methods, and helpers;
   - controllers, models, services, repositories, and storage classes;
   - validation, normalization, formatting, filtering, selection, error handling, and transaction logic;
   - SQL statements, test fixtures, test setup, and CMake target definitions.
2. Search by responsibility and behavior, not only identical text. Compare similarly named methods, repeated branches, repeated constants, SQL fragments, model roles, signals, and property-update sequences.
3. For every duplication cluster:
   - identify all affected files and symbols;
   - describe the repeated responsibility;
   - identify the existing implementation that could be the canonical owner;
   - explain whether reuse, extension, extraction, or keeping the implementations separate is the safer direction.
4. Distinguish harmful duplication from intentional separation. Similar fields or control flow are not sufficient evidence when types represent different lifecycle stages, ownership boundaries, thread contexts, or domain meanings.
5. Do not propose a new abstraction until the relevant existing implementation has been inspected and found insufficient.

## Research Output

Return concise findings with:

- Scope and files inspected.
- Current C++ ownership and data flow.
- QML-facing contracts, if any.
- Relevant tests and missing test coverage.
- Architecture or maintainability risks.
- Suggested implementation direction, without changing code.
- Duplication map grouped by repeated responsibility, with affected files and symbols.
- Classification of each cluster as exact, structural, semantic, or intentional similarity.
- Existing implementation that could become the canonical owner.
- Consolidation risks, including coupling, ownership, lifetime, threading, and domain-boundary concerns.
When the lead Codex is doing the research and a durable artifact is needed, write it under `For-Agent/Research/` with a clear name such as `cpp-research-YYYY-MM-DD-HHMM-topic.md`. Put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

If a read-only subagent is used, treat the subagent output as research input. The lead Codex should compile, verify, and save the final research artifact.
Do not recommend abstraction based only on similar syntax, naming, or fields. 
Verify that the implementations share responsibility, invariants, and reasons to change.

## Boundaries

Do not edit production code during a research task.
Do not broaden into QML visual review unless the C++ behavior depends on QML contracts.
Do not run builds or tests unless the user asks for verification or the research depends on current build state.
