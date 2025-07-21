// Copyright 2017-2025 Adrian Johnston
#pragma once

#include <entanglement.h>
#include <stdint.h>
#include <cstddef>
#include <cstdio>

// Enums.
enum ENTANGLEMENT_TYPE {
	ANONYMOUS_ENUM_0
};
enum ENTANGLEMENT_TYPE EnumCStyleTwoConstants {
	ENUM_C_STYLE_TWO_CONSTANTS_1 = 1,
	ENUM_C_STYLE_TWO_CONSTANTS_2
};
enum ENTANGLEMENT_TYPE EnumInt16ThreeConstants : int16_t {
	ENUM_INT16_THREE_CONSTANTS_0 = -32768,
	ENUM_INT16_THREE_CONSTANTS_1 = -1,
	ENUM_INT16_THREE_CONSTANTS_2 = 32767
};
enum class ENTANGLEMENT_TYPE EnumScopedUInt64 : uint64_t {
	ENUM_SCOPED_UINT64_0 = 0xabcdef0123456789
};

/// Smoke test the C calling convention.
ENTANGLEMENT_LINK int8_t function_roundtrip_int8(int8_t x);
ENTANGLEMENT_LINK uint16_t function_roundtrip_uint16(uint16_t x);
ENTANGLEMENT_LINK int32_t function_roundtrip_int32(int32_t x);
ENTANGLEMENT_LINK uint64_t function_roundtrip_uint64(uint64_t x);

/// Overloaded functions with different return types.
ENTANGLEMENT_LINK void function_overload();
ENTANGLEMENT_LINK int function_overload(int a, int b);
ENTANGLEMENT_LINK float function_overload(int, int, int, int);

/// Pointers and arrays. Writes a series of numbers starting at a value.
/// E.g. size=3 and value=3 results in [3,4,5].
ENTANGLEMENT_LINK int8_t* function_pointer_int8(int8_t* x, size_t size, int8_t value);
ENTANGLEMENT_LINK uint16_t* function_pointer_uint16(uint16_t* x, size_t size, int16_t value);
ENTANGLEMENT_LINK int32_t* function_pointer_int32(int32_t* x, size_t size, int32_t value);
ENTANGLEMENT_LINK uint64_t* function_pointer_uint64(uint64_t* x, size_t size, int64_t value);

/// A struct that points to a double.
struct ENTANGLEMENT_TYPE StructCPointer {
	ENTANGLEMENT_LINK StructCPointer(double* ptr);
	ENTANGLEMENT_LINK ~StructCPointer();
	ENTANGLEMENT_LINK float AsFloat();
	double* m_double;
};
