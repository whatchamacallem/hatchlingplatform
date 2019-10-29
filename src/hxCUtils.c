// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------------
// C utils

int g_hxIsInit; // Do not initialize to 0.  MSVC actually handles that differently.

#if defined(__clang__)
__attribute__((no_sanitize("address")))
#endif
void hxHexDump(const void* address, uint32_t bytes, int pretty) {
	if ((HX_RELEASE) < 2 && address != hxnull) {
		bytes = (bytes + 15u) & ~(uint32_t)15; // round up to 16 bytes.
		uint8_t* addr = (uint8_t*)address;
		for (uint32_t i = 0; i < bytes;) {
			if (pretty) {
				hxLogConsole("%08x: ", (unsigned int)(uintptr_t)addr);
			}
			uint8_t* str = addr;
			for (int32_t maximum = 4; i < bytes && maximum--; i += 4) {
				hxLogConsole("%02x %02x %02x %02x  ", addr[0], addr[1], addr[2], addr[3]);
				addr += 4;
			}
			if (pretty) {
				while (str < addr) {
					hxLogConsole("%c", (*str >= 0x20 && *str <= 0x7e) ? *str : '.');
					++str;
				}
			}
			hxLogConsole("\n");
		}
	}
}

#if defined(__clang__)
__attribute__((no_sanitize("address")))
#endif
void hxFloatDump(const float* address, uint32_t count) {
	if ((HX_RELEASE) < 2 && address != hxnull) {
		for (uint32_t i = 0; i < count;) {
			hxLogConsole("%08x: ", (unsigned int)(uintptr_t)address);
			for (int32_t maximum = 4; i < count && maximum--; i++) {
				hxLogConsole("%8f ", *address++);
			}
			hxLogConsole("\n");
		}
	}
}

const char* hxBasename(const char* path) {
	const char* result = path;
	if (path) {
		for (const char* it = path; *it != '\0'; ++it) {
			if (*it == '/' || *it == '\\') {
				result = it + 1;
			}
		}
	}
	return result;
}

char* hxStringDuplicate(const char* string, enum hxMemoryManagerId id) {
	if (string) {
		size_t len = strlen(string);
		char* temp = (char*)hxMallocExt(len + 1, id, 0u);
		memcpy(temp, string, len + 1);
		return temp;
	}
	else {
		return hxnull;
	}
}
