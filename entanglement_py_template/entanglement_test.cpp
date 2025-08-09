// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "entanglement_test.hpp"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

int8_t function_roundtrip_int8(int8_t x) { return x; }

uint16_t function_roundtrip_uint16(uint16_t x) { return x; }

int32_t function_roundtrip_int32(int32_t x) { return x; }

uint64_t function_roundtrip_uint64(uint64_t x) { return x; }

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

StructPointerFundamentals::StructPointerFundamentals() {
	null_it_all();
}
StructPointerFundamentals::~StructPointerFundamentals() {
	// nuke .vtable to force a crash if ownership is incorrect.
	::memset((void*)this, 0x00, sizeof *this);
}
void StructPointerFundamentals::null_it_all(void) {
	m_pvoid = 0;
	m_pbool = 0;
	m_pfloat = 0;
}

OperatorTest::OperatorTest(int value_) : value(value_) { }

OperatorTest::OperatorTest() : value(0) {}

OperatorTest::OperatorTest(const OperatorTest& x) : value(x.value) {}

OperatorTest::~OperatorTest() {
    value = 0xafafafaf;
}

bool OperatorTest::operator<(const OperatorTest& x) const {
    return value < x.value;
}

bool OperatorTest::operator==(const OperatorTest& x) const {
    return value == x.value;
}

bool OperatorTest::operator!=(const OperatorTest& x) const {
    return value != x.value;
}

bool OperatorTest::operator>(const OperatorTest& x) const {
    return value > x.value;
}

bool OperatorTest::operator>=(const OperatorTest& x) const {
    return value >= x.value;
}

bool OperatorTest::operator<=(const OperatorTest& x) const {
    return value <= x.value;
}

OperatorTest OperatorTest::operator+() const {
    return OperatorTest(+value);
}

OperatorTest OperatorTest::operator-() const {
    return OperatorTest(-value);
}

OperatorTest OperatorTest::operator+(const OperatorTest& x) const {
    return OperatorTest(value + x.value);
}

OperatorTest OperatorTest::operator-(const OperatorTest& x) const {
    return OperatorTest(value - x.value);
}

OperatorTest OperatorTest::operator*(const OperatorTest& x) const {
    return OperatorTest(value * x.value);
}

OperatorTest OperatorTest::operator/(const OperatorTest& x) const {
    return OperatorTest(value / x.value);
}

OperatorTest OperatorTest::operator%(const OperatorTest& x) const {
    return OperatorTest(value % x.value);
}

OperatorTest& OperatorTest::operator+=(const OperatorTest& x) {
    value += x.value;
    return *this;
}

OperatorTest& OperatorTest::operator-=(const OperatorTest& x) {
    value -= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator*=(const OperatorTest& x) {
    value *= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator/=(const OperatorTest& x) {
    value /= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator%=(const OperatorTest& x) {
    value %= x.value;
    return *this;
}

bool OperatorTest::operator&(const OperatorTest& x) const {
    return (value & x.value) != 0;
}

bool OperatorTest::operator|(const OperatorTest& x) const {
    return (value | x.value) != 0;
}

bool OperatorTest::operator^(const OperatorTest& x) const {
    return (value ^ x.value) != 0;
}

OperatorTest OperatorTest::operator~() const {
    return OperatorTest(~value);
}

OperatorTest OperatorTest::operator<<(const OperatorTest& x) const {
    return OperatorTest(value << x.value);
}

OperatorTest OperatorTest::operator>>(const OperatorTest& x) const {
    return OperatorTest(value >> x.value);
}

OperatorTest& OperatorTest::operator&=(const OperatorTest& x) {
    value &= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator|=(const OperatorTest& x) {
    value |= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator^=(const OperatorTest& x) {
    value ^= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator<<=(const OperatorTest& x) {
    value <<= x.value;
    return *this;
}

OperatorTest& OperatorTest::operator>>=(const OperatorTest& x) {
    value >>= x.value;
    return *this;
}

bool OperatorTest::operator&&(const OperatorTest& x) const {
    return value && x.value;
}

bool OperatorTest::operator||(const OperatorTest& x) const {
    return value || x.value;
}

bool OperatorTest::operator!() const {
    return !value;
}

OperatorTest& OperatorTest::operator++() {
    ++value;
    return *this;
}

OperatorTest OperatorTest::operator++(int) {
    OperatorTest tmp(*this);
    ++value;
    return tmp;
}

OperatorTest& OperatorTest::operator--() {
    --value;
    return *this;
}

OperatorTest OperatorTest::operator--(int) {
    OperatorTest tmp(*this);
    --value;
    return tmp;
}

int OperatorTest::operator()(int add) const {
    return value + add;
}

int OperatorTest::operator[](int index) const {
    return value + index;
}
