# QML And UI Style

Keep QML focused on presentation, layout, navigation, binding, and simple UI state.

Never add business logic directly to QML.

Move existing QML business logic into C++ only within the requested task scope.

Use `For-Agent/Docs/qml-to-cpp-extraction.md` for the extraction workflow and QML/C++ contract rules.

## Logic Boundaries

QML may keep simple UI state such as selected tabs, expanded rows, hover state, local popups, animations, and layout behavior.

Move durable behavior to C++ when it affects product state, validation, storage, parsing, filtering, sorting, search, grouping, cross-screen data, or stable cross-platform behavior.

Choose the QML/C++ boundary from product responsibility and architecture, never from test convenience. Do not add or widen QML-facing production APIs solely for tests; tests must follow the established production contract.

QML should send user intent to C++ through explicit signals, slots, or invokable commands, then consume backend state through properties, signals, and models.

## Navigation And Structure

For UI tasks, start from the target page or component. Inspect `qml/Main.qml`
only when the task affects the application shell, navigation, global actions,
page creation, or reachability.

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

## Button Interaction Palette

Use `PrimaryButton` for primary and neutral actions. Set `subtle: true` for
neutral actions and `selected: true` for selected tabs or mode switches. Use
`DangerButton` for destructive actions. Runtime pages and app-owned dialogs
must use shared styled components. Those components must derive from
`QtQuick.Templates.Button`, not `QtQuick.Controls.Button`, so platform styles
cannot inject native hover layers. Show visual keyboard focus by increasing the
existing semantic border to 2 px; do not add a white focus fill.

| Role | Normal | Hover | Pressed | Disabled | Border |
|---|---|---|---|---|---|
| Primary | `#1479EE` | `#2588FF` | `#0F63C9` | `#31506D` | `#59ADFF` |
| Neutral | `#0B1B27` | `#102A58` | `#132F61` | `#26343E` | `#223542` |
| Selected | `#123C76` | `#18498B` | `#0E2B5E` | `#26343E` | `#1687FF` |
| Destructive | `#D94343` | `#EB5353` | `#A92F2F` | `#26343E` | `#FF6B69` |
| Favorite icon | `#FFBD21` | `#FFD45C` | `#D99A00` | `#81909D` | Not applicable |

Use `#EEF3F8` or white for enabled button text and `#81909D` for disabled
button text. Use `#3B4A55` for disabled borders. White must never be used as a
button background. Resolve overlapping states in this order: disabled,
pressed, hovered, selected, then normal.

Apply the same semantic states to app-owned `ItemDelegate` navigation controls
and visually distinct `MouseArea`-backed actions. Keep data-row selection,
checkboxes, combo boxes, scrollbars, native file dialogs, and operating-system
controls outside this button palette.

## Selection Controls

Use the shared `SelectionCheckBox` for application and CV bulk selection. It
must derive from `QtQuick.Templates.CheckBox` and define its own indicator,
background, and content so platform styles cannot inject native glyphs or hover
layers. Draw the checked state with the shared white vector checkmark. Indicate
the partially checked state only with its soft state-specific fill and border;
do not draw an inner dash or other glyph. Do not use font glyphs, bitmap assets,
native indicators, or secondary focus squares. Show visual keyboard focus by
increasing the same semantic border to 2 px.

Indicate the selected data row or card with its subtle background fill while
retaining only the ordinary structural divider or border. Do not add a blue
accent outline or selection bar around selected data items.

## Verification

After QML changes, build the smallest affected production target that provides
meaningful compile and link confidence. Broaden verification only under the
criteria in `.agents/skills/test-and-review/SKILL.md` and
`For-Agent/Docs/testing.md`; use [build.md](build.md) for shared action selection
and its relevant OS reference for commands.

When an available suitable environment supports relevant, proportionate runtime
or visual inspection, use it to exercise the affected interaction and inspect the
changed visual result. Check relevant states, transitions, navigation, bindings,
focus/layout, and QML runtime warnings. Keep the checks on the changed surface
unless evidence or risk requires broader coverage. A successful build does not
prove the interaction or appearance was verified.

When runtime or visual inspection is unavailable, state exactly what remains
unverified and give a concrete manual checklist for the affected behavior: the
screen and action, expected result, relevant empty/loading/error/disabled states,
transitions and navigation, and supported window sizes where fitting or overlap
matters. Distinguish observed checks from those still needed; a full-application
visual tour is not mandatory for every edit.

For ordinary explanatory QML comments, apply the canonical
[comment policy](coding-style.md#explanatory-comments-in-c-and-qml), including
`// Codex Comment:`, preservation of unmarked comments with unchanged code, and
associated-comment updates for authorized code changes.
