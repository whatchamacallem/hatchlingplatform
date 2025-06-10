#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// Unlike the standard, these functions require the same type for both args and
// cannot be instantiated without deducing the arg type. This provides better
// type safety. Use std::less<void> if you want a functor that takes mixed types.

// hxKeyLess - User overloadable function for performing comparisons. Invokes
// operator <.
template<typename T_>
HX_CONSTEXPR_FN bool hxKeyLess(const T_& lhs_, const T_& rhs_) { return lhs_ < rhs_; }

// hxKeyHash (const char*) - Uses a constexpr "strcmp(a, b) < 0".
HX_CONSTEXPR_FN bool hxKeyLess(const char* a_, const char* b_) {
    while(*a_ != '\0' && *a_ == *b_) { ++a_; ++b_; }
    return *a_ < *b_;
}

// hxKeyHash - Used by the base class hash table node. It needs to be overridden
// for your key type. Overrides are evaluated when and where the hash table is
// instantiated. Uses the well studied hash multiplier taken from Linux's hash.h
template<typename T_>
HX_CONSTEXPR_FN uint32_t hxKeyHash(const T_& x_ ) {
	return (uint32_t)x_ * (uint32_t)0x61C88647u;
};

// hxKeyHash (const char*) - Uses FNV-1a string hashing.
HX_CONSTEXPR_FN uint32_t hxKeyHash(const char* k_) {
    uint32_t x_ = (uint32_t)0x811c9dc5;
    while (*k_ != '\0') {
		x_ ^= (uint32_t)*k_++;
		x_ *= (uint32_t)0x01000193;
	}
	return x_;
}

// hxKeyEqual - Used by the hash table. If your key type doesn't support
// operator== then this function needs to be overridden for your key type.
// Overrides are evaluated when and where the hash table is instantiated.
template<typename T>
HX_CONSTEXPR_FN bool hxKeyEqual(const T& a_, const T& b_) {
	return a_ == b_;
}

// hxKeyHash (const char*) - Uses a constexpr strcmp.
HX_CONSTEXPR_FN bool hxKeyEqual(const char* a_, const char* b_) {
	while(*a_ != '\0' && *a_ == *b_) { ++a_; ++b_; }
	return *a_ == '\0' && *b_ == '\0';
}
