// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxStringLiteralHashDebug - A run time version of hxStringLiteralHash.

extern "C"
uint32_t hxStringLiteralHashDebug(const char* s) {
	uint32_t x = 0u;
	size_t i = strlen(s);
	i = (i <= 192u) ? i : 192u; // match limits of hxStringLiteralHash macro
	while (i--) {
		x = (uint32_t)0x01000193 * x ^ (uint32_t)s[i]; // FNV prime.
	}
	return x;
}
