#pragma once
// Copyright 2017-2019 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h>
#endif

// ----------------------------------------------------------------------------
// Compile time string hashing.
//
// To log filename hashes in a debug build, add HX_REGISTER_FILENAME_HASH to C++ source files.

#if defined(__cplusplus) && HX_USE_CPP14_CONSTEXPR
// C++14 compile time hashing for string constants, used by hxAssertRelease().
template<size_t len_>
constexpr uint32_t hxStringLiteralHash(const char(&s_)[len_]) {
	uint32_t x_ = 0u;
	size_t i_ = (len_ <= (size_t)192u) ? len_ : (size_t)192u;
	while (i_--) {
		x_ = (uint32_t)0x01000193 * x_ ^ (uint32_t)s_[i_]; // FNV-1a prime.
	}
	return x_;
}
#elif (HX_RELEASE) > 0
// Compiles string constants up to length 192 to a hash value without a constexpr.
#define HX_H1(s_,i_,x_)   ((uint32_t)0x01000193*x_^(uint32_t)s_[(i_)<sizeof(s_)?(i_):(sizeof(s_)-1)])
#define HX_H4(s_,i_,x_)   HX_H1(s_,i_,HX_H1(s_,i_+1,HX_H1(s_,i_+2,HX_H1(s_,i_+3,x_))))
#define HX_H16(s_,i_,x_)  HX_H4(s_,i_,HX_H4(s_,i_+4,HX_H4(s_,i_+8,HX_H4(s_,i_+12,x_))))
#define HX_H64(s_,i_,x_)  HX_H16(s_,i_,HX_H16(s_,i_+16,HX_H16(s_,i_+32,HX_H16(s_,i_+48,x_))))
#define hxStringLiteralHash(s_) (uint32_t)HX_H64(s_,0,HX_H64(s_,64,HX_H64(s_,128,(uint32_t)0)))
#else
#define hxStringLiteralHash hxStringLiteralHashDebug
#endif

// ----------------------------------------------------------------------------
// HX_REGISTER_FILENAME_HASH
//
// Registers hash of __FILE__ to be logged in a debug build.

#if defined(__cplusplus) && (HX_RELEASE) < 1
// Do not use, implementation of HX_REGISTER_FILENAME_HASH.
struct hxRegisterFileConstructor { hxRegisterFileConstructor(const char* s_); };

#define HX_REGISTER_FILENAME_HASH static hxRegisterFileConstructor s_hxRegisterFileConstructor(__FILE__);
#else
#define HX_REGISTER_FILENAME_HASH
#endif
