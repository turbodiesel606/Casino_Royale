# Testing Policy

Run tests when a change affects:

- Business logic.
- Storage.
- Parsing.
- Algorithms.
- Cross-module contracts.
- High-risk runtime behavior.

Do not add formal tests only for appearance-level QML changes when no suitable test infrastructure exists.

Do not claim that code works unless it was verified.

## Current Test Setup

Tests are enabled through the `JOBTRACKER_BUILD_TESTS` CMake option and the `windows-debug-tests-local` preset.

Use:

```powershell
cmake --preset windows-debug-tests-local
```

```powershell
cmake --build --preset windows-debug-tests-local
```

```powershell
ctest --preset windows-debug-tests-local
```

If CTest reports that no tests were found, report that state clearly instead of treating it as a test failure.

## Backend Test Patterns

Use Qt Test for QObject, model, and signal behavior.

Use `QSignalSpy` when verifying property notify signals, command result signals, or model update signals.

For `QAbstractListModel` subclasses, test:

- Role names.
- Row counts.
- Data returned for each role.
- Insert, remove, reset, and update behavior when applicable.

For services and domain logic, test validation rules, parsing, filtering, sorting, grouping, search, and error handling.

For storage or file-system behavior, use temporary test data and avoid depending on machine-specific paths.

Name tests by the behavior they cover, and keep them registered through the project CMake test setup.

## Manual Checks

When automated validation is unavailable or not applicable, list the manual checks needed for the user to confirm the change.
