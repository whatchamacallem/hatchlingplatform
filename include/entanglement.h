#pragma once
// Copyright 2017-2025 Adrian Johnston

#ifndef ENTANGLEMENT
/// DENTANGLEMENT - Use -DENTANGLEMENT=0 to disable the entanglement attributes.
#define ENTANGLEMENT 1
#endif

#if ENTANGLEMENT

/// ENTANGLEMENT_LINK - Indicates that a function, constructor, destructor or
/// method should be made available in Python and also included in the .so even
/// if it is declared inline. It is recommended to be used with -fvisibility=hidden
/// so that symbols that are not part of the package API are dead-stripped away.
/// Header file libraries using this attribute are fine but they have to be
/// included in at least one C++ translation unit included in the .so for Python
/// to be able to load them.
#define ENTANGLEMENT_LINK \
	__attribute__((annotate("entanglement"))) \
	__attribute__((used)) \
	__attribute__((visibility("default")))

/// ENTANGLEMENT_TYPE - Indicates that an enum, class or struct should be
/// available in Python. It is an error to use a type that is not marked.
#define ENTANGLEMENT_TYPE __attribute__((annotate("entanglement")))

#else
#define ENTANGLEMENT_LINK
#define ENTANGLEMENT_TYPE
#endif
