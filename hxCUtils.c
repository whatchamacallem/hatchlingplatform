// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"

int g_hxIsInit; // Do not initialize to 0.  MSVC actually handles that differently.

void hxHexDump(const void* address, uint32_t bytes, const char* label) {
	hxAssertRelease(address && label, "null arg");
	if ((HX_RELEASE) < 2) {
		uint8_t* addr = (uint8_t*)address;
		hxLogRelease("========= %s (%u bytes) =========\n", label, (unsigned int)bytes);
		bytes = (bytes + 15u) & ~(uint32_t)15; // round up to 16 bytes.
		for (uint32_t i = 0; i < bytes;) {
			hxLogRelease("%08x: ", (unsigned int)(uintptr_t)addr);
			uint8_t* str = addr;
			for (int32_t max = 4; i < bytes && max--; i += 4) {
				hxLogRelease("%02x %02x %02x %02x  ", addr[0], addr[1], addr[2], addr[3]);
				addr += 4;
			}
			while (str < addr) {
				hxLogRelease("%c", (*str >= 0x20 && *str <= 0x7e) ? *str : '.');
				++str;
			}
			hxLogRelease("\n");
		}
	}
}

void hxFloatDump(const float* address, uint32_t count, const char* label) {
	hxAssertRelease(address && label, "null arg");
	if ((HX_RELEASE) < 2) {
		hxLogRelease("========= %s (%u values) =========\n", label, (unsigned int)count);
		for (uint32_t i = 0; i < count;) {
			hxLogRelease("%08x: ", (unsigned int)(uintptr_t)address);
			for (int32_t max = 4; i < count && max--; i++) {
				hxLogRelease("%8f ", *address++);
			}
			hxLogRelease("\n");
		}
	}
}

const char* hxBasename(const char* path) {
	hxAssertRelease(path, "null arg");
	if (path) {
		const char* result = path;
		for (const char* it = path; *it != '\0'; ++it) {
			if (*it == '/' || *it == '\\') {
				result = it + 1;
			}
		}
		return result;
	}
	else {
		return hx_null;
	}
}

char* hxStringDuplicate(const char* string, enum hxMemoryManagerId allocatorId) {
	hxAssertRelease(string, "null arg");
	if (string) {
		size_t len = strlen(string);
		char* temp = (char*)hxMallocExt(len + 1, allocatorId, 0u);
		strcpy(temp, string);
		return temp;
	}
	else {
		return hx_null;
	}
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
