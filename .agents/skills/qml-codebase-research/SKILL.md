---
name: qml-codebase-research
description: Research and assess the JobTracker QML codebase without implementation. Use when Codex needs to inspect qml/Main.qml, qml/pages, qml/components, QML module registration, page/component size, duplication, UI-only state, and refactoring opportunities, especially when the user asks whether QML files should be split or further refactored.
---

# QML Codebase Research

Use this skill for read-only QML architecture research in JobTracker.

## Scope

- Inspect `qml/Main.qml`, `qml/pages`, `qml/components`, and QML file registration in `CMakeLists.txt`.
- Read `docs/qml-style.md` before judging QML structure.
- Do only analysis, not implementation. Do not edit any files.
- Inspect QML concerns separate from C++ backend concerns. Flag business logic in QML as a candidate for C++ only when it is durable product logic, not mock UI state.

## Workflow

1. Start from `qml/Main.qml` to understand navigation and page ownership.
2. List QML files and approximate sizes with `rg --files qml` and line counts.
3. Inspect large page files first, then shared components.
4. Check `CMakeLists.txt` to ensure QML files are registered when assessing existing or proposed splits.
5. Look for:
   - monolithic pages that mix shell, data, tables, cards, overlays, and local components;
   - repeated local components that could be shared;
   - UI state or mock data that is acceptable short-term but should move behind backend models later;
   - accidental visual/style drift risks from proposed extraction;
   - missing or overly broad component APIs.
6. Report refactoring recommendations by priority:
   - `Needed now`: improves maintainability or prevents continued god files;
   - `Optional later`: useful cleanup, but not urgent;
   - `Do not refactor now`: would add abstraction without clear value.

## Output

Include:

- Current QML structure summary.
- Files that still deserve splitting, with concrete split boundaries.
- Files that should stay as they are.
- Risks and verification needs for any future split.
- A short conclusion answering whether more QML refactoring is still necessary.
