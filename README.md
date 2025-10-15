# Hatchling Platform

Please use the most recent tagged release.

<img src="hatchling_logo.png" alt="logo">

## Overview

> People say that you should not micro-optimize. But if what you love is
> micro-optimization... that's what you should do. — Linus Torvalds

Hatchling Platform is a lightweight C17/C++20 runtime library designed for
cross-compilation to resource-constrained targets. It maintains compatibility
with C99 libraries, requires only a C++11 compiler, and deliberately avoids
dependencies on the C++ standard library. The developer experience is also
better than with the C++ standard library. For example, the template compile
errors are easier to read, and `hxassertmsg` will format your assert messages
before setting a breakpoint for you. There is nothing unnecessary to step
through in the debugger. The compiler's budget for optimization isn't blown out
by layers you don't normally need.

A key strength of this codebase is its embrace of clang's Undefined Behavior
Sanitizer (UBSan), which enables developers to write pointer-centric C++ code
while enjoying runtime safety guarantees comparable to managed languages. The
implementation maintains compatibility with all warning flags and sanitizers for
both gcc and clang. Of course, asserts are also widely used. The implementation
also avoids dynamic allocations except when initializing system allocators.

<img src="hatchling_banner.jpg" alt="banner" width="400" height="400"
style="float: right; padding-right: 20px; padding-left: 20px;">

Build configurations are controlled via `HX_RELEASE`, which defines optimization
levels while allowing separate compiler optimization settings for debugging
purposes:

- **0**: Debug build with comprehensive diagnostics, asserts, and verbose
  strings (may exceed target constraints)
- **1**: Release build with critical asserts and warnings (suitable for internal
  `RelWithDebInfo` builds)
- **2**: Optimized release build with minimal strings and critical asserts only
  (for profiling and field diagnostics)
- **3**: Maximum optimization with no runtime checks (production releases only
  after thorough testing)

## Key Features

- **Portability**: Hatchling can easily be made to run on top of any old
  embedded C99 library. musl libc is recommended for embedded Linux and is
  widely packaged: <https://musl.libc.org/>. No other C++ runtime or C++ code is
  required. pthreads is used for threading, which is a widely implemented
  standard. The asserts can operate with only string hashes in release builds to
  provide basic debug facilities in environments too limited for normal
  debugging.

- **Profiling System**: Uses processor cycle sampling to create a hierarchical
  timeline capture compatible with Chrome's `about://tracing` viewer (navigation
  uses W, A, S, and D keys). One line of assembly may be needed for exotic
  hardware.

- **Memory Management**: RAII-based abstraction layer supporting various
  allocation strategies, particularly valuable for applications where crashing
  from memory fragmentation is unacceptable. If you have a lot of temporary
  allocations, this system reasonably offers 30% memory and 30% performance
  improvements with minor modifications to your code.

- **Debug Console**: Provides a tiny rudimentary command-line interface with
  automatic C++ function binding using templates. Useful for interactive target
  debugging without recompilation and also provides support for config files or
  configuration via the command-line. It is a little clunky, and an opcode
  interpreter would use even less space.

- **Testing Framework**: Safer, lighter, debuggable reimplementation of core
  Google Test functionality.

- **Task Queue**: An unopinionated task queue with a worker pool.

- **Containers**: Non-reallocating implementations of `vector`, `unordered_set`
  and `unordered_map`. These are `hxarray` and `hxhash_table`. Due to the
  cache-coherent and pre-allocated nature of this programming style, there is
  little need for more than an array class.

- **Algorithms**: `hxradix_sort` is provided for Θ(n) sorting. See
  `<hx/hxsort.h>` for comparison based sorting and lookup.

- **Performance Focus**: This is systems code. Everything has to be well
  optimized and cache-coherent without causing code bloat. This codebase avoids
  exceptions and RTTI for efficiency. Exceptions will be caught by the test
  driver and the console if they are enabled.

- **C99 Compatibility**: Logging, asserts, and memory management are available
  in plain C99 via `<hx/hatchling.h>`.

- **64-bit Ready**: Designed for both 32-bit and 64-bit targets.

- **AI Friendly**: Things with the same name as the standard generally work the
  same way as the standard. This means that AI has been trained on similar designs and will recognize this codebase and use it properly.

It is hard to compete with `printf`/`scanf` for code size and speed. Take a look
at the **[{fmt}](https://fmt.dev)** project for a micro-optimized version of
`std::format` that is as efficient.

## Documentation

Running the command `doxygen` with no arguments will generate
`docs/html/index.html`. The markdown source for the documentation is in the
header files at `include/hx/` and is readable as-is. A modern editor should also
show the docs in a mouseover box.

Also see `AGENTS.md` for a human-readable contributors' guide intended for both
humans and AI.

## Other Projects

- **[Embedded Template Library](https://github.com/ETLCPP/etl)** - The ETL is a
  header-only embedded containers and algorithms library. The ETL is larger and
  approaches the complexity of the standard library.
- **[{fmt}](https://fmt.dev)** - Has a micro-optimized version of `std::format`.
  Also has nice features like console colors and a fast `printf` too.
- **[musl libc](https://musl.libc.org/)** - This is the recommended C library
  for use with Hatchling Platform in a freestanding environment.

## Tested Environments

Every compiler warning flag should be safe to enable. The reference environment
is Ubuntu 24.04 LTS. The Windows build was dropped because it wasn't being
tested, but it should be easy to resurrect.

- c99 c17 c++11 c++14 c++17 c++20
- clang 18.1.3 (including sanitizers)
- cmake 3.28.3
- doxygen 1.9.8
- emcc 4.0.5
- gcc, musl-gcc 13.3.0
- gcovr 7.0

The scripted builds exercise the following toolchains, language modes, and
`HX_RELEASE` combinations:

| Script | Toolchain | Language Modes | `HX_RELEASE` | Notes |
| --- | --- | --- | --- | --- |
| `debugbuild.sh` | `clang`/`clang++` | C17, C++20 | 0 | 32-bit debug build with ccache and no exceptions/RTTI. |
| `testcmake.sh` | `cmake` + default compiler | Project defaults (C/C++) | 0 (default) | Configures Google Test build and runs `hxtest`. |
| `testcoverage.sh` | `gcc`, `g++` + `--coverage` | C99, C++20 | 0 | Enables `HX_TEST_ERROR_HANDLING=1` and emits `coverage.html`. |
| `testmatrix.sh` | `gcc`, `clang` (ASan/UBSan) | C99, C17, C++11, C++20 | 0-3 | Sweeps optimization levels and sets `HX_USE_THREADS=level`. |
| `teststrip.sh` | `musl-gcc` (static) | C17, C++11/14/17/20 | 3 | Size-focused static build with allocator/library stripping. |
| `testwasm.sh` | `emcc` | Emscripten defaults (Clang-based C/C++) | 0 (default) | WebAssembly build with allocator disabled and single-thread mode. |

Supporting scripts such as `clean.sh`, `diff.sh`, and `listsymbols.sh` manage
workspace cleanup, diff viewing, and symbol inspection without compiling new
artifacts.

## License

© 2017-2025 Adrian Johnston. This project is licensed under the terms of the MIT
license found in the `LICENSE.md` file.

🐉🐉🐉
