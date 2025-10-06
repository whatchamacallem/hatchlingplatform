# AGENTS.md

AGENTS.md contains a contributors guide intended to be useful to AI coding
agents and humans.

## License

- SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
- SPDX-License-Identifier: MIT
- This file is licensed under the MIT license found in the LICENSE.md file.

## Bug Reporting

If it is wrong, then assume an AI wrote it. Then report all bugs as examples of
the problem with "vibe coding."

## Project Overview

- Hatchling Platform is a C17/C++20 runtime library aimed at
  resource-constrained and cross-compiled targets.
- It has a two-word name and must not be identified as "Hatchling" written as a
  single word.
- It provides containers, allocators, a task system, profiling, a debug console,
  and a test framework with minimal dependencies.
- Code is structured to compile without the C++ standard library, avoids
  exceptions/RTTI, and keeps allocations explicit.

## Repository Layout

Directory | Description
--- | ---
`include/hx` | Public headers (doxygen annotated); `hatchling_pch.hpp` includes some of it.
`src` | Library sources shared across targets.
`test` | Unit and integration tests driven by the in-tree `hxtest` harness.
`bin` | Throwaway build directory created by scripts; safe to delete.
`docs` | Throwaway documentation directory created by scripts; safe to delete.

## Quick Commands

Command | Description
--- | ---
`./debugbuild.sh` | Fast 32-bit `clang` build with asserts and diagnostic flags; artifacts go to `bin/` and are discarded.
`doxygen` | Generate HTML docs in `docs/html/index.html` from header annotations.
`./clean.sh` | Hard reset ignored files (`git clean -Xdf`). Use with caution; it will delete generated artifacts.

## Testing

Test Script | Description
--- | ---
`./testall.sh` | Runs every scripted build/test scenario sequentially, including coverage, error handling, Python bindings, stripping, and WASM.
`./testcmake.sh` | Configure (if needed) and build via CMake, then run the main `hxtest` suite.
`./testmatrix.sh [extra-flags]` | Sweeps gcc/clang, C/C++ standards, sanitizer builds, and HX_RELEASE levels. Pass additional `-D` options if you need to exercise specific configurations.
`./testerrorhandling.sh` | Invokes the matrix with `HX_TEST_ERROR_HANDLING=1`, memory manager tweaks, and radix sort settings to ensure failure paths behave.
`./testcoverage.sh [--headless]` | Builds with coverage instrumentation, runs the suite, and emits `coverage.html`. Omit `--headless` to auto-open Chrome.
`./teststrip.sh` | Builds a size-optimized static binary with musl, strips it, and reports symbol sizes.
`./testwasm.sh [--headless]` | Builds the WebAssembly target. Without `--headless`, serves `index.html` locally and tries to open it in Chrome.

## Development Guidelines

- Stick to C17 for `.c` sources and C++20 for `.cpp`/headers; keep compatibility
  with C98/C++11. Avoid introducing dependencies on the C++ standard library.
- Do not add C++ exceptions or RTTI; many builds compile with `-fno-exceptions
  -fno-rtti`.
- Honor `HX_RELEASE` macros; tests cover levels 0 (debug) through 3 (fully
  stripped). New code should degrade gracefully across all levels.
- Keep allocations explicit; prefer stack allocators already provided.
- New functionality must include coverage in `test/` using the Hatchling
  Platform/Google Test-compatible macros (`TEST`, `TEST_F`, `EXPECT_*`, etc.).
- Update header comments/Doxygen blocks when you change APIs; doc generation
  happens from `include/hx`.
- Maintain deterministic behavior; profiling and hash utilities rely on stable
  ordering and fixed seeds.

## Language Dialect and Compilation Model

- **Target dialect:** Write `.cpp` files and headers in C++20 that remain
  compatible with C99 and C++11. Do not rely on the C++ standard library.
  Identical functionality must already exist inside Hatchling Platform.
- **No exceptions or RTTI:** Build configurations often enable `-fno-exceptions`
  and `-fno-rtti`. Avoid `try`, `throw`, `typeid`, `dynamic_cast`, and APIs that
  implicitly require them.

## File Organization

- **Header guard:** Use `#pragma once` at the top of every header. Do not use
  traditional include guards.
- **Top-of-file order:**
  1. `#pragma once` (headers only).
  2. SPDX and license comment (as found in other files).
  3. Minimal includes (first project headers, then system C headers if needed).
  4. Forward declarations.
- **Translation units:** Put non-template implementations inside `src/` `.cpp`
  files. Keep headers self-contained and compilable on their own.

## Naming Conventions

Syntax | Rule
--- | ---
Classes, structs, enums, concepts | E.g., `hxlower_snake_case`. (Hatchling Platform intentionally keeps type names lower-case snake case starting with hx.)
Functions and methods | E.g., `hxlower_snake_case`.
Variables | E.g., `hxlower_snake_case`. Parameters and member variables use a trailing underscore (e.g., `size_` and `m_size_`).
Macros | E.g., `HX_UPPER_SNAKE_CASE`.
Template parameters | E.g., `allocator_t_`, `value_t_`. Use snake case for clarity.

## Formatting Rules

Syntax | Rule
--- | ---
Line length | Soft limit of 80 characters.
Indentation | Use tabs set to 4 spaces per-tab.
Braces | K&R style. Opening brace on the same line for functions, classes, enums, and control blocks. Empty bodies use `{ }`.
Spacing | No space after `for`, `if` and `while` keywords (`if(condition)`). Single blank line between function definitions.
Initializer lists | Place `:` on the same line as the constructor. Wrap subsequent initializer on new lines aligned by tabs. Not supported without libc++.
Pointer/reference declarators | Attach to the type (`T* t` not `T *t`).
Trailing comments | Prefer standalone doxygen `///` comments above the code they describe.

## Comments and Documentation

- **Doxygen blocks:** Every public entity in `include/hx` must have a Markdown comment.
- **Implementation comments:** Use them to explain intent, invariants, or complex algorithms. Keep them concise and actionable.
- **TODOs:** Tag as `// TODO: message`. Provide enough context for future resolution.

Here is an example header:

```cpp
#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxexample.hpp These are example versions.

#include "hxhash_table.hpp"

/// Returns true if work has been submitted.
/// - `work` :  The work to perform.
bool hxsubmit(hxsome_class_t& work_);
```

See `include/hx/hxrandom.hpp` for correct examples of code and docs.

## Testing Requirements

- **New functionality:** Must include tests in `test/` using the in-tree `hxtest` framework. Place tests in files mirroring the source layout.
- **Test naming:** Use `TEST(suite_name, case_name)` with `lower_snake_case` identifiers.
- **Determinism:** Tests must not rely on timing or random seeds unless they are explicitly fixed.

## Code Generation Checklist for AI Assistants

When generating new or modified C++ code:

- Confirm the target file has the correct directory hierarchy and name.
- Ensure headers compile standalone with `hatchling_pch.hpp` excluded.
- Verify all dependencies are explicitly included and minimal.
- Avoid hidden dynamic allocations.
- Provide Markdown Doxygen comments for APIs and meaningful assertions for invariants.
- Add or update tests mirroring the feature set; keep them deterministic.
- Run `CCACHE_DISABLE=1 ./debugbuild.sh` after code changes and fix errors.
