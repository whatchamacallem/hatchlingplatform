// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
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

/// Fundamental references. & and &&.
ENTANGLEMENT bool& function_ref_bool(bool& x, bool value);
ENTANGLEMENT uint16_t& function_ref_uint16(uint16_t& x, uint16_t value);
ENTANGLEMENT void function_ref_wchar(wchar_t& x, wchar_t value);
ENTANGLEMENT uint64_t& function_ref_uint64(uint64_t& x, uint64_t value);

/// Pack a C structure tight with fundamental types and pass it by value.
/// XXX Size check.
struct ENTANGLEMENT_T StructFundamentals {
	bool m_bool;
	char m_char0;
	int8_t m_char1;
	uint8_t m_char2;
	int m_int0;
	int32_t m_int1;
	uint64_t m_uint2;
	double m_double;
};

ENTANGLEMENT StructFundamentals function_struct_fundamentals_multiply(
	StructFundamentals struct_fundamentals, int multiplier);

/// Fill a virtual structure with fundamental pointer types. Inherit it from previous test
/// and pass by reference.
/// XXX Size check.
struct ENTANGLEMENT_T StructPointerFundamentals : public StructFundamentals {
	StructPointerFundamentals();
	virtual ~StructPointerFundamentals();
	virtual void null_it_all(void);
	void* m_void;
	char* m_char;
	wchar_t* m_wchar_t;
	bool* m_bool;
	float* m_float;
	double* m_double;
};

ENTANGLEMENT StructPointerFundamentals& function_struct_pointer_fundamentals_multiply(
	StructPointerFundamentals& struct_fundamentals, int multiplier);
