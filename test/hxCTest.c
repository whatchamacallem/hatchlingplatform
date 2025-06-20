// Copyright 2017-2025 Adrian Johnston


#include <stdio.h>

#include <hx/hatchling.h>

#include "hxcTest.h"

// C utils

bool hxcTest_math(void) {
	return hxmin(-3, 2) == -3
		&& hxmax(-3, 2) == 2
		&& hxmin(3u, 2u) == 2u
		&& hxmax(3u, 2u) == 3u
		&& hxabs(-2) == 2
		&& hxabs(2u) == 2u;
}

bool hxcTest_clamp(void) {
	return hxclamp(0, 1, 5) == 1
		&& hxclamp(1, 1, 5) == 1
		&& hxclamp(5, 1, 5) == 5
		&& hxclamp(6, 1, 5) == 5;
}

bool hxcTest_swap(void) {
	char a[] = { 3, 7 };
	hxswap(a[0], a[1]);

	struct { unsigned int x; short pad; } b[] = { { 30u, -1 }, { 70u, -2 } };
	hxswap(b[0], b[1]);

	return a[0] == 7 && a[1] == 3 && b[0].x == 70u && b[1].x == 30u;
}

bool hxcTest_memory(void) {
	// try triggering a memory sanitizer
	void* b33 = hxmalloc_ext(33, hxmemory_allocator_Temporary_stack, 16);

	char* t = hxstring_duplicate("_est", hxmemory_allocator_Temporary_stack);
	t[0] = 't';

	void* b32 = hxmalloc(32);

	memset(b33, 0xee, 33);
	memset(b32, 0xee, 32);

	hxfree(b33);
	hxfree(b32);

	bool result = strcmp("test", t) == 0;
	hxfree(t);
	return result;
}

// A test harness in 2 lines of C.
#define HX_CTEST_PRINT(x) fwrite(x, (sizeof x) - 1, 1, stdout)
#define HX_CTEST_EXEC(fn) (fn() || (HX_CTEST_PRINT(#fn ": test fail\n"), false))

bool hxcTest_all(void) {
	return HX_CTEST_EXEC(hxcTest_math)
		&& HX_CTEST_EXEC(hxcTest_clamp)
		&& HX_CTEST_EXEC(hxcTest_swap)
		&& HX_CTEST_EXEC(hxcTest_memory);
}
