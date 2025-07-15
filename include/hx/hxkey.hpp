#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// Unlike the standard, these functions require the same type for both args and
// cannot be instantiated without deducing the arg type. This provides better
// type safety.

/// `hxkey_equal` - Used by the hash table. If your key type doesn't support
/// operator== then this function needs to be overridden for your key type.
/// Overrides are evaluated when and where the hash table is instantiated.
template<typename T>
hxconstexpr_fn bool hxkey_equal(const T& a_, const T& b_) {
	return a_ == b_;
}

/// `hxkey_hash(const char*)` - Uses a constexpr strcmp.
hxconstexpr_fn bool hxkey_equal(const char* a_, const char* b_) {
	while(*a_ != '\0' && *a_ == *b_) { ++a_; ++b_; }
	return *a_ == '\0' && *b_ == '\0';
}

/// `hxkey_hash` - Used by the base class hash table node. It needs to be overridden
/// for your key type. Overrides are evaluated when and where the hash table is
/// instantiated. Uses the well studied hash multiplier taken from Linux's hash.h
template<typename T_>
hxconstexpr_fn hxhash_t hxkey_hash(const T_& x_ ) {
	return (hxhash_t)x_ * (hxhash_t)0x61C88647u;
};

/// `hxkey_hash(const char*)` - Uses FNV-1a string hashing.
hxconstexpr_fn hxhash_t hxkey_hash(const char* k_) {
    hxhash_t x_ = (hxhash_t)0x811c9dc5;
    while (*k_ != '\0') {
		x_ ^= (hxhash_t)*k_++;
		x_ *= (hxhash_t)0x01000193;
	}
	return x_;
}

/// `hxkey_less` - User overloadable function for performing comparisons. Invokes
/// operator <.
template<typename T_>
hxconstexpr_fn bool hxkey_less(const T_& a_, const T_& b_) {
	return a_ < b_;
}

/// `hxkey_hash (const char*)` - Uses a constexpr "strcmp(a, b) < 0".
hxconstexpr_fn bool hxkey_less(const char* a_, const char* b_) {
    while(*a_ != '\0' && *a_ == *b_) { ++a_; ++b_; }
    return *a_ < *b_;
}
