# Architecture and Dependencies

### Single Responsibility Principle — SRP

A class or component has a cohesive responsibility and corresponding reasons to change; a function performs one logical operation. Unrelated responsibilities, such as data presentation, business rules, and storage access, are not mixed unnecessarily.

**Details:** [Robert C. Martin — Single Responsibility Principle](https://blog.cleancoder.com/uncle-bob/2014/05/08/SingleReponsibilityPrinciple.html); [C++ Core Guidelines — F.2 — one logical operation](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-logical).

### Encapsulation and a Minimal Public API

The interface is separated from the implementation and exposes only the necessary operations. Internal state and helper methods are not made public solely for convenient access from other components.

**Details:** [C++ Core Guidelines — C.3 — interface and implementation](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-interface); [C++ Core Guidelines — C.9 — limiting exposure](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-private).

### Explicit Dependencies and Loose Coupling

Required dependencies are passed explicitly, and hidden global mutable state is minimized. Components depend on the contracts they need, while additional abstractions are introduced only when they provide clear practical value.

**Details:** [C++ Core Guidelines — I.1 — explicit interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-explicit); [C++ Core Guidelines — I.2 — global mutable state](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global); [C++ Core Guidelines — C.3 — separating implementation](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-interface).

### DRY — Eliminating Duplication of the Same Logic

Repeating the same rule or algorithm in multiple places makes maintenance harder. A shared implementation is justified when the meaning and reasons to change are the same; superficially similar code with different responsibilities does not necessarily need to be combined.

**Details:** [C++ Core Guidelines — ES.3 — eliminating redundant code](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-dry); [Robert C. Martin — responsibility boundaries](https://blog.cleancoder.com/uncle-bob/2014/05/08/SingleReponsibilityPrinciple.html).

### KISS — Simplicity of the Solution

The simplest, clearest solution that meets actual requirements is preferred. Complex constructs, additional layers of indirection, and excessive fragmentation are not advantages in themselves.

**Details:** [C++ Core Guidelines — F.3 — simple functions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-single); [C++ Core Guidelines — P.1 — expressing ideas directly](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-direct).

### YAGNI — Avoiding Speculative Functionality

Features, extension points, and general-purpose mechanisms are not added solely to address hypothetical future needs. Current requirements determine the necessary scope of the solution; this principle does not negate the importance of a clear structure and maintainability.

**Details:** [Martin Fowler — YAGNI](https://martinfowler.com/bliki/Yagni.html).
