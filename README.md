[![Generic badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

# Hatchling Platform

Small C++ run-time intended to be developed against on the desktop before cross
compiling to an embedded target with limited RAM. Does not make allocations
before main or on test failure.

 * A lightweight streamlined reimplementation of Google Test. Allows running
   CI tests on a development board.

 * Profiling. Captures a hierarchical time-line view with a minimum of
   overhead. Generates output that can be viewed with Chrome's
   `about://tracing` view. (Use the WASD keys to move around.)

 * Memory Management. Abstracts a range of allocation strategies behind an
   RAII interface. This is intended for targets where memory fragmentation
   is not allowed. This may save up to 30% of memory and 30% execution time.

 * DMA. Cross platform DMA wrapper which allows validation on the host before
   deploying to a target. Provides profiling information.

 * Containers and algorithms. Provides a small non-reallocating subset of
   `std::vector` `std::allocator` and `std::unordered_{ map, set, multimap,
   multiset }.`  These are integrated with the memory manager.

 * Command line based console with simple C++ function binding. Allows debugging
   on the target without relinking and relaunching.

 * Task Queue. Provides an object oriented interface to multi-threading. The
   default implementation uses `<thread>` and allows for target specific
   implementations.

 * Uses the minimum necessary standard C99 headers as required. With the
   exception of host implementations where threading and time headers are
   included. Also `<new>` is used for placement new.

 * Logging and memory management available in plain C99. `<hx/hatchling.h>` is
   available in C and C++.

 * 64-bit clean. Intended for but not limited to use with a 32-bit
   target. size_t is fairly widely used.

 * Does not use exceptions or `std::type_info` as these are inefficient.

 * See `LICENSE.txt` for license terms.

Tested using:
 * Microsoft Visual Studio Community 2022
 * gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
 * Ubuntu clang version 18.1.3 (1ubuntu1) x86_64-pc-linux-gnu
 * emcc (Emscripten 4.0.5)
 * valgrind-3.22.0 (tested in 64-bit mode.)
 * See `test.sh`
