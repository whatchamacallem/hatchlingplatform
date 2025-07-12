# TODO

The goal here is to demonstrate an automatic translation of a C++ API into a
Python wrapper without writing or modifying code. Python code written using the
wrapper should look as much like C++ code written using the C++ API as possible.

This code depends on the clang compiler and the different C++ ABIs. Specifically
C++ features are being supported in ways that are "implementation-defined behavior"
as defined by the standard.

enum.IntFlag is being used for enums because it is the most literal translation
of a C enum into something considered Pythonian.

ctypes is being used for marshalling and it is being left to throw exceptions
or not. That is Pythonian too.
