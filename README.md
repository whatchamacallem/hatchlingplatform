[![Build Status](https://travis-ci.org/adrian3git/HatchlingPlatform.svg?branch=master)](https://travis-ci.org/adrian3git/HatchlingPlatform)
[![codecov](https://codecov.io/gh/adrian3git/HatchlingPlatform/branch/master/graph/badge.svg)](https://codecov.io/gh/adrian3git/HatchlingPlatform)


The Hatchling Platform is designed to facilitate C++ embedded systems
development on a target too constrained to provide traditional operating
system services.  This is a narrowly defined platform intended to be
developed against in a modern desktop development environment before
cross compiling to an embedded system.  An approach intended to limit
debugging on the target.  It is also a replacement C++ runtime, intended
to largely replace the use of the standard C++ libraries.

 * Test Driver.  A partial lightweight reimplementation of GoogleTest.

 * Profiling.  Captures a hierarchical time-line view with a minimum of
   overhead.  View the example etc/profile.json file in Chrome's
   about://tracing view.

 * Memory Management.  Hides a range of allocation strategies behind a
   simple RAII interface.

 * DMA.  Cross platform DMA with validation.

 * Container Support.  Provides a small non-reallocating subset of
   std::vector, std::allocator and std::unordered_{map,multimap,set,
   multiset}.

 * Command line based console with simple C++ function binding.
 
 * Task Queue.  Simple object oriented interface to multi-threading.

 * 64-bit clean.  Intended for but not limited to use with a 32-bit
   target.  Memory allocation, DMA and File I/O use size_t, everything
   else is 32-bit.

 * Does not use exceptions or std::type_info.

 * Uses standard C99 headers as required.  With the exception of host
   implementations where threading and time headers are included.  And
   <new>.

 * Logging and memory management available in plain C99.  See hatchling.h.

 * Includes printf from: https://github.com/mpaland/printf

Tested with ./etc/test.sh using:
 * Microsoft Visual Studio Community 2017 Version 15.9.8+28307.481
 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.10) 5.4.0 20160609
 * clang version 3.8.0-2ubuntu4

Licensed under Apache License Version 2.0, January 2004.
 * http://www.apache.org/licenses/
 * See etc/LICENSE.txt for license terms.
