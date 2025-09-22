# Hatchling Platform

[![Generic
badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub
version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

<img src="hatchling_logo.png" alt="logo">

## Overview

> People say that you should not micro-optimize. But if what you love is
> micro-optimization... that's what you should do. — Linus Torvalds

Hatchling Platform is a lightweight C17/C++20 runtime library designed for
cross-compilation to resource-constrained targets. It maintains compatibility
with C99 libraries, requires only a C++11 compiler and deliberately avoids
dependencies on the C++ standard library. The developer experience is also
better than with the C++ standard library. For example the template compile
errors are easier to read and `hxassertmsg` will format your assert messages
before setting a breakpoint for you. There is nothing unnecessary to step
through in the debugger. The compilers budget for optimization isn't blown out
by layers you don't normally need.

A key strength of this codebase is its embrace of clang's Undefined Behavior
Sanitizer (UBSan), which enables developers to write pointer-centric C++ code
while enjoying runtime safety guarantees comparable to managed languages. The
implementation maintains compatibility with all warning flags and sanitizers for
both gcc and clang. Of course asserts are also widely used. The implementation
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
  embedded c99 library. musl libc is recommended for embedded Linux and is
  widely packaged: <https://musl.libc.org/> No other C++ runtime or C++ code is
  required. pthreads is used for threading which is a widely implemented
  standard. The asserts can work with only string hashes in release in order to
  provide basic debug facilities in environments too limited to debug in
  normally.

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
  exceptions and RTTI for efficiency. Exceptions will be caught by the test
  driver and the console if they are enabled.

- **C99 Compatibility**: Logging, asserts and memory management available in
  plain C99 via `<hx/hatchling.h>`

- **64-bit Ready**: Designed for both 32-bit and 64-bit targets.

## Documentation

Running the command `doxygen` with no args will generate `docs/html/index.html`.
The markdown source for the documentation is in the header files at
`include/hx/` and is readable as is. A modern editor should also show the docs
in a mouseover box.

Also see `AGENTS.md` for a human readable contributors guide also meant for AI.

## Tested Environments

Every compiler warning flag should be safe to enable. This is Ubuntu 24.04 LTS.
The Windows build got dropped because it wasn't being tested. It should be easy
to resurrect.

c99 c17 c++11 c++14 c++17 c++20
clang 18.1.3 (including sanitizers)
cmake 3.28.3
doxygen 1.9.8
emcc 4.0.5
gcc, musl-gcc 13.3.0
gcovr 7.0
python 3.12.3

## License

© 2017-2025 Adrian Johnston. This project is licensed under the terms of the MIT license found in the `LICENSE.md` file.

🐉🐉🐉
