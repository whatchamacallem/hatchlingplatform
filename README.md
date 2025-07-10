# Hatchling Platform

[![Generic badge](https://img.shields.io/badge/hatchling-platform-blue.svg)](https://github.com/whatchamacallem/hatchlingplatform)
[![GitHub version](https://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform.svg)](http://badge.fury.io/gh/whatchamacallem%2Fhatchlingplatform)

<br/><img src="hatchling_logo.png" alt="logo" style="display: block; margin: auto;"><br/>

<div style="width: 90%; margin: 0 auto; text-align: justify; font-size: 120%;">

This is a small C++ run-time intended to be developed against on the desktop
before cross compiling to an embedded target with limited RAM. Does not make
dynamic allocations except when allocating system allocators. Requires C99
libraries, a C++98 compiler and does not require the C++ standard library.

Lately I am using this to learn about C++ library design in the context of core
C++ runtime features. These are things I might suggest to the realtime working
group some day. Although a few of my concerns have been addressed by recent
versions of the standard library. The latest versions of the standard have been
refining const correctness which still does nothing for the compiler it wasn't
already doing. And the compilers are still refusing to evaluate the kind of
template bloat in the standard 100% at compile time because they don't seem to
think it is a priority. So I don't think this codebase is obsolete yet. It might
be perfect for WASM actually.

<img src="hatchling_banner.jpg" alt="banner" width="400" height="400"
style="float: right; padding-right: 20px; padding-left: 20px;">

What makes this codebase really shine is Clang's Undefined Behavior Sanitizer
(UBSan) because it allows you to write code the way C++ was originally designed
to be written and debugged using pointers and then have the compiler provide you
with the same runtime guarantees that managed languages have. Why spend our days
mending fences when we belong roaming the open prairie. That said, this codebase
should be compatible with all warning flags and sanitizers on gcc/clang/msvc. No
type was punned in the making of this library.

hx/hatchling.h requires C99. If compiled as C++98 it will include C99 headers.
Those headers were only added to C++11. However all the C++98 compilers allow
C99 headers without complaint. In particular this codebase relies on stdint.h
for fixed width integers which is not in C++98. Features from C++17 and a few
compiler intrinsics are used when available.

HX_RELEASE - This the C/C++ optimization level. Compiler optimization may be
adjusted separately if you want to debug unoptimized versions of these settings.
It is also possible to have compiler optimization turned on for this platform
when it is configured to generate code for debug.
0 - Is a debug build with creature comforts, all asserts and verbose strings.
This build may not fit on the target platform but it has the diagnostics needed
to make a build that does so.
1 - Is a release build with critical asserts and verbose warnings. E.g. for
CMake's "RelWithDebInfo". Might be good for internal use.
2 - Is a release build with only critical asserts using minimal strings. This
build is for profiling and capturing assert call-stacks in the wild. E.g. for
CMake's "MinSizeRel". Filenames are hashed at compile time.
3 - Nothing sacred. No asserts or tear down and very minimal logging. This build
is only for releasing to customers after testing with the above.

</div><br/>
<div style="width: 80%; margin: 0 auto; text-align: justify; font-size: 120%;">

* Testing. Provides a lightweight streamlined reimplementation of Google Test.
  Allows running CI tests on a development board.

* Profiling. Captures a hierarchical time-line view with a minimum of overhead.
  Generates output that can be viewed with Chrome's `about://tracing` view.
  (Use the WASD keys to move around.)

* Memory Management. Abstracts a range of allocation strategies behind an RAII
  interface. This is intended for targets where memory fragmentation is not
  allowed. This may save up to 30% of memory and 30% execution time.

* Containers and algorithms. Provides a small non-reallocating subset of
  `std::vector`, `std::allocator` and `std::unordered_{ map, set, multimap,
  multiset }.` These are integrated with the memory manager.

* Command line based console with simple C++ function binding. Allows debugging
  on the target without relinking and relaunching. Uses no allocations.

* Task Queue. Provides an object oriented interface to multi-threading. The
  default implementation uses `<thread>` and allows for target specific
  implementations.

* Uses the minimum necessary standard C99 headers as required. With the
  exception of host implementations where threading and time headers are
  included. Also `<new>` is used for placement new.

* Logging and memory management available in plain C99. `<hx/hatchling.h>` is
  available in C and C++.

* 64-bit clean. Intended for but not limited to use with a 32-bit target.
  size_t is fairly widely used.

* Does not use exceptions or `std::type_info` as these are inefficient.

* See `LICENSE.txt` for license terms.

Tested using:

* gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
* Ubuntu clang version 18.1.3 (1ubuntu1) x86_64-pc-linux-gnu
* emcc (Emscripten 4.0.5)
* valgrind-3.22.0 (tested in 64-bit mode.)
* See `testmatrix.sh`

</div>
