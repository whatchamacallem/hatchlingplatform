#pragma once
// Copyright 2017-2025 Adrian Johnston
//
// The ENTANGLEMENT_T and ENTANGLEMENT attributes define a .so library
// interface. The usage of these C++ attributes is as follows:
//
// example.hpp:
//
//     enum ENTANGLEMENT_T example_enum { };
//
//     ENTANGLEMENT float example_function(float x) noexcept;
//
//     class ENTANGLEMENT_T example_class {
//     public:
//         ENTANGLEMENT int example_method(void) noexcept;
//     };
//
// example.cpp:
//
//     float example_function(float x) noexcept { return 1.0f; };
//
//     int example_class::example_method(void) noexcept { return 2; }
//

#ifndef ENTANGLEMENT_PASS
/// `ENTANGLEMENT_PASS` - Indicates that an entanglement.py binding pass might
/// be taking place. Use `-DENTANGLEMENT_PASS=0` to disable the entanglement
/// attributes and signal that script bindings are not being generated. This
/// should not normally be required with gcc and clang.
#define ENTANGLEMENT_PASS 1
#endif

#if ENTANGLEMENT_PASS

/// `ENTANGLEMENT` - Indicates that a function, constructor, destructor or
/// method should be made available in Python and also included in an .so. The
/// only way to guarantee a symbol is available for Python to link against is to
/// declare it non-inline or out-of-line in a .cpp file. Any symbol annotated
/// with `ENTANGLEMENT` must be in your .cpp file, outside of any class or
/// struct definitions and must not be inline. The `__attribute__((noinline))`
/// and `__attribute__((used))` compiler attributes do not remove the normal
/// C++ inline attribute and weird link errors may still result. This header is
/// recommended to be used with -fvisibility=hidden so that symbols that are not
/// part of the .so's intended API are dead-stripped away. The gcc/clang
/// attributes used are:
/// - `__attribute__((annotate("entanglement")))` : Used by entanglement.py to identify the API.
/// - `__attribute__((used))` : Causes the compiler to emit the function regardless of use.
/// - `__attribute__((visibility("default")))` : Causes the linker to keep the symbol.
#define ENTANGLEMENT \
	__attribute__((annotate("entanglement"))) \
	__attribute__((used)) \
	__attribute__((visibility("default")))

/// `ENTANGLEMENT_T` - Indicates that an enum, class or struct should be
/// available in Python. It is an error to use a type that is not marked.
/// - `__attribute__((annotate("entanglement")))` : Used by entanglement.py to identify the API.
#define ENTANGLEMENT_T __attribute__((annotate("entanglement")))

#else
#define ENTANGLEMENT
#define ENTANGLEMENT_T
#endif
