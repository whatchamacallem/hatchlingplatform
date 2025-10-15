#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxkey.hpp User-overloadable key-equal, key-less, and key-hash
/// functions. By default this code uses only the `==` and `<` operators, which
/// works with either the default or a custom `<=>` operator. Alternatively,
/// these functions can be overloaded to resolve key operations without global
/// operator overloads. This code uses C++20 concepts when available and
/// provides no fallbacks for SFINAE otherwise. Partial specialization does not
/// work before C++20. As an alternative, functors are recommended and supported
/// for complex use cases because they are relatively easy to debug. See
/// `hxkey_equal_function` and `hxkey_less_function` for generating default
/// functors.

#include "hatchling.h"
#include "hxutility.h"

#if HX_CPLUSPLUS >= 202002L
/// A concept that requires one type to be convertible to another. See usage
/// below. The compiler applies some unintuitive rules when evaluating this.
/// - `from_t` : The source type.
/// - `to_t` : The target type.
template<typename from_t_, typename to_t_>
concept hxconvertible_to = requires(from_t_ (&&from_)()) {
	// from_ is an unexecuted function pointer used to provide a from_t_&&. This
	// is what the standard does and avoids requiring other operators.
    requires requires { static_cast<to_t_>(from_()); };
};
#endif

/// `hxkey_equal(const A& a, const B& b)` - Returns true if two objects are
/// equivalent. If your key type does not support `operator==`, then this
/// function may need to be overridden for your key type(s). Function overloads
/// are evaluated when and where the derived container is instantiated and must
/// be consistently available. Uses `operator==`.
template<typename A_, typename B_>
#if HX_CPLUSPLUS >= 202002L
requires requires(const A_& a_, const B_& b_) { { a_ == b_ } -> hxconvertible_to<bool>; }
#endif
constexpr bool hxkey_equal(const A_& a_, const B_& b_) {
    return a_ == b_;
}

/// `hxkey_equal(const char* a, const char* b)` is `strcmp(a, b) == 0`.
/// Returns true if two C strings are equal (`strcmp(a, b) == 0`).
/// - `a` : The first C string.
/// - `b` : The second C string.
inline bool hxkey_equal(const char* const& a_, const char* const& b_) {
    return ::strcmp(a_, b_) == 0;
}

/// Utility for resolving function pointers to `hxkey_equal` from a partially
/// specialized set of overloaded functions. e.g.,
/// `hxkey_equal_function<int>()(1, 7)`.
template<typename T_>
inline bool(*hxkey_equal_function(void))(const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&) {
	return static_cast<bool(*)(const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&)>
        (hxkey_equal<const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&>);
}

/// `hxkey_less(const T&, const T&)` - User-overloadable function for performing
/// comparisons. Invokes `operator<` by default. All the other comparison
/// operators can be written using `operator<`. However `hxkey_equal` is also used
/// for efficiency. Returns true if the first object is less than the second.
/// Override for custom ordering.
/// - `a` : The first object.
/// - `b` : The second object.
template<typename A_, typename B_>
#if HX_CPLUSPLUS >= 202002L
requires requires(const A_& a_, const B_& b_) { { a_ < b_ } -> hxconvertible_to<bool>; }
#endif
constexpr bool hxkey_less(const A_& a_, const B_& b_) {
    return a_ < b_;
}

/// `hxkey_less(const T*, const T*)` - User overloadable function for performing
/// comparisons. Delegates to `T::operator<` by default. Pointer `<` comparisons
/// are not available by default because that is undefined behavior unless the
/// pointers are from the same array. For example, the compiler may silently
/// ignore comparisons between function pointers.
/// - `a` : Pointer to the first object.
/// - `b` : Pointer to the second object.
template<typename A_, typename B_>
#if HX_CPLUSPLUS >= 202002L
requires requires(const A_* a_, const B_* b_) { { hxkey_less(*a_, *b_) } -> hxconvertible_to<bool>; }
#endif
constexpr bool hxkey_less(const A_* a_, const B_* b_) {
    return hxkey_less(*a_, *b_);
}

/// `hxkey_less(const char*, const char*)` - Returns true if the first C string
/// is lexicographically less than the second by ASCII. UTF-8 is assigned a
/// stable ordering without looking up a locale. Uses (`strcmp(a, b) < 0`).
/// - `a` : The first C string.
/// - `b` : The second C string.
inline bool hxkey_less(const char* const& a_, const char* const& b_) {
    return ::strcmp(a_, b_) < 0;
}

/// Utility for resolving function pointers to `hxkey_less` from a partially
/// specialized set of overloaded functions. e.g.,
/// `hxkey_less_function<int>()(78, 77)`.
template<typename T_>
inline bool (*hxkey_less_function(void))(const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&) {
    return static_cast<bool(*)(const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&)>
        (hxkey_less<const hxremove_cvref_t<T_>&, const hxremove_cvref_t<T_>&>);
}

/// `hxkey_hash(T)` - Returns the hash of a numeric value. Used by the base hash
/// table node. It must be overridden for your key type. Overrides are evaluated
/// when and where the hash table is instantiated. Uses the well-studied hash
/// multiplier taken from Linux's `hash.h`.
/// - `x` : The input value.
template<typename T_>
constexpr hxhash_t hxkey_hash(T_ x_) {
    return (hxhash_t)x_ * (hxhash_t)0x61C88647u;
};

/// `hxkey_hash(const char*)` - Returns the FNV-1a hash of a C string. Uses
/// FNV-1a string hashing.
/// - `s` : The C string.
inline hxhash_t hxkey_hash(const char* s_) {
    hxhash_t x_ = (hxhash_t)0x811c9dc5;
    while(*s_ != '\0') {
        x_ ^= (hxhash_t)*s_++;
        x_ *= (hxhash_t)0x01000193;
    }
    return x_;
}
