# entanglement.py

<div style="text-align: justify; font-size: 120%;">

## Table of Contents

- [Overview of entanglement.py](#overview-of-entanglementpy)
- [Project Goals and Comparison with Other Projects](#9-output-structure)
- [Command Line Arguments](#2-command-line-arguments)


## Overview of entanglement.py

`entanglement.py` is a Python binding generator that automatically creates
Python interfaces for C++ shared libraries. This command-line tool uses Clang to
parse C++ header files annotated with `<entanglement.h>` macros and generates
corresponding Python code that interacts with the native library through
`ctypes`. The generated bindings provide a Pythonian way to call C++ functions
and work with C++ data structures while maintaining type safety.

## Project Goals and Comparison with Other Projects

The primary aim of this project is to demonstrate an automatic transpilation of
a C/C++17 API into a Python wrapper using implementation-defined behavior to
allow Python to call C++ directly. This is enabled by using a scripted clang API
transpiler that is able to configure Python's C marshalling package to publish
the C++ symbol table directly into Python. `clang` is only needed to build the
bindings and is not used at runtime.

There are a few other very interesting projects that bind Python to C++ and
there are different tradeoffs involved.

- Automatically generated binding code for an API that does not accept Python
  objects natively cannot reasonably be expected compete with hand written code
  that does use the Python C API.
- Tools using C++ templates for C++ introspection are encountering the
  limitations of what compilers can do during compile time optimization.

This tool tries to optimize your compile-test loop by making the binding process
instant and automatic. Marshalling overhead is only incurred when modifying
C/C++ data as the C/C++ data structures are otherwise cached in Python. It
should be as efficient as possible while still asking Python to speak a foreign
language.

Key characteristics:

- No C/C++ or Python code should need to be written.
- No C/C++ code is generated.
- Any old `.so` (that actually includes all the required C/C++ symbols) should.
  work without adding the kind of bloat that template based solutions add.
- C++ is the interface definition language.
- C++ code is not being translated into Python at all.
- The wrapper just calls your `.so` as quickly as possible and leaves `ctypes`
  to throw exceptions or not.

## Design Philosophy

Python code written using the wrapper should:

- Look Pythonian.
- Remain functionally identical to C++ code written using the C++ API.

Implementation choices:

- `enum.IntFlag` is used for un-scoped enums because it is the most literal
  translation of a C enum into something considered Pythonian.
- `ctypes` is used for marshalling and it is left to throw exceptions or not
  (also considered Pythonian).

## Command Line Arguments

The tool takes several required command line arguments:

- Compiler flags needed by Clang
- The name of the shared library to bind against
- One or more header files containing the interface to wrap
- An output filename where the generated Python code should be written

The compiler flags can be provided in any order as long as they follow standard
flag conventions (starting with hyphens). The tool validates all arguments and
provides clear usage messages if they are incorrect.

## Core Functionality

At its core, `entanglement.py` performs C++ code analysis using `libclang` to
understand the library's interface. It handles fundamental C/C++ types by
mapping them to appropriate `ctypes` equivalents - for example, converting C++
`int` to `ctypes.c_int`. The generator includes special handling for pointers
and references, creating conversion shims that allow Python code to work
naturally with these constructs while maintaining proper memory access.

## Type Handling

For classes and structs, the tool:

- Generates Python classes that inherit from `ctypes.Structure`
- Automatically defines the `_fields_` attribute to match the C++ memory layout
- Properly handles inheritance hierarchies and virtual method tables

Enums become Python `IntEnum` classes to preserve their integer nature while
providing named constants. The generator preserves documentation by extracting
C++ comments and converting them into Python docstrings.

## Class and Struct Handling

The tool enforces Python naming conventions and rejects invalid symbols that
would cause problems, such as:

- Names starting with underscores followed by uppercase letters
- Python keywords
- Names reserved by `ctypes`

The generated code includes runtime checks to:

- Verify library loading succeeds
- Confirm structure sizes match expectations

## Generated Document Structure

The output Python file follows a clear three-part structure:

1. The Python API with all the wrapped functions and classes
2. The structure and class definitions with their memory layouts
3. The symbol table that connects everything to the actual shared library

The generated code includes:

- All necessary imports
- Helper functions like the pointer conversion shim that handles both direct
  `ctypes` objects and NumPy arrays

## Exception Handling

Critical considerations:

- C++ exceptions must not be allowed to escape into the Python interpreter as
  they are from a language that is foreign to it.
- Allowing C++ exceptions to escape into a plain C application like Python may
  result in inexplicable core dumps.
- Use the Python C API to report C++ exceptions back to the Python interpreter
  if exceptions are required.
- The `noexcept` keyword may be useful during testing with C++ test cases but it
  will not be enforced by Python.

## Building

The following strategy is recommended.

Compile all of your C++ code with `-fvisibility=hidden` in order to make all
function calls candidates for dead code elimination by default. Otherwise C++
generates code with external linkage and so every time the compiler declines to
inline a function it will end up bloating your .so symbol table.

Then explicitly mark your C++ API with decorators that publishes your inline
symbols into your .so regardless of whether they were used.

## TODO

- Oh shit constructors.
- Nested class fields. Oh shit dependency order.
- structs are allowed to have any kind of pointer they want. no errors.
- All C++ operators. E.g. hxrandom.
- Nested classes.
- Pointers and references to classes, all kinds.
- Enum return value.
- Reopening a namespace and sub-namespace to inherit from them in another file.
- Reopening a namespace and sub-namespace to overload them in another file.
- Pure virtual method with a non-virtual wrapper calling 2 base classes.
- Test multiple headers.
- Add default function parameters. Note arg count based dispatch is affected.

- Pylance wrapper generation.
- Make hatchling.py wrapper.
- dll loading for real? how to separate.
  - lib clang path resolution.
  - Wrapped .so resolution (make user responsible?).

## Long Term

These would be nice to have but are not implemented and are not a priority.

- Overloaded function dispatch by first arg type when dispatch by arg count is
  ambiguous. Need to match subclasses first.
- Write C++ examples that trigger every error possible and have tests that check
  they are all reported correctly.

</div>
