# README

The goal here is to demonstrate an automatic transpilation of a C/C++17 API into
a Python wrapper using implementation-defined behavior to allow Python to call
C++ directly. This is enabled by using a scripted clang API transpiler that is able
to configure Python's C marshalling package to publish the C++ symbol table
directly into Python. clang is only needed to build the bindings and is not used
at runtime.

There are a few other very interesting projects that bind Python to C++ and
there are different tradeoffs involved. Automatically generated binding code for
an API that does not accept Python objects natively cannot reasonably be
expected compete with hand written code that does use the Python C API. On the
other end of the spectrum the tools using C++ templates for C++ introspection
are encountering the limitations of what compilers can do during compile time
optimization. This tool tries to optimize your compile-test loop by making the
binding process instant and automatic. Marshalling overhead is only incurred
when modifying C/C++ data as the C/C++ data structures are otherwise cached in
Python. It should be as efficient as possible while still asking Python to speak
a foreign language.

No C/C++ or Python code should need to be written. No C/C++ code is generated.
Any old .so (that actually includes all the required C/C++ symbols) should work without adding the kind of bloat that template based
solutions add. C++ is the interface definition language. C++ code is not being
translated into Python at all. Instead the wrapper just calls your .so as
quickly as possible and leaves ctypes to throw exceptions or not.

Python code written using the wrapper should look Pythonian while remaining
functionally identical to C++ code written using the C++ API.

enum.IntFlag is being used for un-scoped enums because it is the most literal
translation of a C enum into something considered Pythonian. ctypes is being
used for marshalling and it is being left to throw exceptions or not. That is
Pythonian too.

C++ exceptions must not be allowed to escape into the Python interpreter as they
are from a language that is foreign to it. Allowing C++ exceptions to escape
into a plain C application like Python may result in inexplicable core dumps.
Use the Python C API to report C++ exceptions back to the Python interpreter if
exceptions are required. The `noexcept` keyword may be useful during testing
with C++ test cases but it will not be enforced by Python.

## Building

The following strategy is recommended.

Compile all of your C++ code with `-fvisibility=hidden` in order to make all
function calls candidates for dead code elimination by default. Otherwise C++
generates code with external linkage and so every time the compiler declines to
inline a function it will end up bloating your .so symbol table.

Then explicitly mark your C++ API with decorators that publishes your inline
symbols into your .so regardless of whether they were used.

```cpp

// Include the entanglement link attribute and the api type attribute.
#include <entanglement.h>

// Attribute registers enums, functions, classes.
#define ENTANGLEMENT_T

// Attribute registers functions, constructors, destructors and method calls.
#define ENTANGLEMENT

```

## TODO

- test multiple headers

- C++ operators and default params.  E.g. hxrandom.
- Plain C linkage.
- dll loading for real.

- get_array_element_type(self):
- get_array_size(self):

- get_size(self): # assert(sizeof == get_size())
- is_bitfield(self):
- get_bitfield_width(self):

- config file?

```cpp

x = MyAPI.NestedStruct(MyAPI.VirtualBaseClass())
print(f"Size of DerivedStruct: {ctypes.sizeof(x)} bytes")

```
