#pragma once
// Copyright 2017-2025 Adrian Johnston

// The ENTANGLEMENT_TYPE and ENTANGLEMENT_LINK attributes define a .so library API.
// The usage of these C++ attributes is as follows:
//
//     enum ENTANGLEMENT_TYPE example_enum { };
//
//     ENTANGLEMENT_LINK float example_function(float x) noexcept;
//
//     class ENTANGLEMENT_TYPE example_class {
//     public:
//         ENTANGLEMENT_LINK int example_method(void) noexcept;
//     };

#ifndef ENTANGLEMENT_PASS
/// `ENTANGLEMENT_PASS` - Indicates that an entanglement.py binding pass might be
/// taking place. Use `-DENTANGLEMENT_PASS=0` to disable the entanglement
/// attributes and signal that script bindings are not being generated. This should
/// not normally be required with gcc and clang.
#define ENTANGLEMENT_PASS 1
#endif

#if ENTANGLEMENT_PASS

/// `ENTANGLEMENT_LINK` - Indicates that a function, constructor, destructor or
/// method should be made available in Python and also included in an .so even
/// if it is declared inline. It is recommended to be used with -fvisibility=hidden
/// so that symbols that are not part of the .so's intended API are dead-stripped
/// away. Header file libraries using this attribute are fine but they have to be
/// included in at least one C++ translation unit included in the .so for Python
/// to be able to load them. The `nowthrow` C++ attribute is recommended with this.
/// - `__attribute__((annotate("entanglement")))` : Used by entanglement.py to identify the API.
/// - `__attribute__((used))` : Causes the compiler to emit the function regardless of use.
/// - `__attribute__((visibility("default")))` : Causes the linker to keep the symbol.
#define ENTANGLEMENT_LINK \
	__attribute__((annotate("entanglement"))) \
	__attribute__((used)) \
	__attribute__((visibility("default")))

/// `ENTANGLEMENT_TYPE` - Indicates that an enum, class or struct should be
/// available in Python. It is an error to use a type that is not marked.
/// - `__attribute__((annotate("entanglement")))` : Used by entanglement.py to identify the API.
#define ENTANGLEMENT_TYPE __attribute__((annotate("entanglement")))

#else
#define ENTANGLEMENT_LINK
#define ENTANGLEMENT_TYPE
#endif
