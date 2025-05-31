#pragma once
// Copyright 2017-2025 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

// ----------------------------------------------------------------------------
// Compile time string hashing.
//
// To log filename hashes in a debug build, add HX_REGISTER_FILENAME_HASH to C++ source files.

// Compiles string constants up to length 192 to a hash value without a constexpr.
// constexpr isn't enough to force the compiler to run this at compile time.
#define HX_H1(s_,i_,x_)   ((uint32_t)0x01000193*x_^(uint32_t)s_[(i_)<sizeof(s_)?(i_):(sizeof(s_)-1)])
#define HX_H4(s_,i_,x_)   HX_H1(s_,i_,HX_H1(s_,i_+1,HX_H1(s_,i_+2,HX_H1(s_,i_+3,x_))))
#define HX_H16(s_,i_,x_)  HX_H4(s_,i_,HX_H4(s_,i_+4,HX_H4(s_,i_+8,HX_H4(s_,i_+12,x_))))
#define HX_H64(s_,i_,x_)  HX_H16(s_,i_,HX_H16(s_,i_+16,HX_H16(s_,i_+32,HX_H16(s_,i_+48,x_))))
#define hxStringLiteralHash(s_) (uint32_t)HX_H64(s_,0,HX_H64(s_,64,HX_H64(s_,128,(uint32_t)0)))

// ----------------------------------------------------------------------------
// HX_REGISTER_FILENAME_HASH
//
// Registers hash of __FILE__ to be logged in a debug build.  This information
// will be needed to identify file name hashes in release builds.

#if HX_CPLUSPLUS && (HX_RELEASE) < 1
// Do not use, this is just the implementation of HX_REGISTER_FILENAME_HASH.
struct hxRegisterFileConstructor { hxRegisterFileConstructor(const char* s_); };

#define HX_REGISTER_FILENAME_HASH static hxRegisterFileConstructor s_hxRegisterFileConstructor(__FILE__);
#else
#define HX_REGISTER_FILENAME_HASH
#endif
