# C++ and Qt 6: Best Practices

A concise topical reference to selected practices for modern C++ and Qt 6. Each practice provides an overview and links to detailed primary sources.

## Contents

| File | Topic |
| --- | --- |
| [01_Resource_Ownership_and_Lifetime.md](01_Resource_Ownership_and_Lifetime.md) | Resources, ownership, and lifetime |
| [02_Types_and_Interfaces.md](02_Types_and_Interfaces.md) | Types and interfaces |
| [03_Architecture_and_Dependencies.md](03_Architecture_and_Dependencies.md) | Architecture and dependencies |
| [04_Error_Handling.md](04_Error_Handling.md) | Error handling |
| [05_Concurrency_and_Asynchronous_Code.md](05_Concurrency_and_Asynchronous_Code.md) | Concurrency and asynchronous code |
| [06_Modern_CPP_and_Performance.md](06_Modern_CPP_and_Performance.md) | Modern C++ and performance |
| [07_Maintainability_and_Testing.md](07_Maintainability_and_Testing.md) | Maintainability, documentation, and testing |
| [08_Qt6_Best_Practices.md](08_Qt6_Best_Practices.md) | Qt 6: objects, threads, SQL, and QML |

## Sources and Scope

The primary sources are the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) and the [official Qt 6 documentation](https://doc.qt.io/qt-6/best-practices.html). Additional links point to the C++ Working Draft and explanations of SRP and YAGNI by their authors.

The descriptions are brief summaries of the practices, not verbatim reproductions of the sources. C++ Core Guidelines links include rule identifiers; Qt links point to the relevant documentation sections.

The language and standard library features mentioned are available in C++20 unless stated otherwise. The linked documentation may also describe newer features whose availability depends on the language or library version. In particular, `QSqlDatabase::moveToThread` requires Qt 6.8 or later.