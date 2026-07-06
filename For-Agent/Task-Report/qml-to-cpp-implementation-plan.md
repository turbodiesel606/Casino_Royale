# QML To C++ Implementation Plan

This plan describes how to move JobTracker business logic from QML into C++ while preserving current behavior and keeping the application bootstrap exception-safe.

## 1. Inventory Current QML Logic

- Start from `qml/Main.qml`.
- Inspect `qml/pages`, `qml/components`, and current C++ objects exposed to QML.
- Identify all QML-owned business logic:
  - mock data;
  - filtering;
  - sorting;
  - validation;
  - computed product state;
  - cross-screen state;
  - parsing;
  - storage-like behavior;
  - action handlers that mutate durable state.

## 2. Classify Logic

- Keep in QML:
  - navigation;
  - selected page or tab;
  - popups;
  - hover and focus state;
  - visual formatting;
  - layout state.
- Move to C++:
  - application data;
  - validation;
  - search, filter, and sort behavior;
  - status transitions;
  - entity relationships;
  - storage, import, and export;
  - model data;
  - behavior that should be tested.

## 3. Define Backend Contracts Before Editing

For each screen or domain, define the intended QML/C++ contract:

- properties QML reads;
- commands QML invokes;
- signals QML handles;
- model roles delegates consume.

The goal is to avoid random C++ APIs that merely mirror old QML JavaScript.

## 4. Create Backend Slices

Use focused C++ responsibilities:

- Domain/value types: job application, company, contact, CV, attachment, status, and stage.
- Services: validation, filtering/search, sorting, relationship rules, and persistence coordination.
- Models: `QAbstractListModel` classes for repeated list or table data.
- Controllers/view models: small `QObject` APIs exposed to QML per screen or feature area.

## 5. Make Bootstrap Exception-Safe

- Keep `src/main.cpp` minimal.
- Move dependency construction into a focused bootstrap/application wiring class once setup grows.
- Use deterministic ownership:
  - Qt parent-child ownership for `QObject`s;
  - clear owning members for services and models.
- Wrap startup, bootstrap, and QML loading in a controlled failure boundary.
- Log `std::exception` and unknown exceptions with `qCritical()`.
- Return non-zero on startup failure.
- Keep `QQmlApplicationEngine::objectCreationFailed` handling.

## 6. Migrate One Slice At A Time

- Move the smallest high-value domain first, likely job applications or CV linkage.
- Replace QML JavaScript with bindings and calls into C++.
- Build after each meaningful slice.
- Avoid visual redesign while doing logic extraction.

## 7. Update CMake

- Add new C++ headers and sources to the app target.
- Add tests to the test target when needed.
- Add QML files to `qt_add_qml_module` only if new QML components are introduced.

## 8. Add Tests

- Use Qt Test.
- Use `QSignalSpy` for property and command signals.
- Test model role names, row counts, and data.
- Test validation, search, filter, and sort behavior in services.
- Use temporary data for storage tests if storage becomes part of the slice.

## 9. Verification

Run commands one at a time from `D:\Project_CV\Root`.

Normal build:

```powershell
cmake --preset windows-debug-local
```

```powershell
cmake --build --preset windows-debug-local
```

For logic changes or tests:

```powershell
cmake --preset windows-debug-tests-local
```

```powershell
cmake --build --preset windows-debug-tests-local
```

```powershell
ctest --preset windows-debug-tests-local
```

## 10. Final Review

- Confirm QML no longer owns moved business logic.
- Confirm `src/main.cpp` stayed minimal.
- Confirm no unrelated visual or style changes were made.
- List changed files.
- Report verification results.
- List manual UI checks.
- List risks and unfinished migration slices.
