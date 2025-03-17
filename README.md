[![Generic badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

# Hatchling Platform

Small C++ run-time intended to be developed against on the desktop before cross
compiling to an embedded target.  This is minimalist programming.  I wrote this for
myself to have on hand for bare metal projects but welcome feedback and patches.

 * A lightweight streamlined reimplementation of Google Test.  This code base is
   exhaustively tested.

 * Profiling.  Captures a hierarchical time-line view with a minimum of
   overhead.  View the example `profile.json` file in Chrome's
   `about://tracing` view.

 * Memory Management.  Abstracts a range of allocation strategies behind an
   RAII interface.

 * DMA.  Cross platform DMA wrapper with validation.

 * Containers.  Provides a small non-reallocating subset of
   `std::vector` `std::allocator` and `std::unordered_{ map, set, multimap,
   multiset }.`

 * Command line based console with simple C++ function binding.
 
 * Task Queue.  Provides an object oriented interface to multi-threading.

 * Uses standard C99 headers as required.  With the exception of host
   implementations where threading and time headers are included.  And
   `<new>.`

 * Logging and memory management available in plain C99.

  * 64-bit clean.  Intended for but not limited to use with a 32-bit
   target.  Memory allocation, DMA and File I/O use size_t, everything
   else is 32-bit to keep structure layouts predictable.

 * Does not use exceptions or `std::type_info.`

Tested using:
 * Microsoft Visual Studio Community 2017 Version 15.9.12
 * gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
 * Ubuntu clang version 18.1.3 (1ubuntu1)
 * emcc (Emscripten 4.0.5)

 * See `test.sh` 

Licensed under Apache License Version 2.0.
 * http://www.apache.org/licenses/
 * See `LICENSE.txt` for license terms.
