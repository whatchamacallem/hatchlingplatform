// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// C utils

// g_hxIsInit. Do not initialize to 0. MSVC actually handles that differently.
int g_hxIsInit;

// g_hxSettings. Declared here in plain C for maximum portability.
struct hxSettings g_hxSettings;

#if defined(__clang__)
__attribute__((no_sanitize("address")))
#endif
void hxHexDump(const void* address, size_t bytes, int pretty) {
	if ((HX_RELEASE) < 2 && address != hxnull) {
		bytes = (bytes + 15u) & ~(size_t)15; // round up to 16 bytes.
		const uint8_t* addr = (const uint8_t*)address;
		for (size_t i = 0; i < bytes;) {
			if (pretty) {
				hxLogConsole("%08x: ", (unsigned int)(uintptr_t)addr);
			}
			const uint8_t* str = addr;
			for (size_t maximum = 4; i < bytes && maximum--; i += 4) {
				hxLogConsole("%02x%02x%02x%02x ", addr[0], addr[1], addr[2], addr[3]);
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
void hxFloatDump(const float* address, size_t count) {
	if ((HX_RELEASE) < 2 && address != hxnull) {
		for (size_t i = 0; i < count;) {
			hxLogConsole("%08x: ", (unsigned int)(uintptr_t)address);
			for (size_t maximum = 4; i < count && maximum--; i++) {
				hxLogConsole("%8f ", *address++);
			}
			hxLogConsole("\n");
		}
	}
}

const char* hxBasename(const char* path) {
	const char* result = path ? path : "(null)";
	for (const char* it = result; *it != '\0'; ++it) {
		if (*it == '/' || *it == '\\') {
			result = it + 1;
		}
	}
	return result;
}

char* hxStringDuplicate(const char* string, enum hxMemoryAllocator id) {
	if (string) {
		size_t len = strlen(string);
		char* temp = (char*)hxMallocExt(len + 1, id, 0u);
		memcpy(temp, string, len + 1);
		return temp;
	}
	return hxnull;
}
