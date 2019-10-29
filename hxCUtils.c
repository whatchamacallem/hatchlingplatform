// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"

int g_hxIsInit; // Do not initialize to 0.  MSVC actually handles that differently.

void hxHexDump(const void* p, uint32_t bytes, const char* label) {
	hxAssertRelease(p && label, "null arg");
	uint8_t* ptr = (uint8_t*)p;
	hxLogRelease("========= %s (%u bytes) =========\n", label, (unsigned int)bytes);
	bytes = (bytes + 15u) & ~(uint32_t)15; // round up to 16 bytes.
	for (uint32_t i = 0; i < bytes;) {
		hxLogRelease("%08x: ", (unsigned int)(uintptr_t)ptr);
		uint8_t* str = ptr;
		for (int32_t max = 4; i < bytes && max--; i += 4) {
			hxLogRelease("%02x %02x %02x %02x  ", ptr[0], ptr[1], ptr[2], ptr[3]);
			ptr += 4;
		}
		while (str < ptr) {
			hxLogRelease("%c", (*str >= 0x20 && *str <= 0x7e) ? *str : '.');
			++str;
		}
		hxLogRelease("\n");
	}
}

void hxFloatDump(const float* ptr, uint32_t count, const char* label) {
	hxAssertRelease(ptr && label, "null arg");
	hxLogRelease("========= %s (%u values) =========\n", label, (unsigned int)count);
	for (uint32_t i = 0; i < count;) {
		hxLogRelease("%08x: ", (unsigned int)(uintptr_t)ptr);
		for (int32_t max = 4; i < count && max--; i++) {
			hxLogRelease("%8f ", *ptr++);
		}
		hxLogRelease("\n");
	}
}

const char* hxBasename(const char* path) {
	hxAssertRelease(path, "null arg");
	const char* result = path;
	for (const char* it = path; *it != '\0'; ++it) {
		if (*it == '/' || *it == '\\') {
			result = it + 1;
		}
	}
	return result;
}

char* hxStringDuplicate(const char* s, enum hxMemoryManagerId allocatorId) {
	hxAssertRelease(s, "null arg");
	size_t len = strlen(s);
	char* d = (char*)hxMallocExt(len + 1, allocatorId, 0u);
	strcpy(d, s); // d = s
	return d;
}

uint32_t hxHashStringLiteralDebug(const char* s) {
	uint32_t x = 0u;
	size_t i = strlen(s);
	i = (i <= 192u) ? i : 192u; // match limits of hxHashStringLiteral macro
	while (i--) {
		x = (uint32_t)0x61C88647 * x ^ (uint32_t)s[i];
	}
	return x;
}
