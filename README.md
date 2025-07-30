# Hatchling Platform

[![Generic badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

<br/><img src="hatchling_logo.png" alt="logo" style="display: block; margin: auto;"><br/>

<div style="width: 90%; margin: 0 auto; text-align: justify; font-size: 120%;">

Hatchling Platform is a lightweight C17/C++17 runtime library designed for desktop development with eventual cross-compilation to resource-constrained embedded targets. The implementation carefully avoids dynamic allocations except when initializing system allocators. It maintains compatibility with C99 libraries, requires only a C++11 compiler, and deliberately avoids dependencies on the C++ standard library. A C++ project using this platform should run equally well on your thermostat using a single megabyte of RAM as in your web-browser or plugged into your Python back end.

This project serves as both a practical tool and a research platform for exploring C++ library design principles, particularly focusing on core runtime features that might inform future realtime systems standards. While recent C++ standards have addressed some concerns, significant opportunities remain for optimization.

<img src="hatchling_banner.jpg" alt="banner" width="400" height="400"
style="float: right; padding-right: 20px; padding-left: 20px;">

A key strength of this codebase is its integration with clang's Undefined Behavior Sanitizer (UBSan), which enables developers to write pointer-centric C++ code while enjoying runtime safety guarantees comparable to managed languages. This approach combines C++'s raw power with modern safety features. The implementation maintains strict compatibility with all major warning flags and sanitizers across gcc, clang, and MSVC, with rigorous type safety throughout.

The `hx/hatchling.h` header requires C99 support. The codebase relies on `stdint.h` for fixed-width integers and selectively incorporates C++17 features and compiler intrinsics when available.

Build configurations are controlled via `HX_RELEASE`, which defines optimization levels while allowing separate compiler optimization settings for debugging purposes:

- **0**: Debug build with comprehensive diagnostics, asserts, and verbose strings (may exceed target constraints)
- **1**: Release build with critical asserts and warnings (suitable for internal "RelWithDebInfo" builds)
- **2**: Optimized release build with minimal strings and critical asserts only (for profiling and field diagnostics)
- **3**: Maximum optimization with no runtime checks (production releases only after thorough testing)

</div><br/>
<div style="width: 80%; margin: 0 auto; text-align: justify; font-size: 120%;">

## Key Features

- **Testing Framework**: Lightweight reimplementation of Google Test functionality, enabling CI testing on development boards

- **Profiling System**: Low-overhead hierarchical timeline capture compatible with Chrome's `about://tracing` viewer (navigation with WASD keys)

- **Memory Management**: RAII-based abstraction layer supporting various allocation strategies, particularly valuable for systems where memory fragmentation is unacceptable (potential 30% memory/performance improvements)

- **Containers & Algorithms**: Non-reallocating implementations of essential std components (`vector`, `allocator`, unordered associative containers) integrated with memory management

- **Debug Console**: Command-line interface with C++ function binding for target debugging without recompilation (allocation-free implementation)

- **Task Queue**: OO multi-threading interface with `<thread>`-based default implementation and support for platform-specific variants

- **C99 Compatibility**: Logging, asserts and memory management available in plain C99 via `<hx/hatchling.h>`

- **64-bit Ready**: Designed for but not limited to 32-bit targets, `size_t` is used a lot.

- **Performance Focus**: Avoids exceptions and RTTI for efficiency. Exceptions will be caught by the test driver and the console if they are enabled.

- **License**: See `LICENSE.txt` for terms

## Tested Environments

Every compiler warning flag should be safe to enable. This is Ubuntu 24.04 LTS. The Windows
build got dropped because it wasn't being tested. It should be easy to resurrect.

- gcc, musl-gcc 13.3.0
- clang 18.1.3 (including sanitizers)
- emcc 4.0.5
- gcovr 7.0
- cmake 3.28.3
- python 3.12.3
- c99 c17 c++11 c++14 c++17

</div>
