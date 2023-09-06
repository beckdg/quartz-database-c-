# Coding Guidelines

## Language and Standard

- C++17 (no C++20 or later features)
- Standard Library only — no external runtime dependencies
- Exceptions may be used in public API entry points but not for internal control flow

## Naming Conventions

| Category               | Convention          | Examples                       |
|------------------------|---------------------|--------------------------------|
| Namespaces             | snake_case          | `quartz::file`, `quartz::endian` |
| Classes / Structs      | PascalCase          | `ScopeGuard`, `FileBackend`    |
| Free functions         | camelCase           | `isLittleEndian()`, `readLE32()` |
| Member functions       | camelCase           | `ok()`, `setLevel()`, `toString()` |
| Member variables       | snake_case_         | `stream_`, `code_`, `active_`  |
| Function parameters    | camelCase           | `oldPath`, `newPath`           |
| Constants / Enums      | PascalCase          | `Code::IOError`, `Level::Info` |
| Macros (rare)          | UPPER_SNAKE_CASE    | `QUARTZ_ASSERT`, `QUARTZ_SCOPE_EXIT` |

## Include Order

Within each source file, includes appear in this order, separated by blank lines:

1. Associated header (the `.h` for this `.cpp`)
2. C++ Standard Library headers
3. Project headers (`quartz/...`)
4. Third-party library headers

Within each group, includes are sorted lexicographically.

### Example

```cpp
#include "quartz/common/Status.h"

#include <cstdint>
#include <string>
#include <vector>

#include "quartz/common/Logger.h"
#include "quartz/util/Endian.h"

#include <catch2/catch_test_macros.hpp>
```

## Class Design Philosophy

- Favor composition over inheritance
- Make classes non-copyable / non-movable by default unless they represent a value type
- Use `explicit` on single-argument constructors
- Mark overriding functions with `override`
- Prefer `constexpr` and `noexcept` where applicable
- Keep classes small and focused on a single responsibility

## Error Handling

- Use `quartz::Status` for recoverable errors
- Assertions via `QUARTZ_ASSERT` for programming errors and invariant violations
- Avoid `throw` in library internals
- The top-level public API may use exceptions for exceptional conditions (e.g., out-of-memory)

## RAII Rules

- Every resource acquisition must be immediately wrapped in an owning object
- Destructors must not throw
- Use `ScopeGuard` for ad-hoc cleanup when a dedicated RAII wrapper is overkill

## Const Correctness

- Mark member functions `const` whenever they do not modify the observable state of the object
- Pass parameters by `const&` when the callee does not need a copy
- Return `const&` for accessors that expose internal state (avoid returning pointers to mutable internals)

## Formatting

- Use 4 spaces for indentation (no tabs)
- 120 character column limit
- Opening braces on the same line for control statements, on the next line for function/class definitions
- Spaces inside parentheses for control flow keywords (`if (...)`, `for (...)`, `while (...)`)
- No spaces inside parentheses for function calls (`func(a, b)`)
- Stick to the style established in the existing codebase

## File Structure

Each header file should follow this template:

```cpp
#pragma once

// Standard library includes
// ...

// Project includes
// ...

namespace quartz {

class MyClass {
public:
    // ...
private:
    // ...
};

} // namespace quartz
```
