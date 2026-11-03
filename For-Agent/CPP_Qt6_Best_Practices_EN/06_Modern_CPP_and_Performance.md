# Modern C++ and Performance

### Modern Features for Clarity

`auto`, range-based `for`, and structured bindings are used where they reduce redundancy and express intent more clearly. Newer or shorter syntax is not, by itself, a reason to complicate otherwise understandable code.

**Details:** [C++ Core Guidelines — ES.11 — auto and structured bindings](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-auto); [C++ Core Guidelines — ES.71 — range-based for](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-range); [C++ Core Guidelines — P.3 — expressing intent](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-what).

### Purposeful Move Semantics

Moving is used when transferring resources or consuming a value, where this matches the operation's contract. `std::move` is not a universal optimization and does not guarantee that a move actually occurs; its use accounts for any subsequent use of the source object.

**Details:** [C++ Core Guidelines — ES.56 — justified use of std::move](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-move); [C++ Core Guidelines — F.18 — consumed parameters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-consume).

### Appropriate Containers and Algorithms

Standard algorithms and `std::ranges` facilities are preferred over custom equivalents when they directly express the required operation. Containers are selected according to access patterns, data modifications, and operation costs, rather than habit or assumptions about speed.

**Details:** [C++ Core Guidelines — ES.1 — using the standard library](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-lib); [C++ Core Guidelines — SL.con.2 — container selection](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-vector); [C++ Working Draft — ranges](https://eel.is/c++draft/ranges).

### Performance Supported by Measurements

Unnecessary copies, allocations, and locks are eliminated without speculative implementation complexity. Optimization priorities and results are determined through profiling and measurement; additional code complexity is justified by demonstrated benefits.

**Details:** [C++ Core Guidelines — P.9 — avoiding unnecessary costs](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-waste); [C++ Core Guidelines — Per.1 — justified optimization](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-reason); [C++ Core Guidelines — Per.6 — measuring performance](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-measure).
