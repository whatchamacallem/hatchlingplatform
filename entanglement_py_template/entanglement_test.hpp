// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
#pragma once

#include <entanglement.h>
#include <stdint.h>
#include <stdio.h>


// Enums.
extern "C" {
enum ENTANGLEMENT_T {
	ANONYMOUS_ENUM_0
};
enum ENTANGLEMENT_T EnumCStyleTwoConstants {
	ENUM_C_STYLE_TWO_CONSTANTS_1 = 1,
	ENUM_C_STYLE_TWO_CONSTANTS_2
};
} // extern "C"
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

/// Pointers and arrays. Writes a series of numbers starting at a value. E.g.
/// size=3 and value=3 results in [3,4,5].
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

/// Pack a C structure tight with fundamental types and pass it by value. This
/// is packed assuming bool is 1 byte and int is not 16-bit.
extern "C" {
struct ENTANGLEMENT_T StructFundamentals {
	bool m_bool;
	char m_char0;
	int8_t m_char1;
	uint8_t m_char2;
	int m_int0;
	int m_int_a : 16;
	int m_int_b : 8;
	int m_int_c : 8;
	int32_t m_int1;
	uint64_t m_uint2;
	double m_double[1];
	uint8_t m_three_dim[2][3][4];
};

ENTANGLEMENT StructFundamentals function_struct_fundamentals_multiply(
	StructFundamentals struct_fundamentals, int multiplier);
} // extern "C" {

// Fill a virtual structure with fundamental pointer types. Inherit it from
// previous test and pass by reference.
struct ENTANGLEMENT_T StructPointerFundamentals : public StructFundamentals {
	ENTANGLEMENT StructPointerFundamentals(void);
	ENTANGLEMENT virtual ~StructPointerFundamentals(void);
	void* m_pvoid;
	bool* m_pbool;
	float* m_pfloat;
};

ENTANGLEMENT StructPointerFundamentals& function_struct_pointer_fundamentals_multiply(
	StructPointerFundamentals& struct_fundamentals, int multiplier);

// All operations return the symbolic name of the operator. A number of
// operators are missing. These are just the ones that have literal translations
// between languages. E.g. Python uses a cast to bool to implement && and ||.
// There is no assignment operator because it wouldn't be what was expected.
class ENTANGLEMENT_T OperatorTest {
public:
	ENTANGLEMENT OperatorTest(void);
	ENTANGLEMENT OperatorTest(int);
	ENTANGLEMENT ~OperatorTest();

	//__bool__ is used to implement the logical operators: && || !
	ENTANGLEMENT bool __bool__(void) const;

	// These are in alphabetical order by python dunder name. see
	// _operator_name_map.
	ENTANGLEMENT wchar_t* operator+(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator&(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator()(size_t) const;
	ENTANGLEMENT wchar_t* operator==(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator>=(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator>(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator[](size_t) const;
	ENTANGLEMENT wchar_t* operator&=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator+=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator<<=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator*=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator|=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator%=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator~() const;
	ENTANGLEMENT wchar_t* operator^=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator-=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator/=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator>>=(const OperatorTest&);
	ENTANGLEMENT wchar_t* operator<=(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator<<(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator<(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator%(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator*(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator!=(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator|(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator>>(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator-(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator+() const;
	ENTANGLEMENT wchar_t* operator-() const;
	ENTANGLEMENT wchar_t* operator/(const OperatorTest&) const;
	ENTANGLEMENT wchar_t* operator^(const OperatorTest&) const;
	bool pad;
};

namespace NameSpaceOne {
	class ENTANGLEMENT_T NameSpaceOneClassOne {
	public:
		ENTANGLEMENT int class_one_one(int);
		int pad0;
	};
	class ENTANGLEMENT_T NameSpaceOneClassTwo : public NameSpaceOneClassOne {
	public:
		ENTANGLEMENT int class_one_two(int);
		int pad1;
	};
	ENTANGLEMENT int namespace_one(int);
};

namespace NameSpaceTwo {
	class ENTANGLEMENT_T NameSpaceTwoClassOne {
	public:
		ENTANGLEMENT int class_two_one(int);
		NameSpaceOne::NameSpaceOneClassTwo one_two;
	};
	ENTANGLEMENT int namespace_two(int);
};

// Creates an inheritance cycle between two namespaces. one -> two -> one.
namespace NameSpaceOne {
	// Add subclass of a different namespace to a re-opened namespace.
	class ENTANGLEMENT_T NameSpaceOneClassThree : NameSpaceTwo::NameSpaceTwoClassOne {
	public:
		ENTANGLEMENT int class_one_three(int);
		int pad3;
	};
	// Add overload to re-opened namespace. Important for template programming.
	ENTANGLEMENT int namespace_one(int,int);
};
