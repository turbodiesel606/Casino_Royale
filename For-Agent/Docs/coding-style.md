# C++ And Qt Style

Use C++20 Standard where supported by the current compiler and where it does not reduce portability between Windows and Linux.

## Reuse, Generalization, And Best Practices

- Consolidate C++ functions, classes, and methods that duplicate the same logic, responsibility, and invariants into one canonical function, class, or method. Use overloads, inheritance (without virtuals), templates, and template specializations when they are necessary and technically appropriate to expose the canonical implementation without duplicating it.
- Use existing C++ classes, functions, and methods when solving a problem. Reuse a fully suitable entity directly. If an entity is only partially suitable, extend it through an overload, template, or specialization when appropriate. Create a new entity only when the existing entities are not suitable for the responsibility.
- Follow established modern C++ best practices for correctness, type safety, ownership, lifetime and resource management, exception safety, clarity, maintainability, portability, and testability.
- Treat testability as an outcome of clear production responsibilities, not as permission to reshape production for tests. Do not add or widen production APIs, weaken access control, introduce test-only hooks or compile definitions, or alter ownership, lifetime, threading, behavior, or architectural boundaries solely for tests. Keep test support under `tests/`.

## Header Conventions

Use `#ifndef` / `#define` include guards instead of `#pragma once` in C++ header files.

Use the `.hpp` extension for all C++ header files.

## Naming

Use trailing underscores for user-defined class and struct fields:

```cpp
class Example {
private:
    QString name_;
    std::vector<int> data_;
};
```

Use clear, project-local names over generic names.

## Parameters And Ownership

Use references for required non-owning parameters when appropriate.

Use pointers when pointer semantics are more correct:

- Qt parent-child ownership.
- `QObject` lifetimes.
- Nullable dependencies.
- Polymorphism.
- Signal/slot integration.
- Model/view APIs that conventionally use pointers.

Do not hide ownership transfer. Prefer explicit ownership through Qt parentage or standard C++ ownership types.

## Code Organization

Keep large classes, models, services, controllers, and helpers in focused file pairs when that improves maintainability.

Use `.hpp` / implementation file pairs for C++ code instead of `.h`-based pairs.

Prefer small cohesive files over god files and god classes.

Document non-trivial classes, functions, data structures, and architectural decisions.

Update comments when related code changes make old comments inaccurate.

If code logic has not changed, do not change comments for that code.

Do not add unrelated refactoring to task-focused changes.

## Initialization

Use curly braces for object constructors.
