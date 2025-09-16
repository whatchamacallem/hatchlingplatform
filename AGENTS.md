# AGENTS.md

AGENTS.md contains context that AI coding agents need.

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

## Contribution workflow

- Please ensure `./testall.sh` passes before submitting.
