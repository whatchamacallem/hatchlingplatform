# README

The goal here is to demonstrate an automatic translation of a C/C++17 API into
a Python wrapper using implementation-defined behavior to allow Python to call
C++ directly. This is enabled by using a script as a clang backend that is able
to configure Python's C marshalling package to publish the C++ symbol table in an
.so. Clang is only needed to compile the bindings and it not needed at runtime.

No C/C++ or Python code should need to be written. No C/C++ code is generated.
Any old .so should work without adding the kind of bloat that template based
solutions add. C++ is the interface definition language. C++ code is not being
translated into Python at all. Instead the wrapper just calls your .so as quickly
as possible and leaves ctypes to throw exceptions or not.

Python code written using the wrapper should look Pythonian while remaining
functionally identical to C++ code written using the C++ API.

enum.IntFlag is being used for enums because it is the most literal translation
of a C enum into something considered Pythonian.

ctypes is being used for marshalling and it is being left to throw exceptions
or not. That is Pythonian too.

## Building

Inline C++ code will not be usable by Python by default because it will not be
included in your .so. This situation is inherited from C. The portable solution
is to move the implementation of your C++ library interface into a .cpp file.

If you are using gcc or clang and you have a lot of code in your header files
then it is recommended to use the following strategy instead:

Compile all of your C++ code with `-fvisibility=hidden` in order to make all
function calls candidates for dead code elimination by default. Otherwise C++
generates code with external linkage and so every time the compiler declines to
inline a function it will end up bloating your .so symbol table.

Then explicitly mark your C++ API with a decorator that publishes your inline
symbols into your .so regardless of whether they were used:

```cpp
#define ENTANGLE_API __attribute__((visibility("default"))) \
                     __attribute__((used))
```

## TODO

- Support compiler specific annotation as the way of identifying the API
  instead of header matching. E.g. `__attribute__((annotation("entanglement")))`.

- It should be possible to instantiate templates in header files and have
them linkable with ENTANGLE_API.

- C++ operators.
