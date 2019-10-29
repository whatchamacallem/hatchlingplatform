[![Generic badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/adrian3git/HatchlingPlatform)
[![GitHub version](https://badge.fury.io/gh/adrian3git%2FHatchlingPlatform.svg)](http://badge.fury.io/gh/adrian3git%2FHatchlingPlatform)
[![Build Status](https://travis-ci.org/adrian3git/HatchlingPlatform.svg?branch=master)](https://travis-ci.org/adrian3git/HatchlingPlatform)

# Hatchling Platform

Small C++ run-time intended to be developed against on the desktop before cross
compiling to an embedded target.  This is minimalist programming.  I wrote this for
myself to have on hand for bare metal projects but welcome feedback and patches.

 * `#include` [`<hx/hatchling.h>`](https://github.com/adrian3git/HatchlingPlatform/blob/master/include/hx/hatchling.h)

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

 * Includes a tiny and configurable `printf` from: https://github.com/mpaland/printf

  * 64-bit clean.  Intended for but not limited to use with a 32-bit
   target.  Memory allocation, DMA and File I/O use size_t, everything
   else is 32-bit to keep structure layouts predictable.

 * Does not use exceptions or `std::type_info.`

Tested using:
 * Microsoft Visual Studio Community 2017 Version 15.9.12
 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.10) 5.4.0 20160609
 * clang version 3.8.0-2ubuntu4
 * See `test.sh` 

Licensed under Apache License Version 2.0.
 * http://www.apache.org/licenses/
 * See `LICENSE.txt` for license terms.
