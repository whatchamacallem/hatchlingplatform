#pragma once
// Copyright 2017-2019 Adrian Johnston

#ifndef HATCHLING_H
#error #include <hx/hatchling.h>
#endif

// ----------------------------------------------------------------------------
// Compile time string hashing.
//
// To log filename hashes in a debug build, add HX_REGISTER_FILENAME_HASH to C++ source files.

#if HX_USE_CPP14_CONSTEXPR
// C++14 compile time hashing for string constants, used by hxAssertRelease().
template<size_t len>
HX_INLINE constexpr uint32_t hxStringLiteralHash(const char(&s)[len]) {
	uint32_t x = 0u;
	size_t i = (len <= (size_t)192u) ? len : (size_t)192u;
	while (i--) {
		x = (uint32_t)0x01000193 * x ^ (uint32_t)s[i]; // FNV prime.
	}
	return x;
}
#elif (HX_RELEASE) > 0
// Compiles string constants up to length 192 to a hash value without a constexpr.
#define HX_H1(s,i,x)   ((uint32_t)0x01000193*x^(uint32_t)s[(i)<sizeof(s)?(i):(sizeof(s)-1)])
#define HX_H4(s,i,x)   HX_H1(s,i,HX_H1(s,i+1,HX_H1(s,i+2,HX_H1(s,i+3,x))))
#define HX_H16(s,i,x)  HX_H4(s,i,HX_H4(s,i+4,HX_H4(s,i+8,HX_H4(s,i+12,x))))
#define HX_H64(s,i,x)  HX_H16(s,i,HX_H16(s,i+16,HX_H16(s,i+32,HX_H16(s,i+48,x))))
#define hxStringLiteralHash(s) (uint32_t)HX_H64(s,0,HX_H64(s,64,HX_H64(s,128,(uint32_t)0)))
#else
#define hxStringLiteralHash hxStringLiteralHashDebug
#endif

// ----------------------------------------------------------------------------
// HX_REGISTER_FILENAME_HASH
//
// Registers hash of __FILE__ to be logged in a debug build.

#if __cplusplus && (HX_RELEASE) < 1
// Do not use, implementation of HX_REGISTER_FILENAME_HASH.
struct hxRegisterFileConstructor { hxRegisterFileConstructor(const char* s); };

#define HX_REGISTER_FILENAME_HASH static hxRegisterFileConstructor s_hxRegisterFileConstructor(__FILE__);
#else
#define HX_REGISTER_FILENAME_HASH
#endif
