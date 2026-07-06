---
name: qt-qml-ui-task
description: Work on JobTracker Qt/QML UI tasks. Use when Codex needs to add or modify QML pages, components, navigation, layout, visual styling, QML resources, or UI behavior in qml/Main.qml, qml/pages, qml/components, or related C++ objects exposed to QML.
---

# Qt QML UI Task

Use this skill for QML and UI work in JobTracker.

## Workflow

1. Read `For-Agent/Docs/qml-style.md`.
2. Start from `qml/Main.qml` for navigation and shell context.
3. Inspect the target page or component under `qml/pages` or `qml/components`.
4. Preserve existing backend contracts and QML imports.
5. Keep durable business logic out of QML unless the task explicitly asks for a UI-only prototype.
6. Extract a reusable component only when repetition or readability justifies it.
7. If adding QML files, update `qt_add_qml_module` in `CMakeLists.txt`.
8. Verify with the normal build when QML, resources, CMake, or runtime behavior changed.

## Manual UI Checks

When visual verification is not possible, list concrete manual checks:

- Application starts.
- Target screen opens.
- Sidebar navigation still works.
- Text fits at supported window sizes.
- No obvious overlapping or clipped UI.
