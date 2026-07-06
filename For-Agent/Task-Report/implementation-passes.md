# QML To C++ Implementation Passes

This file breaks the QML-to-C++ migration into practical implementation passes.

## Pass 1. Inventory And Contracts Only

- Inspect QML business logic.
- List migration candidates.
- Decide what stays in QML.
- Define the first C++ contracts.
- Do not migrate behavior yet unless a tiny preparatory change is explicitly approved.

## Pass 2. Bootstrap Foundation

- Make application bootstrap cleaner and exception-safe.
- Keep `src/main.cpp` minimal.
- Add a bootstrap or wiring class if startup setup has grown enough to justify it.
- Run the normal build.

## Pass 3. First Domain Migration

- Move one central area, probably job applications, from QML logic to C++ model/controller/service code.
- Update QML to consume C++ state and commands.
- Add tests where useful.
- Run build and test verification.

## Pass 4. CV Library And CV Linkage

- Move CV-related data and behavior into C++.
- Move CV linkage rules, selected CV behavior, and related computed state.
- Preserve visibility of which CV was used for which vacancy.

## Pass 5. Companies And Contacts

- Move company and contact data into C++.
- Move relationship logic into services or controllers.
- Add models/controllers if the screens need them.

## Pass 6. Shared Filtering, Search, Sort, And Validation

- Consolidate repeated rules into C++ services.
- Remove duplicate QML JavaScript helpers.
- Add focused tests for shared behavior.

## Pass 7. Cleanup And Review

- Remove leftover mock or business data from QML.
- Tighten QML-facing APIs.
- Run a full build and test pass.
- Produce the final manual UI checklist, risks, and remaining limitations.
