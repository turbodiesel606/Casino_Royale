---
name: qml-code-research
description: Research the JobTracker QML codebase without making code changes. Use when Codex needs to map QML screens, components, navigation, bindings, visual structure, repeated UI patterns, QML-to-C++ contracts, or business logic still present in QML before a UI or extraction task.
---

# QML Code Research

Use this skill for read-only QML investigation.

## Required Context

Read these files first:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/qml-style.md`

Read `For-Agent/Docs/qml-to-cpp-extraction.md` when the research involves mock data, validation, filtering, sorting, search, cross-screen state, or other durable behavior in QML.

Before repeating broad research, check `For-Agent/Research/` and relevant `For-Agent/Task-Report/` artifacts. Use the most recent relevant artifact by timestamp as context only, then verify current facts against the source.

## Entry Points

Start from the relevant QML surface:

- App shell and navigation: `qml/Main.qml`.
- Shared UI components: `qml/components/`.
- Pages and feature panes: `qml/pages/`.
- Backend contracts exposed to QML: related files in `src/`.
- QML file registration: `CMakeLists.txt`.

Use `rg` for `property`, `signal`, `function`, `model:`, `Repeater`, `ListView`, `StackLayout`, `Connections`, backend object names, inline arrays, and duplicated helper logic.

## Research Output

Return concise findings with:

- Scope and files inspected.
- Current page/component ownership.
- UI-only state that should stay in QML.
- Durable behavior or mock data that should move to C++.
- Repeated component patterns or decomposition candidates.
- Backend contracts the QML depends on.
- Suggested next step, without changing code.

When the lead Codex is doing the research and a durable artifact is needed, write it under `For-Agent/Research/` with a clear name such as `qml-research-YYYY-MM-DD-HHMM-topic.md`. Put a `Created: YYYY-MM-DD HH:MM local time` line at the beginning of the file immediately after the title.

If a read-only subagent is used, treat the subagent output as research input. The lead Codex should compile, verify, and save the final research artifact.

## Boundaries

Do not edit QML during a research task.
Do not recommend global visual redesign unless the user asks for it.
Do not treat presentation-only behavior as a backend migration candidate.
