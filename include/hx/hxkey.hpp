#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//
// <hx/hxkey.hpp> - User overloadable key-equal, key-less and key-hash functions.
// This code will only use the == and < operators by default. That will work
// with a default or custom <=> operator. Alternately these calls can be
// overriden to resolve key operations without global operator overloads.

#include <hx/hatchling.h>

/// `hxkey_equal(const T& a, const T& b)` - Compares two objects for equivalence.
/// If your key type doesn't support `operator==` then this function may need to
/// be overridden for your key type. Function overloads are evaluated when and where
/// the derived container is instantiated and need to be consistently available.
template<typename T_>
constexpr bool hxkey_equal(const T_& a_, const T_& b_) {
	return a_ == b_;
}

/// `hxkey_equal(const char* a, const char* b)` is `strcmp(a, b) == 0`.
inline bool hxkey_equal(const char* a_, const char* b_) {
	return ::strcmp(a_, b_) == 0;
}

/// Utility for making function pointers to `hxkey_equal` from a partially specialized
/// set of overloaded functions. E.g. `hxkey_equal_function<int>()(1, 7)`
template<typename T_>
inline bool(*hxkey_equal_function(void))(const T_&, const T_&) {
	return static_cast<bool(*)(const T_&, const T_&)>(hxkey_equal<T_>);
}

/// `hxkey_less(const T&, const T&)` - User overloadable function for performing comparisons.
/// Invokes `operator<` by default. All the other comparison operators can be written using
/// `operator<`. However hxkey_equal is also used for efficiency.
template<typename T_>
constexpr bool hxkey_less(const T_& a_, const T_& b_) {
	return a_ < b_;
}

/// `hxkey_less(const T*, const T*)` - User overloadable function for performing comparisons.
/// Invokes `T::operator<` by default. Pointer `<` comparisons are not available by default
/// because that is undefined behavior unless the pointers are from the same array. For
/// example the compiler may silently ignore comparisons between function pointers.
template<typename T_>
constexpr bool hxkey_less(const T_* a_, const T_* b_) {
	return hxkey_less(*a_, *b_);
}

/// `hxkey_less(const char*, const char*)` is `strcmp(a, b) < 0`.
inline bool hxkey_less(const char* a_, const char* b_) {
	return ::strcmp(a_, b_) < 0;
}

/// Utility for making function pointers to `hxkey_less` from a partially specialized
/// set of overloaded functions. E.g. `hxkey_less_function<int>()(78, 77)`
template<typename T_>
inline bool (*hxkey_less_function(void))(const T_&, const T_&) {
	return static_cast<bool(*)(const T_&, const T_&)>(hxkey_less<T_>);
}

/// `hxkey_hash(T)` - Used by the base class hash table node. It needs to be overridden
/// for your key type. Overrides are evaluated when and where the hash table is
/// instantiated. Uses the well studied hash multiplier taken from Linux's hash.h
template<typename T_>
constexpr hxhash_t hxkey_hash(T_ t_) {
	return (hxhash_t)t_ * (hxhash_t)0x61C88647u;
};

/// `hxkey_hash(const char*)` - Uses FNV-1a string hashing.
inline hxhash_t hxkey_hash(const char* k_) {
	hxhash_t x_ = (hxhash_t)0x811c9dc5;
	while (*k_ != '\0') {
		x_ ^= (hxhash_t)*k_++;
		x_ *= (hxhash_t)0x01000193;
	}
	return x_;
}
