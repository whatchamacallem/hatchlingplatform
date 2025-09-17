# AGENTS.md

AGENTS.md contains context intended to be useful to AI coding agents.

## Project overview

- Hatchling Platform is a C17/C++20 runtime library aimed at resource-constrained and cross-compiled targets.
- Provides containers, allocators, a task system, profiling, a debug console, and a test framework with minimal dependencies.
- Code is structured to compile without the C++ standard library, avoids exceptions/RTTI, and keeps allocations explicit.

## Repository layout

- `include/hx`: Public headers (doxygen annotated); `hatchling_pch.hpp` includes everything.
- `src`: Library sources shared across targets.
- `test`: Unit and integration tests driven by the in-tree `hxtest` harness.
- `bin`: Throwaway build directory created by scripts; safe to delete.
- `docs`: Throwaway documentation directory created by scripts; safe to delete.

## Quick commands

- `./debugbuild.sh`: Fast 32-bit clang build with asserts and diagnostic flags; artifacts go to `bin/` and are discarded.
- `./testcmake.sh`: Configure (if needed) and build via CMake, then run the main `hxtest` suite.
- `doxygen`: Generate HTML docs in `docs/html/index.html` from header annotations.
- `./clean.sh`: Hard reset ignored files (`git clean -Xdf`). Use with caution; it will delete generated artifacts.

## Testing

- `./testall.sh`: Runs every scripted build/test scenario sequentially, including coverage, error handling, Python bindings, stripping, and WASM.
- `./testmatrix.sh [extra-flags]`: Sweeps gcc/clang, C/C++ standards, sanitizer builds, and HX_RELEASE levels. Pass additional `-D` options if you need to exercise specific configurations.
- `./testerrorhandling.sh`: Invokes the matrix with `HX_TEST_ERROR_HANDLING=1`, memory manager tweaks, and radix sort settings to ensure failure paths behave.
- `./testcoverage.sh [--headless]`: Builds with coverage instrumentation, runs the suite, and emits `coverage.html`. Omit `--headless` to auto-open Chrome.
- `./testpythonbindings.sh`: Spins up a venv, installs the `entanglement_example` package in editable mode, and runs its tests (requires `clang.cindex`).
- `./teststrip.sh`: Builds a size-optimized static binary with musl, strips it, and reports symbol sizes.
- `./testwasm.sh [--headless]`: Builds the WebAssembly target. Without `--headless`, serves `index.html` locally and tries to open it in Chrome.

## Development guidelines

- Stick to C17 for `.c` sources and C++20 for `.cpp`/headers; keep compatibility with C98/C++11. Avoid introducing dependencies on the C++ standard library.
- Do not add C++ exceptions or RTTI; many builds compile with `-fno-exceptions -fno-rtti`.
- Honor `HX_RELEASE` gates—tests cover levels 0 (debug) through 3 (fully stripped). New code should degrade gracefully across all levels.
- Keep allocations explicit; prefer stack allocators already provided.
- New functionality must include coverage in `test/` using the Hatchling/Google Test-compatible macros (`TEST`, `TEST_F`, `EXPECT_*`, etc.).
- Update header comments/Doxygen blocks when you change APIs; doc generation happens from `include/hx`.
- Maintain deterministic behavior; profiling and hash utilities rely on stable ordering and fixed seeds.


## Language Dialect and Compilation Model

- **Target dialect:** Write `.cpp` files and headers in C++20 that remain compatible with C99 and C++11. Do not rely on the C++ standard library. Identical functionality must already exist inside Hatchling Platform.
- **No exceptions or RTTI:** Build configurations often enable `-fno-exceptions` and `-fno-rtti`. Avoid `try`, `throw`, `typeid`, `dynamic_cast`, and APIs that implicitly require them.

## File Organization

- **Header guard:** Use `#pragma once` at the top of every header. Do not use traditional include guards.
- **Top-of-file order:**
  1. `#pragma once` (headers only).
  2. SPDX and license comment (if present in neighboring files).
  3. Minimal includes (first project headers, then system C headers if needed).
  4. Forward declarations.
- **Translation units:** Put non-template implementations inside `src/` `.cpp` files. Keep headers self-contained and compilable on their own.

## Naming Conventions

- **Classes, structs, enums, concepts:** `hxlower_snake_case`. (Hatchling intentionally keeps type names lower-case.)
- **Functions and methods:** `hxlower_snake_case`.
- **Variables:** `hxlower_snake_case`. Parameters and member variables use a trailing underscore (e.g., `size_`).
- **Macros:** `HX_UPPER_SNAKE_CASE`. Avoid new macros unless necessary.
- **Template parameters:** `allocator_t`, `value_t`. Use snake case for clarity.

## Formatting Rules

- **Line length:** Soft limit of 80 characters.
- **Indentation:** Use tabs set to 4 spaces per-tab.
- **Braces:** K&R style. Opening brace on the same line for functions, classes, enums, and control blocks. Empty bodies use `{ }`.
- **Spacing:**
  - Space after keywords (`if (condition)`).
  - Single blank line between function definitions.
- **Initializer lists:** Place `:` on the same line as the constructor. Wrap subsequent initializers on new lines aligned by two spaces.

```cpp
widget::widget(i32 width, i32 height)
  : width_(width),
    height_(height) { }
```

- **Pointer/reference declarators:** Attach to the type (`allocator* arena` not `allocator *arena`).
- **Trailing comments:** Prefer standalone doxygen /// comments above the code they describe.

## Comments and Documentation

- **Doxygen blocks:** Every public entity in `include/hx` must have a markdown comment.
- **Implementation comments:** Use them to explain intent, invariants, or complex algorithms. Keep them concise and actionable.
- **TODOs:** Tag as `// TODO: message`. Provide enough context for future resolution.

```cpp
/// Returns true if work has been scheduled onto an executor.
/// - `work` :  The task to run. Ownership transfers to the scheduler.
bool submit(task_t work);
```

## Testing Requirements

- **New functionality:** Must include tests in `test/` using the in-tree `hxtest` framework. Place tests in files mirroring the source layout.
- **Test naming:** Use `TEST(suite_name, case_name)` with lower_snake_case identifiers.
- **Determinism:** Tests must not rely on timing or random seeds unless they are explicitly fixed.

## Code Generation Checklist for AI Assistants

When generating new or modified C++ code:

1. Confirm the target file has the correct directory hierarchy and name.
2. Ensure headers compile standalone with `hatchling_pch.hpp` excluded.
3. Verify all dependencies are explicitly included and minimal.
4. Avoid hidden dynamic allocations.
5. Provide markdown Doxygen comments for APIs and meaningful assertions for invariants.
6. Add or update tests mirroring the feature set; keep them deterministic.
7. Run `./debugbuild.sh` after code changes and fix errors.
