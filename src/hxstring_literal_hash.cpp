// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

HX_REGISTER_FILENAME_HASH

// hxstring_literal_hash_debug - A run time version of hxstring_literal_hash.

extern "C"
uint32_t hxstring_literal_hash_debug(const char* s) {
	uint32_t x = 0u;
	size_t i = hxmin<size_t>(strlen(s), 192u); // match limits of hxstring_literal_hash macro
	while (i--) {
		x = (uint32_t)0x01000193 * x ^ (uint32_t)s[i]; // FNV-a1 prime.
	}
	return x;
}
