# QML And UI Style

Keep QML focused on presentation, layout, navigation, binding, and simple UI state.

Never add business logic directly to QML.

Move existing QML business logic into C++.

Use `For-Agent/Docs/qml-to-cpp-extraction.md` for the extraction workflow and QML/C++ contract rules.

## Logic Boundaries

QML may keep simple UI state such as selected tabs, expanded rows, hover state, local popups, animations, and layout behavior.

Move durable behavior to C++ when it affects product state, validation, storage, parsing, filtering, sorting, search, grouping, cross-screen data, or behavior that should be covered by tests.

QML should send user intent to C++ through explicit signals, slots, or invokable commands, then consume backend state through properties, signals, and models.

## Navigation And Structure

For UI tasks, start from `qml/Main.qml`, then inspect the relevant page or component.

Preserve the current shell structure unless the task asks for navigation changes:

- `ApplicationWindow`.
- Sidebar.
- `StackLayout`.
- Page components under `qml/pages`.
- Reusable components under `qml/components`.

## Components

Extract reusable components when a UI pattern repeats or when a page becomes hard to scan.

Keep component APIs explicit through properties and signals.

Avoid surprising global style changes. Prefer local changes scoped to the requested screen or component.

When adding QML files, update the `qt_add_qml_module` file list in `CMakeLists.txt`.

## Verification

After QML changes, run the normal build unless impossible.

If visual verification is not possible, list concrete manual checks:

- App starts.
- Target page opens.
- Navigation still works.
- Text fits at supported window sizes.
- No obvious overlapping UI.
