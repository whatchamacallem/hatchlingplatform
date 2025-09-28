#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxstring_literal_hash.h This code is used to hash filenames so that
/// they are not added to the release build. This provides asserts in a low RAM
/// or limited download size scenario.

// TODO: Try a consteval version that uses basenames at level 1 and hashes
// basenames at level 2.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead.
#endif

/// `hxhash_t` - Unsigned 32-bit hash value. Expect collisions.
typedef uint32_t hxhash_t;

/// `hxstring_literal_hash` - Compile time string hashing. To log filename hashes in
/// a debug build, add `HX_REGISTER_FILENAME_HASH` to C++ source files. Compiles
/// string constants up to length `192` to a hash value without a `constexpr`.
/// `constexpr` isn't enough to force the compiler to run this at compile time.
#define HX_H1(s_,i_,x_)   ((hxhash_t)0x01000193*x_^(hxhash_t)s_[(i_)<sizeof(s_)?(i_):(sizeof(s_)-1)])
#define HX_H4(s_,i_,x_)   HX_H1(s_,i_,HX_H1(s_,i_+1,HX_H1(s_,i_+2,HX_H1(s_,i_+3,x_))))
#define HX_H16(s_,i_,x_)  HX_H4(s_,i_,HX_H4(s_,i_+4,HX_H4(s_,i_+8,HX_H4(s_,i_+12,x_))))
#define HX_H64(s_,i_,x_)  HX_H16(s_,i_,HX_H16(s_,i_+16,HX_H16(s_,i_+32,HX_H16(s_,i_+48,x_))))
#define hxstring_literal_hash(s_) (hxhash_t)HX_H64(s_,0,HX_H64(s_,64,HX_H64(s_,128,(hxhash_t)0)))

#if HX_CPLUSPLUS && (HX_RELEASE) < 1
/// `hxregister_string_literal_hash` - Intended for use as a global constructor to
/// register the hashes of string literals in debug so they can be identified
/// when part of release mode messages. Used to implement `HX_REGISTER_FILENAME_HASH`.
/// This code avoids any memory allocations. See console commands `printhashes`
/// and `checkhash`.
class hxregister_string_literal_hash {
public:
	typedef hxhash_t key_t;

	// permanently adds object to hxstring_literal_hashes_.
	hxregister_string_literal_hash(const char* str_);
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }
	hxhash_t key(void) const { return m_hash_; };
	hxhash_t hash(void) const; /// this is rehashed.
	const char* str(void) const { return m_str_; }

private:
	void* m_hash_next_;
	hxhash_t m_hash_;
	const char* m_str_;
};

/// `HX_REGISTER_FILENAME_HASH` - Registers hash of `__FILE__` to be logged in a debug
/// build. This information will be needed to identify file name hashes in release
/// builds.
#define HX_REGISTER_FILENAME_HASH static hxregister_string_literal_hash \
	s_hxregister_filename_hash(__FILE__);
#else
#define HX_REGISTER_FILENAME_HASH
#endif

#if HX_CPLUSPLUS
extern "C" {
#endif

/// `hxstring_literal_hash_debug` - Calculates a string hash at runtime that is the
/// same as `hxstring_literal_hash`.
/// - `string` : The null-terminated string to hash.
hxhash_t hxstring_literal_hash_debug(const char* string_) hxattr_nonnull(1);

#if HX_CPLUSPLUS
} // extern "C"
#endif
