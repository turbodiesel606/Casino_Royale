# Concurrency and Asynchronous Code

### Minimal Shared Mutable State

Shared access to mutable data requires consistent synchronization and freedom from data races. Passing messages, independent values, and immutable data is preferable to excessive shared mutation of objects.

**Details:** [C++ Core Guidelines — CP.2 — avoiding data races](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-races); [C++ Core Guidelines — CP.3 — minimizing shared mutable state](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-data); [C++ Core Guidelines — CP.31 — passing values between threads](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-data-by-value).

### Safe Locking and Short Critical Sections

Locks are managed by RAII objects, and the protected region is limited to work that genuinely requires synchronization. When acquiring multiple mutexes, a consistent deadlock-avoiding approach is used, such as `std::scoped_lock`.

**Details:** [C++ Core Guidelines — CP.20 — RAII for locking](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-raii); [C++ Core Guidelines — CP.21 — acquiring multiple mutexes](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lock); [C++ Core Guidelines — CP.43 — time spent in a critical section](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-time).

### Atomicity of Related State Updates

Individual `std::atomic` members do not automatically make a sequence of related actions atomic. The synchronization mechanism is selected for the entire invariant being protected: consistent updates to multiple values often require a common mutex or a specifically designed atomic protocol.

**Details:** [C++ Core Guidelines — CP.50 — data and the mutex that protects it](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-mutex); [C++ Working Draft — ordering and consistency of atomic operations](https://eel.is/c++draft/atomics.order).

### Lifetime, Cancellation, and Completion of Background Operations

Objects and data remain valid until all accesses from a background operation have finished, including accesses through captured references and `this`. Cancellation follows a defined protocol and does not imply an immediate stop; thread completion is coordinated with the destruction of dependencies.

**Details:** [C++ Core Guidelines — F.53 — lifetime of captured data](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-value-capture); [C++ Core Guidelines — CP.23 — scoped thread lifetime](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-join); [C++ Working Draft — jthread and stop requests](https://eel.is/c++draft/thread.jthread.class).
