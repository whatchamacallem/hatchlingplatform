// Copyright 2017-2025 Adrian Johnston
#pragma once

#include <entanglement.h>
#include <stdint.h>
#include <stdio.h>

// Enums.
enum ENTANGLEMENT_T {
	ANONYMOUS_ENUM_0
};
enum ENTANGLEMENT_T EnumCStyleTwoConstants {
	ENUM_C_STYLE_TWO_CONSTANTS_1 = 1,
	ENUM_C_STYLE_TWO_CONSTANTS_2
};
enum ENTANGLEMENT_T EnumInt16ThreeConstants : int16_t {
	ENUM_INT16_THREE_CONSTANTS_0 = -32768,
	ENUM_INT16_THREE_CONSTANTS_1 = -1,
	ENUM_INT16_THREE_CONSTANTS_2 = 32767
};
enum class ENTANGLEMENT_T EnumScopedUInt64 : uint64_t {
	ENUM_SCOPED_UINT64_0 = 0xabcdef0123456789
};

/// Smoke test the C calling convention.
ENTANGLEMENT int8_t function_roundtrip_int8(int8_t x);
ENTANGLEMENT uint16_t function_roundtrip_uint16(uint16_t x);
ENTANGLEMENT int32_t function_roundtrip_int32(int32_t x);
ENTANGLEMENT uint64_t function_roundtrip_uint64(uint64_t x);

/// Overloaded functions with different return types.
ENTANGLEMENT void function_overload();
ENTANGLEMENT int function_overload(int a, int b);
ENTANGLEMENT float function_overload(int, int, int, int);

/// Pointers and arrays. Writes a series of numbers starting at a value.
/// E.g. size=3 and value=3 results in [3,4,5].
ENTANGLEMENT int8_t* function_pointer_int8(int8_t* x, size_t size, int8_t value);
ENTANGLEMENT uint16_t* function_pointer_uint16(uint16_t* x, size_t size, int16_t value);
ENTANGLEMENT int32_t* function_pointer_int32(int32_t* x, size_t size, int32_t value);
ENTANGLEMENT uint64_t* function_pointer_uint64(uint64_t* x, size_t size, int64_t value);
ENTANGLEMENT void* function_pointer_void_to_int(void* x, size_t size, int value);
ENTANGLEMENT char* function_pointer_char(char* x);
ENTANGLEMENT wchar_t* function_pointer_wchar(wchar_t* x);

/// Fundamental references. & and &&. Honey badger don't care.
ENTANGLEMENT char& function_ref_char(char& x, char value);
ENTANGLEMENT uint16_t& function_ref_uint16(uint16_t& x, uint16_t value);
ENTANGLEMENT wchar_t& function_ref_wchar(wchar_t& x, wchar_t value);
ENTANGLEMENT uint64_t& function_ref_uint64(uint64_t& x, uint64_t value);

/// A struct that points to a double.
struct ENTANGLEMENT_T StructCPointer {
	ENTANGLEMENT StructCPointer(double* ptr);
	ENTANGLEMENT ~StructCPointer();
	ENTANGLEMENT float AsFloat();
	double* m_double;
};
