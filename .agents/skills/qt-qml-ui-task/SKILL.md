---
name: qt-qml-ui-task
description: Work on JobTracker Qt/QML UI tasks. Use when Codex needs to add or modify QML pages, components, navigation, layout, visual styling, QML resources, or UI behavior in qml/Main.qml, qml/pages, qml/components, or related C++ objects exposed to QML.
---

# Qt QML UI Task

Use this skill for QML and UI work in JobTracker.

## Workflow

1. Apply `AGENTS.md` and read `For-Agent/Docs/qml-style.md`.
2. Inspect `git status` and existing diffs for files in scope. Identify and preserve pre-existing changes.
3. Start from the target page or component under `qml/pages` or `qml/components`.
4. Inspect `qml/Main.qml` only when the task affects the application shell, navigation, global actions, page creation, or reachability.
5. Inspect the target component's definition, its direct parent or instantiation sites, and the properties, signals, bindings, imports, and model roles involved in the requested behavior. Do not inventory unrelated QML files.
6. Inspect related C++ only at the QML-facing controller or model contract consumed by the target. Read `For-Agent/Docs/architecture/qml-contracts.md` and the affected architecture domain document only when this boundary is involved. Use the architecture overview only if routing is unclear.
7. Use `cpp-backend-task` if implementing the request requires C++ changes. Read `For-Agent/Docs/testing.md` only when durable behavior or a high-risk contract requires test changes.
8. Preserve existing backend contracts and QML imports.
9. Keep durable business logic out of QML unless the task explicitly asks for a UI-only prototype.
10. Extract a reusable component only when repetition or readability justifies it.
11. If adding QML files, update `qt_add_qml_module` in `CMakeLists.txt`.
12. After implementation is stable, finish with one incremental `test-and-review` pass. Use `cmake-build-debug` for required build/tests, `testing.md` for test selection and expansion, and perform one documentation-impact check.
13. Use an available suitable environment to exercise the changed interaction and inspect its visual result when relevant and proportionate. Follow `For-Agent/Docs/qml-style.md` for states, transitions, navigation, bindings, layout, and runtime warnings. Report what was observed separately from build success; broaden only for concrete risks.

## Runtime And Manual UI Evidence

If runtime or visual inspection is unavailable, state the unavailable environment
or capability and what remains unverified. Provide the focused manual checklist
defined by `qml-style.md`, with concrete actions and expected results for this
change. A build alone is not runtime or visual verification.
