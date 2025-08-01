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

StructFundamentals function_struct_fundamentals_multiply(StructFundamentals struct_fundamentals, int multiplier) {
	struct_fundamentals.m_bool = !struct_fundamentals.m_bool;
	struct_fundamentals.m_char0 *= multiplier;
	struct_fundamentals.m_char1 *= multiplier;
	struct_fundamentals.m_char2 *= multiplier;
	struct_fundamentals.m_int0 *= multiplier;
	struct_fundamentals.m_int1 *= multiplier;
	struct_fundamentals.m_uint2 *= multiplier;
	struct_fundamentals.m_double *= multiplier;
	return struct_fundamentals;
}

StructPointerFundamentals& function_struct_pointer_fundamentals_multiply(StructPointerFundamentals& struct_fundamentals, int multiplier) {
	// Nuke the base without nuking the .vtable.
	StructFundamentals& base = struct_fundamentals;
	::memset(&base, 0xaf, sizeof(StructFundamentals));

	// Proof of life for pointers.
	*(int*)struct_fundamentals.m_void *= multiplier;
	*struct_fundamentals.m_char *= multiplier;
	*struct_fundamentals.m_wchar_t *= multiplier;
	*struct_fundamentals.m_bool = !*struct_fundamentals.m_bool;
	*struct_fundamentals.m_float *= multiplier;
	*struct_fundamentals.m_double *= multiplier;
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
	m_void = 0;
	m_char = 0;
	m_wchar_t = 0;
	m_bool = 0;
	m_float = 0;
	m_double = 0;
}
