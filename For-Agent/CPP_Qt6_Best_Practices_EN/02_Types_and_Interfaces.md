# Types and Interfaces

### Const Correctness

`const` expresses data and access that are not intended to allow modification. Where possible, methods that do not change an object's observable state are declared `const`; this clarifies the interface contract and limits accidental changes.

**Details:** [C++ Core Guidelines — Con.1 — immutability](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-immutable); [C++ Core Guidelines — Con.2 — const member functions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-fct); [C++ Core Guidelines — Con.3 — access through const](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-ref).

### Types That Express Meaning

Interfaces use types that reflect the purpose of values rather than ambiguous combinations of primitive types. `enum class` is preferred for enumerations, and related data is grouped into meaningful structures or classes.

**Details:** [C++ Core Guidelines — I.4 — strongly typed interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-typed); [C++ Core Guidelines — C.1 — grouping related data](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-org); [C++ Core Guidelines — Enum.3 — enum class](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-class).

### Optional Values and Alternatives

`std::optional<T>` explicitly represents the presence or absence of a value. `std::variant` represents a value of one of a predefined set of types; these models are preferable to ambiguous sentinel values and implicit state conventions.

**Details:** [C++ Working Draft — optional](https://eel.is/c++draft/optional); [C++ Working Draft — variant](https://eel.is/c++draft/variant); [C++ Core Guidelines — I.4 — expressing meaning through types](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-typed).

### Safe Type Conversions

Unnecessary and unsafe casts are avoided. When an explicit cast is genuinely required, a named C++ cast makes the intention visible, but does not by itself guarantee that the conversion is correct.

**Details:** [C++ Core Guidelines — ES.48 — minimizing casts](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts); [C++ Core Guidelines — ES.49 — named casts](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts-named).

### Parameter Passing According to Intent

Input values that are cheap to copy are generally passed by value, more expensive read-only values by `const T&`, and mutable input/output parameters by `T&`. Ownership transfer or consumption of a value is expressed separately and explicitly.

**Details:** [C++ Core Guidelines — F.16 — input parameters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-in); [C++ Core Guidelines — F.17 — input/output parameters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inout); [C++ Core Guidelines — F.18 — consuming a value](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-consume).

### Non-Owning Data Views

`std::span` describes a contiguous sequence without ownership, while `std::string_view` provides a read-only string view. Their use requires the underlying data to remain valid for every access: a view does not extend the lifetime of the backing storage.

**Details:** [C++ Core Guidelines — F.24 — range-based interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-range); [C++ Core Guidelines — SL.str.2 — string views](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-view); [C++ Working Draft — std::span](https://eel.is/c++draft/span.overview).
