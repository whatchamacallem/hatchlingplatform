# Hatchling Platform

[![Generic
badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub
version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

<img src="hatchling_logo.png" alt="logo">

> "People say that you should not micro-optimize. But if what you love is
> micro-optimization... that's what you should do." — Linus Torvalds

Hatchling Platform is a lightweight C17/C++17 runtime library designed for
desktop development with simultaneous cross-compilation to resource-constrained
targets. The developer experience is also better than with the C++ standard
library. For example the template compile errors are easier to read and
hxassertmsg will format your assert messages before setting a breakpoint for
you. There is no unnecessary template library boilerplate to step through in the
debugger. The compilers budget for optimization isn't blown out by boilerplate
layers you don't normally need. The implementation carefully avoids dynamic
allocations except when initializing system allocators. It maintains
compatibility with C99 libraries, requires only a C++11 compiler and deliberately
avoids dependencies on the C++ standard library. A C++ project using this
platform should run equally well on your thermostat using a single megabyte of
RAM as in your web-browser or plugged into your Python back end.

If this all seems un-relatable I understand. However, I have seem professionally
written C++ codebases where the profiler showed we were spending 3% of our time
executing `vector::operator[]` with all optimizations turned on. And this was in a
setting where it made sense to spend weeks working on an optimization that would
shave 1% off. *I'm sorry to destroy everyone's ideals, but even the authors of
libclang wrote their own custom C++ container library.* The C++ language standard
also has special provisions for "freestanding" development without the standard
library and this is intended for that purpose.

This project serves as both a practical tool and a research platform for
exploring C++ standard library design principles, particularly focusing on core
runtime features. While recent C++ standards have addressed some concerns,
significant opportunities remain for optimization.

<img src="hatchling_banner.jpg" alt="banner" width="400" height="400"
style="float: right; padding-right: 20px; padding-left: 20px;">

A key strength of this codebase is its reliance on clang's Undefined Behavior
Sanitizer (UBSan), which enables developers to write pointer-centric C++ code
while enjoying runtime safety guarantees comparable to managed languages. The
implementation maintains compatibility with all possible warning flags and
sanitizers for both gcc and clang.

The `hx/hatchling.h` header requires C99 support. The codebase relies on
`stdint.h` for fixed-width integers and selectively incorporates C++17 features
and compiler intrinsics when available.

Build configurations are controlled via `HX_RELEASE`, which defines optimization
levels while allowing separate compiler optimization settings for debugging
purposes:

- **0**: Debug build with comprehensive diagnostics, asserts, and verbose
  strings (may exceed target constraints)
- **1**: Release build with critical asserts and warnings (suitable for internal
  "RelWithDebInfo" builds)
- **2**: Optimized release build with minimal strings and critical asserts only
  (for profiling and field diagnostics)
- **3**: Maximum optimization with no runtime checks (production releases only
  after thorough testing)

## Key Features

- **Portability**: Hatchling could easily be made to run on top of any old
  embedded c99 library. musl libc is recommended for embedded linux and is
  widely available: <https://musl.libc.org/> No other C++ runtime or C++ code is
  required. pthreads is used for threading which is a widely implemented
  standard. The assert framework can work with only string hashes in release in
  order to provide basic debug facilities in environments too limited to debug
  in normally.

- **Profiling System**: Uses processor cycle sampling to create a hierarchical
  timeline capture compatible with Chrome's `about://tracing` viewer (navigation
  uses WASD keys.) One line of assembly might be needed for exotic hardware.

- **Memory Management**: RAII-based abstraction layer supporting various
  allocation strategies, particularly valuable for applications where crashing
  from memory fragmentation is unacceptable. If you have a lot of temporary
  allocations then this system reasonably offers 30% memory and 30% performance
  improvements with minor modifications to your code.

- **Debug Console**: Provides a tiny rudimentary command line interface with
  automatic C++ function binding using templates. Useful for interactive target
  debugging without recompilation and also provides support for config files or
  configuration via the command line. It is a little clunky and an opcode
  interpreter would use even less space.

- **Testing Framework**: Safer lighter debuggable reimplementation of core
  Google Test functionality.

- **Task Queue**: An unopinionated task queue with a worker pool.

- **Containers**: Non-reallocating implementations of `vector`, `unordered_set`
  and `unordered_map`. These are `hxarray` and `hxhash_table`. Due to the cache
  coherent and pre-allocated nature of this kind of programming there isn't much
  need for more than an array class.

- **Algorithms**: `hxradix_sort` is provided for Θ(n) sorting. See `<hx/hxsort.h>`
  for comparison based sorting and lookup.

- **Performance Focus**: This is systems code. Everything has to be well
  optimized and cache coherent without causing code bloat. This code base avoids
  exceptions and RTTI for efficiency. Of course exceptions will be caught by the
  test driver and the console if they are enabled.

- **C99 Compatibility**: Logging, asserts and memory management available in
  plain C99 via `<hx/hatchling.h>`

- **64-bit Ready**: Designed for both 32-bit and 64-bit targets.

## Documentation

Running the command `doxygen` with no args will generate `docs/html/index.html`.
The markdown source for the documentation is in the header files at
`include/hx/` and is readable as is. A modern editor like vscode will also show
you the docs in a mouseover box.

## Tested Environments

Every compiler warning flag should be safe to enable. This is Ubuntu 24.04 LTS.
The Windows build got dropped because it wasn't being tested. It should be easy
to resurrect.

- gcc, musl-gcc 13.3.0
- clang 18.1.3 (including sanitizers)
- doxygen 1.9.8
- emcc 4.0.5
- gcovr 7.0
- cmake 3.28.3
- python 3.12.3
- c99 c17 c++11 c++14 c++17 c++20

## Remaining Tasks

- Add test coverage for hash table nodes.
- constexpr compatible asserts.
- Find and test realistic compile-time use cases for constexpr with this code.

## License

© 2017-2025 Adrian Johnston. This project is licensed under the terms of the LICENSE.md file.

🐉🐉🐉
