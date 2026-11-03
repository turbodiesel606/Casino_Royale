# Maintainability, Documentation, and Testing

### Consistent Code Style

Naming, formatting, and the organization of header files and includes follow agreed conventions. Consistency within a codebase is more valuable than mixing several acceptable styles; names communicate the purpose of the entities they identify.

**Details:** [C++ Core Guidelines — NL.8 — consistent naming style](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-name); [C++ Core Guidelines — NL.4 — consistent indentation](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-indent); [C++ Core Guidelines — source file organization](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-source).

### Documentation of Contracts and Design Rationale

Where possible, names and code structure explain what the code does without comments. Documentation clarifies contracts, invariants, nontrivial algorithms, and the reasons behind decisions, rather than restating obvious instructions.

**Details:** [C++ Core Guidelines — NL.1 — meaning expressed by code](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-comments); [C++ Core Guidelines — NL.2 — describing intent](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-comments-intent); [C++ Core Guidelines — I.5 — preconditions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-pre); [C++ Core Guidelines — I.7 — postconditions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-post).

### Independence of Production Code from Tests

Production code must not depend on tests, testing frameworks, test data, or special modes designed to pass checks. Dependencies run from tests to the code under test, not the other way around; building and running the application do not require test components. Test-specific workarounds do not determine business logic.

**Details:** [Qt 6 — standalone test executables](https://doc.qt.io/qt-6/qttestlib-tutorial1-example.html).

### Behavior Verification Through Independent Tests

Unit and integration tests verify expected contracts, including edge cases, errors, cancellation, and concurrency scenarios. Business logic separated from presentation can be tested without launching the GUI; regression tests capture expected behavior and detect unintended changes.

**Details:** [Qt 6 — Qt Test Best Practices](https://doc.qt.io/qt-6/qttest-best-practices.html); [C++ Core Guidelines — I.1 — explicit, testable interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-explicit); [Qt 6 — separating UI from business logic](https://doc.qt.io/qt-6/qtquick-bestpractices.html#separate-ui-from-business-logic).
