// Copyright 2017 Adrian Johnston

#include "hatchling.h"

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxHashStringLiteralDebug - A run time version of hxHashStringLiteral.

extern "C"
uint32_t hxHashStringLiteralDebug(const char* s) {
	uint32_t x = 0u;
	size_t i = strlen(s);
	i = (i <= 192u) ? i : 192u; // match limits of hxHashStringLiteral macro
	while (i--) {
		x = (uint32_t)0x61C88647 * x ^ (uint32_t)s[i];
	}
	return x;
}
