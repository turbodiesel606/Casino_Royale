# Error Handling

### A Consistent Error-Handling Strategy

Component contracts define which outcomes are normal, how errors are propagated, and where they are handled. An expected rejection under a business rule, operation cancellation, a system failure, and a program invariant violation are distinguished rather than collapsed into an uninformative generic result.

**Details:** [C++ Core Guidelines — E.1 — error-handling strategy](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design); [C++ Core Guidelines — E.4 — relating errors to invariants](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design-invariants).

### Exceptions for Errors, Not Ordinary Control Flow

In code that uses exceptions, they signal an inability to perform the task specified by the contract. Ordinary branch selection, loop termination, and other normal control-flow transitions are not modeled with exceptions.

**Details:** [C++ Core Guidelines — E.2 — the purpose of exceptions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-throw); [C++ Core Guidelines — E.3 — exceptions only for error handling](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-errors).

### Meaningful Error Handling and Propagation

An error is handled where recovery, a justified translation to another error representation, or reporting to the caller is possible. Empty `catch` blocks and loss of the failure cause are not substitutes for error handling; catching exceptions in every intermediate function is generally unnecessary.

**Details:** [C++ Core Guidelines — E.17 — where to catch exceptions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-not-always); [C++ Core Guidelines — E.18 — minimizing explicit try/catch](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-catch).
