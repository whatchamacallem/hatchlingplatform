# entanglement.py

<div style="text-align: justify; font-size: 120%;">

## Table of Contents

- [Overview of entanglement.py](#overview-of-entanglementpy)
- [Goals and Comparison with Other Projects](#goals-and-comparison-with-other-projects)
- [Command Line Arguments](#command-line-arguments)
- [Function Handling](#function-handling)
- [Class and Struct Handling](#class-and-struct-handling)
- [Generated Document Structure](#generated-document-structure)
- [Exception Handling](#exception-handling)
- [Building](#building)
- [Roadmap](#roadmap)

## Overview of entanglement.py

> **_NOTE:_** *This is an alpha release.*

`entanglement.py` is a Python binding generator that automatically creates
Python interfaces for C++ shared libraries. This command-line tool uses `clang`
to parse C++ header files annotated with `<entanglement.h>` macros and generates
corresponding Python code that calls C++ library functions directly through
`ctypes`. The generated bindings provide a Pythonian way to call C++ functions
and work with C++ data structures while maintaining type safety.

There are a few limitations to this approach. Python isn't a great language to
use for manipulating data structures made out of C/C++ pointers. Object oriented
interfaces should be a lot easier to use with `entanglement.py`. Also, Python
uses `memcpy` to return classes by value from a method or function. This is
because it is a foreign language that does not support the C++ lifecycle
natively and _will result in a missing C++ constructor call._ The C++ lifecycle
is supported correctly for C++ objects that are created in Python and passed by
reference.

## Goals and Comparison with Other Projects

The primary aim of this project was to demonstrate an automatic transpilation of
a C/C++17 API into a Python wrapper using implementation-defined behavior to
allow Python to call C++ directly. This is enabled by using a scripted clang API
transpiler that is able to configure Python's C marshalling package to publish
the C++ symbol table directly into Python. `clang` is only needed to build the
bindings and is not used at runtime.

There are a few other very interesting projects that bind Python to C++ and
there are different tradeoffs involved:

- Other tools using C++ templates for C++ introspection are encountering the
  limitations of that approach. Budget half your compile time and half your
  executable size just for your script bindings.
- Meanwhile, automatically generated marshalling code for an API that does not
  accept Python objects natively cannot reasonably be expected to compete with
  hand-written code that does uses Python objects natively via the Python C API.
  The only problem is that it requires a lot of hand written C code.

So this tool tries to optimize your C++ compile-test loop by making the binding
process almost instant and automatic. This is done in a generated script so
there is no compile time and everything involved happens in plain view. It
should be as efficient as possible while still asking Python to speak a foreign
language.

Key characteristics:

- C/C++ is the interface definition language.
- No C/C++ or Python code needs to be written.
- No C/C++ code is generated.
- No C/C++ code is translated into Python.
- Any `.so` (that does include all the C/C++ symbols required) should work without
  adding additional overhead to the `.so` itself.
- A public API for your package can be composed by mixing hand written code with
  symbols from the wrapper using the following syntax: `from .the_wrapper import
  wrapper_symbol`.

Python code written using the wrapper should look Pythonian while remaining
functionally identical to C++ code written using the C++ API. C++ programmers
should not be caught off guard by the nature of the translation.

- `ctypes` is used for marshalling and it is left to throw exceptions or not
  (also considered Pythonian). This is the most common solution for C.
- `enum.IntFlag` is used for enums because it is the most literal translation of
  a C enum into something considered Pythonian.

## Command Line Arguments

The usage is:

```text
python3 entanglement.py [compiler_flags] [lib_name] [header_files]... [output_file]

  compiler_flags  - Flags to pass to clang. Can be in any order on command line.
  lib_name        - C/C++ library/.so name to bind everything to.
  header_files... - Path(s) to the C++ header file(s) to parse.
  output_file     - Path to write the generated Python binding code.
```

The compiler flags can actually be provided in any order as long as they start
with hyphens.

Also see `VERBOSE` and `LIBCLANG_PATH` in entanglement.py for configuration.

## Function Handling

Objects may be passed as pointers, either kind of reference and by value.
References are returned as pointers in Python because Python has no other way to
access data stored in C++. There is no const in Python so it is ignored. Both
`ctypes` and `NumPy` have robust type checking and the proper diagnostics are
generated when the wrong type is passed. Nothing is coerced.

Functions can be overloaded and are resolved in two steps. First overloads are
selected by the argument count. If there is more than one overload remaining
then they are selected between using the type of the first arg. This is
performed by generating only the code required.

`NumPy`'s `ndarray` is popular for moving large datasets between C/C++ and
Python.

## Class and Struct Handling

For C++ classes and structs `entanglement.py`:

- Generates Python classes that inherit from `ctypes.Structure`.
- Automatically defines the `_fields_` attribute to match the C++ memory layout.
- Implements single inheritance hierarchies with virtual method tables.

Mapping C++'s ability to reopen namespaces and forward declare nested classes
onto Python and `ctypes` does require monkey patching. However, there shouldn't
be anything that is unsupported.

## Generated Document Structure

The output Python file has a three-part structure:

1. The Python API with all the wrapped enums, functions, classes and structs.
2. The structure and class definitions with their memory layouts.
3. The symbol table that connects everything to the actual shared library.

The generated code also includes:

- All necessary imports and .so loading.
- A shim that handles both direct `ctypes` Python objects and NumPy arrays
  directly.
- Documentation is copied into the wrapper by extracting C++ doxygen comments
  and converting them into Python docstrings. The Python wrapper also has
  documentation showing the underlying C++ code being called.

## Exception Handling

- C++ exceptions must not be allowed to escape into the Python interpreter as
  they are from a language that is foreign to it. It will crash.
- Use the Python C API to report C++ exceptions back to the Python interpreter
  if exceptions are required.
- The `noexcept` keyword may be useful during testing with C++ test cases but it
  will not be enforced by Python.

## Building

The following strategy is recommended when building a .so:

- Compile all of your C++ code with `-fvisibility=hidden` in order to make all
  function calls candidates for dead code elimination by default. Otherwise C++
  generates code with external linkage and so every time the compiler declines
  to inline a function it will end up bloating your .so symbol table.
- Then mark your C++ API with `<entanglement.h>` attributes so that they are
  retained as symbols into your .so.

## Roadmap

- .pyi stub file generation. Needed for pylance checking of inheritance and
  member variables.
- Default function parameters.
- Portable libclang path resolution?
- Portable .so resolution?

</div>
