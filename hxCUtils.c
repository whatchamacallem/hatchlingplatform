// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"

int g_hxIsInit = 0; // Does not require global constructors to set.

void hxHexDump(const void* p, uint32_t bytes, const char* label) {
	hxAssertRelease(p && label, "null arg");
	uint8_t* ptr = (uint8_t*)p;
	hxLogRelease("========= %s (%u bytes) =========\n", label, (unsigned int)bytes);
	for (uint32_t i = 0; i < bytes;) {
		hxLogRelease("%08x  ", (unsigned int)(uintptr_t)(ptr + i));
		for (int32_t max = 4; i < bytes && max--; i += 4) {
			hxLogRelease("%08x ", (ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3]));
			ptr += 4;
		}
		hxLogRelease("\n");
	}
}

void hxFloatDump(const float* ptr, uint32_t count, const char* label) {
	hxAssertRelease(ptr && label, "null arg");
	hxLogRelease("========= %s (%u values) =========\n", label, (unsigned int)count);
	for (uint32_t i = 0; i < count;) {
		hxLogRelease("%08x  ", (unsigned int)(uintptr_t)(ptr + i));
		for (int32_t max = 4; i < count && max--; i++) {
			hxLogRelease("%8f ", ptr[i]);
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
