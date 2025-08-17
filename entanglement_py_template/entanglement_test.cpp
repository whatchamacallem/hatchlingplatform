// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "entanglement_test.hpp"

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

void function_overload() { }

int function_overload(int a, int b) { (void)a; (void)b; return -1; }

float function_overload(int, int, int, int) { return -2.0; }

int8_t* function_pointer_int8(int8_t* x, size_t size, int8_t value) {
	while(size--) { x[size] = value + size; }
	return x;
}

uint16_t* function_pointer_uint16(uint16_t* x, size_t size, int16_t value) {
	while(size--) { x[size] = value + size; }
	return x;
}

int32_t* function_pointer_int32(int32_t* x, size_t size, int32_t value) {
	while(size--) { x[size] = value + size; }
	return x;
}

uint64_t* function_pointer_uint64(uint64_t* x, size_t size, int64_t value) {
	while(size--) { x[size] = value + size; }
	return x;
}

void* function_pointer_void_to_int(void* x_, size_t size, int value) {
	int* x = reinterpret_cast<int*>(x_);
	while(size--) { x[size] = value + size; }
	return x;
}

char* function_pointer_char(char* x) {
	::strcpy(x, "🐉🐉🐉 A");
	return x;
}

wchar_t* function_pointer_wchar(wchar_t* x) {
	::wcscpy(x, L"🐉🐉🐉 B");
	return x;
}

bool& function_ref_bool(bool& x, bool value) { x = value; return x; }

uint16_t& function_ref_uint16(uint16_t& x, uint16_t value) { x = value; return x; }

void function_ref_wchar(wchar_t& x, wchar_t value) { x = value; }

uint64_t& function_ref_uint64(uint64_t& x, uint64_t value) { x = value; return x; }

extern "C" {

StructFundamentals function_struct_fundamentals_multiply(StructFundamentals struct_fundamentals, int multiplier) {
	struct_fundamentals.m_bool = !struct_fundamentals.m_bool;
	struct_fundamentals.m_char0 *= multiplier;
	struct_fundamentals.m_char1 *= multiplier;
	struct_fundamentals.m_char2 *= multiplier;
	struct_fundamentals.m_int0 *= multiplier;
	struct_fundamentals.m_int1 *= multiplier;
	struct_fundamentals.m_uint2 *= multiplier;
	struct_fundamentals.m_double[0] *= multiplier;
	return struct_fundamentals;
}

} // extern "C" {

StructPointerFundamentals& function_struct_pointer_fundamentals_multiply(StructPointerFundamentals& struct_fundamentals, int multiplier) {

	// Nuke the base without nuking the .vtable.
	StructFundamentals& base = struct_fundamentals;
	::memset(&base, 0xaf, sizeof(StructFundamentals));

	// Proof of life for pointers.
	*(int*)struct_fundamentals.m_pvoid *= multiplier;
	*struct_fundamentals.m_pbool = !*struct_fundamentals.m_pbool;
	*struct_fundamentals.m_pfloat *= multiplier;
	return struct_fundamentals;
}

ENTANGLEMENT void function_struct_pointer_fundamentals_multiply2(StructPointerFundamentals* struct_fundamentals, int multiplier) {
	function_struct_pointer_fundamentals_multiply(*struct_fundamentals, multiplier);
}

StructPointerFundamentals::StructPointerFundamentals(void) {
	m_pvoid = 0;
	m_pbool = 0;
	m_pfloat = 0;
}

StructPointerFundamentals::~StructPointerFundamentals(void) {
}

// Returns only '<=' not 'operator<='.
static wchar_t* OperatorTest_to_unicode(const char* narrow_str) {
    static wchar_t s_buf[4];
    ::mbstowcs(s_buf, narrow_str + (sizeof "operator") - 1, 4);
    return s_buf;
}

OperatorTest::OperatorTest(void) : pad(0) {}
OperatorTest::OperatorTest(int) : pad(1) {}
OperatorTest::~OperatorTest() {}
bool OperatorTest::__bool__(void) const{ return true; }
wchar_t* OperatorTest::operator+(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator&(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator()(size_t) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator==(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator>=(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator>(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator[](size_t) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator&=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator+=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator<<=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator*=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator|=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator%=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator~() const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator^=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator-=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator/=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator>>=(const OperatorTest&){ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator<=(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator<<(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator<(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator%(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator*(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator!=(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator|(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator>>(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator-(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator+() const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator-() const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator/(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }
wchar_t* OperatorTest::operator^(const OperatorTest&) const{ return OperatorTest_to_unicode(__func__); }

EnumCStyleTwoConstants NameSpaceOne::NameSpaceOneClassOne::class_one_one(EnumCStyleTwoConstants) { return (EnumCStyleTwoConstants)10; }
EnumInt16ThreeConstants NameSpaceOne::NameSpaceOneClassTwo::class_one_two(EnumInt16ThreeConstants) { return (EnumInt16ThreeConstants)20; }
int NameSpaceOne::namespace_one(int) { return 30; }
int NameSpaceTwo::NameSpaceTwoClassOne::class_two_one(int) { return 40; }
int NameSpaceTwo::namespace_two(int) { return 50; }
EnumScopedUInt64 NameSpaceOne::NameSpaceOneClassThree::class_one_three(EnumScopedUInt64) { return (EnumScopedUInt64)60; }
int NameSpaceOne::namespace_one(int,int) { return 70; }
