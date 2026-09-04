---
name: qt-qml-ui-task
description: Work on JobTracker Qt/QML UI tasks. Use when Codex needs to add or modify QML pages, components, navigation, layout, visual styling, QML resources, or UI behavior in qml/Main.qml, qml/pages, qml/components, or related C++ objects exposed to QML.
---

# Qt QML UI Task

Use this skill for QML and UI work in JobTracker.

## Workflow

1. Read `For-Agent/Docs/qml-style.md`.
2. Inspect `git status` and existing diffs for files in scope. Identify and preserve pre-existing changes.
3. Start from the target page or component under `qml/pages` or `qml/components`.
4. Inspect `qml/Main.qml` only when the task affects the application shell, navigation, global actions, page creation, or reachability.
5. Inspect the target component's definition, its direct parent or instantiation sites, and the properties, signals, bindings, imports, and model roles involved in the requested behavior. Do not inventory unrelated QML files.
6. Inspect related C++ only at the QML-facing controller or model contract consumed by the target. Use `cpp-backend-task` if implementing the request requires C++ changes.
7. Preserve existing backend contracts and QML imports.
8. Keep durable business logic out of QML unless the task explicitly asks for a UI-only prototype.
9. Extract a reusable component only when repetition or readability justifies it.
10. If adding QML files, update `qt_add_qml_module` in `CMakeLists.txt`.
11. Verify with the normal build when QML, resources, CMake, or runtime behavior changed.

## Manual UI Checks

When visual verification is not possible, list concrete manual checks:

- Application starts.
- Target screen opens.
- Affected navigation and interactions still work.
- Text fits at supported window sizes.
- No obvious overlapping or clipped UI.
