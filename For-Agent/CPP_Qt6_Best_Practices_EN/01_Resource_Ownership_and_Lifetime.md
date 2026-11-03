# Resources, Ownership, and Lifetime

### RAII (Resource Acquisition Is Initialization)

Resource management is tied to object lifetime: memory, files, and locks are released automatically when their owner is destroyed, including during stack unwinding caused by an exception. Resource-managing types and RAII wrappers, such as `std::unique_ptr`, `std::lock_guard`, and `std::scoped_lock`, provide this behavior.

**Details:** [C++ Core Guidelines — R.1 — RAII](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-raii); [C++ Core Guidelines — CP.20 — managing locks through RAII](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-raii).

### Rule of Zero

Whenever possible, a class uses members that manage their own resources. If their default semantics are appropriate, custom destructors, copy operations, and move operations are unnecessary: the member types and the compiler provide the required behavior.

**Details:** [C++ Core Guidelines — C.20 — Rule of Zero](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-zero).

### Explicit Ownership with the Simplest Sufficient Model

Ordinary objects and storage by value are preferred over unnecessary dynamic allocation. When dynamic exclusive ownership is required, `std::unique_ptr` is appropriate; `std::shared_ptr` is intended for genuinely shared ownership, not merely shared access to an object.

**Details:** [C++ Core Guidelines — R.5 — scoped objects](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-scoped); [C++ Core Guidelines — R.20 — representing ownership](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-owner); [C++ Core Guidelines — R.21 — unique_ptr and shared_ptr](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-unique).

### Non-Owning Access

Raw pointers and references are appropriate for access without ownership transfer. The owner remains responsible for destroying the object, and the object's lifetime must cover all accesses through these pointers and references.

**Details:** [C++ Core Guidelines — R.3 — non-owning pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ptr); [C++ Core Guidelines — R.4 — non-owning references](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ref).
