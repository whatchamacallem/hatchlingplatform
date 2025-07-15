# README

The goal here is to demonstrate an automatic transpilation of a C/C++17 API into
a Python wrapper using implementation-defined behavior to allow Python to call
C++ directly. This is enabled by using a scripted clang transpiler that is able
to configure Python's C marshalling package to publish the C++ symbol table in an
.so. clang is only needed to build the bindings and is not used at runtime.

There are a few other very interesting projects that bind Python to C++ and there
are different tradeoffs involved. Automatically generated binding code for an
API that is not Python native cannot reasonably be expected compete with hand
written code that uses the Python C API. On the other end of the spectrum the
tools using C++ templates for C++ introspection are encountering the limitations
of what compilers can do during compile time optimization. This tool tries to
optimize your compile-test loop by making the binding process instant and
automatic. Marshalling overhead is only incurred when modifying C/C++ data as
the C/C++ data structures are otherwise cached in Python.

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

- add __vtable when no virtual parent. abort when multiple virtual parents.

- Support compiler specific annotation as the way of identifying the API
  instead of header matching. E.g. `__attribute__((annotation("entanglement")))`.

- It should be possible to instantiate templates in header files and have
them linkable with ENTANGLE_API.

- C++ operators and default params.  E.g. hxrandom.

- Yet another config file?

This compiles and runs.

```cpp
import ctypes

class MyAPI:
    class VirtualBaseClass(ctypes.Structure):
        _fields_ = [
            ("__vtable", ctypes.c_void_p)
        ]

    class NestedStruct(ctypes.Structure):
        _fields_ = [
            ("nested_field1", ctypes.c_double),
            ("nested_field2", ctypes.c_char_p)
        ]

        # Use string quotes to defer Pylance type lookup.
        def __init__(self, arg0: 'MyAPI.VirtualBaseClass') -> None:
            pass

        def __del__(self) -> None:
            pass

    class DerivedStruct(VirtualBaseClass):
        pass

# STRUCTURE TABLE

MyAPI.DerivedStruct._fields_ = [
    ("nested", MyAPI.NestedStruct),
    ("derived_field", ctypes.c_uint32),
    ("global_enum_field", ctypes.c_uint32),
]

x = MyAPI.NestedStruct(MyAPI.VirtualBaseClass())
print(f"Size of DerivedStruct: {ctypes.sizeof(x)} bytes")

# SYMBOL TABLE

_Z3fn1f = __clib._Z3fn1f
_Z3fn1f.argtypes = [ctypes.c_float]
_Z3fn1f.restype = ctypes.c_float
```
