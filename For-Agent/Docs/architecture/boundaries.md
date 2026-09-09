# Cross-Domain Boundaries

Read this document when a task changes responsibility placement, dependency
direction, ownership, lifetime, error translation, or production/test isolation.

## UI And Backend Boundary

Keep QML focused on presentation, layout, navigation, binding, and simple UI
state.

Place durable business logic, storage, parsing, algorithms, and application state
in C++ backend classes when they are introduced.

Use `For-Agent/Docs/qml-to-cpp-extraction.md` when moving durable behavior from
QML into C++.

Keep `src/main.cpp` minimal. Move non-trivial startup wiring into dedicated
bootstrap/application classes when the startup surface grows.

Avoid global visual rewrites unless the task explicitly asks for a broader
design change.

Avoid large monolithic pages. Extract reusable QML components when repetition
becomes meaningful.

Large current QML panes that should be treated carefully before adding more
behavior include:

- `JobDescriptionPane.qml`
- `JobsApplicationsPane.qml`
- `CvLibraryBrowserPane.qml`
- `ContactsPage.qml`
- `JobFormPage.qml`
- `CompaniesPage.qml`
- `CvLibraryPreviewPanel.qml`

Presentation-only state should stay in QML. Examples include current page index,
form visibility, local hover/selection styling, layout geometry, popup state,
custom scroll visuals, and display-only table column metadata.

Backend-owned behavior should remain in C++ when it affects validation,
persistence, import, filtering, sorting, search, grouping, cross-screen data, or
behavior that should be tested.

## Backend Responsibilities

Organize backend code by responsibility:

- Domain and value types for durable product data.
- Models for list or table data consumed by QML delegates.
- Controllers or view models for QML-facing properties, commands, and signals.
- Services for validation, parsing, algorithms, and business operations.
- Storage and configuration classes for persistence and platform-aware file
  access.
- Bootstrap/application classes for startup wiring that does not belong in
  `src/main.cpp`.

QML-facing controllers should expose a small screen contract and delegate
non-trivial behavior to services or models.

## Dependency Direction

Use the primary durable-command dependency direction
`QML -> controllers/models -> worker facades/executors -> services -> repositories -> storage`.

This is a responsibility map, not a claim that every call traverses every
layer. Bootstrap currently hydrates repositories and runs managed-file recovery
directly. `CvLibraryController` uses `CvFileAccessService` to open a selected
document and persists the single-row favorite toggle directly through
`CvRepository` on the GUI thread. Multi-step job saves, CV imports, and removals
use the worker/service paths.

Tests sit outside that production chain. Their only permitted dependency
direction is `tests -> production`; no production layer may depend on test code,
test support, Qt Test, test-only build settings, or test targets.

- QML consumes focused controllers and models. Do not expose repositories or
  broad service objects directly to QML.
- Controllers translate QML values and actions into backend calls, own UI-facing
  state, and publish the smallest useful property, command, signal, and
  model-role contract. The current CV controller also owns the synchronous
  favorite persistence and file-open boundary described above.
- Models present repeated data and maintain Qt model/view contracts. They do not
  query storage or coordinate multi-step business operations.
- Services own validation, parsing, algorithms, multi-step business operations,
  and transaction coordination. Reusable proxy models own the current
  filtering, sorting, search, and relation projection mechanics.
- Repositories encapsulate persistence queries and map stored rows to domain
  values without depending on QML contracts.
- Storage classes own database connections, schema migration, and platform-aware
  storage primitives.
- Bootstrap/application classes construct and own the dependency graph and
  connect cross-component notifications. Do not place business rules in
  bootstrap wiring.
- Define production responsibilities and contracts from product behavior and
  architecture. Do not add or widen an API, expose an internal, weaken access
  control, or change ownership, lifetime, threading, or module boundaries solely
  for tests; tests must exercise the resulting production contract.

## Ownership And Lifetime

- Give each object one explicit owner. Use either Qt parent-child ownership,
  direct member ownership, or a standard C++ ownership type; do not combine
  ownership mechanisms for the same object.
- Treat injected pointers and references as non-owning unless the API explicitly
  transfers ownership.
- Keep every controller and model exposed through a QML context property alive
  until the QML engine can no longer evaluate or call that object.
- Define destruction order for connected QObjects, asynchronous work, database
  connections, and the QML engine so no callback can target a destroyed object.

## Errors And QML Boundaries

- Represent expected validation and business rejections as structured results,
  including field errors when the UI can act on them.
- Use exceptions for unexpected startup, storage, file-system, or infrastructure
  failures when the caller cannot handle them locally.
- Catch and translate failures at controller or application boundaries before
  they reach QML. Do not allow C++ exceptions to cross QML invocations, signal
  delivery, or queued callbacks.
- Preserve the process-level startup exception boundary in `App::start()`.
