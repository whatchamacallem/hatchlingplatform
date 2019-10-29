
The Hatchling Platform is designed to facilitate C++98 embedded systems
development on a target too constrained to provide traitional operating
system services.  This is a narrowly defined platform which can be developed
against on a modern desktop development environment before cross compiling
to an embedded system.  It uses C++11 in certain cases on the host.

* Test Driver.  A lightweight reimplementation of GoogleTest.

* Profiling.  Captures a hierarchical timeline view with a minimum of overhead.
  View the example profile.json file in Chrome's about://tracing view.

* Memory Management.  Hides a range of allocation strategies behind a simple
  RAII interface.

* DMA.  Cross platform DMA with validation.

* Container Support.  Provides a small non-reallocating subset of std::vector,
  std::allocator and std::unordered_{map,multimap,set,multiset}.

* Command line based console with simple C++ bindings.

* Task Queue.  Simple object oriented interface to multi-threading.

* 64-bit clean.  Intended for but not limited to use with a 32-bit target.

* Does not use exceptions or std::type_info.

* Uses standard C99 headers as required.  With the exception of host
  implementations where threading and time headers are included.  And <new>
  and <algorithm>...  Back porting to C95 headers should not be too hard.

* Logging and memory management available in plain C.  See hatchling.h.

Tested with:
	Visual Studio all builds

	gcc -O3 -Wall -DHX_RELEASE=0 -std=c++98 -fno-exceptions -fno-rtti -Wno-unused-local-typedefs *.cpp *.c -lstdc++ -o hxtest
	./hxtest

	for i in 0 1 2 3; do
	clang -O3 -Wall -DHX_RELEASE=$i -lubsan -fsanitize=undefined -Wno-unused-local-typedefs *.cpp *.c -lstdc++ -o hxtest
	./hxtest
	done

Using versions:
	Visual Studio Community 2017 15.7.6
	gcc version 5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.5)
	clang version 3.8.0-2ubuntu4 (tags/RELEASE_380/final)

