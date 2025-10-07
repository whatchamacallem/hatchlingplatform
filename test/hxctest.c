// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxutility.h>

#include "hxctest.h"

#include <stdio.h>


// Make sure this all compiles and runs in C99.
bool hxctest_hatchling_h(void) {
	hxlog("Smoke testing from C: hxlog %d, ", 1);
	hxlogrelease("hxlogrelease %d, ", 2);
	hxlogconsole("hxlogconsole %d, ", 3);
	hxlogwarning("hxlogwarning %d", 4);
	hxassertmsg(1, "hxassertmsg %d", 5);
	hxassert(1 == 1); // Shouldn't warn because those may be errors.
	hxassertrelease(1.0, "hxassertrelease %f", 1.0f);
	hxwarnmsg("true", "not true"); // Cast pointer.
	return true; // Did not crash.
}

bool hxctest_math(void) {
	return hxmin(-3, 2) == -3
		&& hxmax(-3, 2) == 2
		&& hxmin(3u, 2u) == 2u
		&& hxmax(3u, 2u) == 3u
		&& hxabs(-2) == 2
		&& hxabs(2u) == 2u;
}

bool hxctest_clamp(void) {
	return hxclamp(0, 1, 5) == 1
		&& hxclamp(1, 1, 5) == 1
		&& hxclamp(5, 1, 5) == 5
		&& hxclamp(6, 1, 5) == 5;
}

bool hxctest_swap(void) {
	char a[] = { 3, 7 };
	hxswap_memcpy(a[0], a[1]);

	struct { unsigned int x; short pad; } b[] = { { 30u, -1 }, { 70u, -2 } };
	hxswap_memcpy(b[0], b[1]);

	return a[0] == 7 && a[1] == 3 && b[0].x == 70u && b[1].x == 30u;
}

bool hxctest_memory(void) {
	// Try triggering a memory sanitizer.
	void* b33 = hxmalloc_ext(33, hxsystem_allocator_temporary_stack, 16);

	char* t = hxstring_duplicate("_est", hxsystem_allocator_temporary_stack);
	t[0] = 't';

	void* b32 = hxmalloc(32);

	memset(b33, 0xde, 33);
	memset(b32, 0xde, 32);

	hxfree(b33);
	hxfree(b32);

	const bool result = strcmp("test", t) == 0;
	hxfree(t);
	return result;
}

// C Test dispatcher. Uses preprocessor string concatenation and sizeof for strlen.
#define HX_CTEST_PRINT(x) fwrite(x, (sizeof x) - 1, 1, stderr)
#define HX_CTEST_EXEC(fn) (fn() || (HX_CTEST_PRINT("ASSERT_FAIL test_fail " #fn "\n"), false))

bool hxctest_all(void) {
	return HX_CTEST_EXEC(hxctest_hatchling_h)
	    && HX_CTEST_EXEC(hxctest_math)
		&& HX_CTEST_EXEC(hxctest_clamp)
		&& HX_CTEST_EXEC(hxctest_swap)
		&& HX_CTEST_EXEC(hxctest_memory);
}
