// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxutility.h"

// The non-inline utility functions are all in plain C.

#if defined __clang__
__attribute__((no_sanitize("address")))
#endif
void hxhex_dump(const void* address, size_t bytes, int pretty) {
	if((HX_RELEASE) < 2) {
		bytes = (bytes + 15u) & ~(size_t)15; // round up to 16 bytes.
		const volatile uint8_t* addr = (const uint8_t*)address;
		for(size_t i = 0; i < bytes;) {
			if(pretty) {
				// Adjust the number of leading zeros for pointers to match uintptr_t.
				hxlogconsole("%0*zx: ", (int)sizeof(uintptr_t), (size_t)addr);
			}
			const volatile uint8_t* str = addr;
			for(size_t maximum = 4; i < bytes && maximum--; i += 4) {
				hxlogconsole("%02x%02x%02x%02x ", addr[0], addr[1], addr[2], addr[3]);
				addr += 4;
			}
			if(pretty) {
				while(str < addr) {
					hxlogconsole("%c", (*str >= 0x20 && *str <= 0x7e) ? *str : '.');
					++str;
				}
			}
			hxlogconsole("\n");
		}
	}
}

#if defined __clang__
__attribute__((no_sanitize("address")))
#endif
void hxfloat_dump(const float* address, size_t count) {
	(void)address;
	if((HX_RELEASE) < 2) {
		for(size_t i = 0; i < count;) {
			hxlogconsole("%08x: ", (unsigned int)(uintptr_t)address);
			for(size_t maximum = 4; i < count && maximum--; i++) {
				hxlogconsole("%8f ", *address++);
			}
			hxlogconsole("\n");
		}
	}
}

const char* hxbasename(const char* path) {
	for(const char* it = path; *it != '\0'; ++it) {
		if(*it == '/' || *it == '\\') {
			path = it + 1;
		}
	}
	return path;
}

char* hxstring_duplicate(const char* string, enum hxsystem_allocator_t id) {
	const size_t len = strlen(string);
	char* temp = (char*)hxmalloc_ext(len + 1, id, 0u);
	memcpy(temp, string, len + 1);
	return temp;
}
