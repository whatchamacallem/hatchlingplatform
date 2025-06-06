// Copyright 2017-2025 Adrian Johnston


#include <stdio.h>

#include <hx/hatchling.h>

#include "hxCTest.h"

// ----------------------------------------------------------------------------
// C utils

bool hxCTestMath(void) {
	return hxmin(-3, 2) == -3
		&& hxmax(-3, 2) == 2
		&& hxmin(3u, 2u) == 2u
		&& hxmax(3u, 2u) == 3u
		&& hxabs(-2) == 2
		&& hxabs(2u) == 2u;
}

bool hxCTestClamp(void) {
	return hxclamp(0, 1, 5) == 1
		&& hxclamp(1, 1, 5) == 1
		&& hxclamp(5, 1, 5) == 5
		&& hxclamp(6, 1, 5) == 5;
}

bool hxCTestSwap(void) {
	char a[] = { 3, 7 };
	hxswap(a[0], a[1]);

	struct { unsigned int x; short pad; } b[] = { { 30u, -1 }, { 70u, -2 } };
	hxswap(b[0], b[1]);

	return a[0] == 7 && a[1] == 3 && b[0].x == 70u && b[1].x == 30u;
}

bool hxCTestMemory(void) {
	// try triggering a memory sanitizer
	void* b33 = hxMallocExt(33, hxMemoryAllocator_TemporaryStack, 16);

	char* t = hxStringDuplicate("_est", hxMemoryAllocator_TemporaryStack);
	t[0] = 't';

	void* b32 = hxMalloc(32);

	memset(b33, 0xee, 33);
	memset(b32, 0xee, 32);

	hxFree(b33);
	hxFree(b32);

	bool result = strcmp("test", t) == 0;
	hxFree(t);
	return result;
}

#define HX_CTEST_PRINT(x) fwrite(x, (sizeof x) - 1, 1, stdout)
#define HX_CTEST_EXEC(fn) (fn() || (HX_CTEST_PRINT(#fn ": test fail\n"), false))

bool hxCTestAll(void) {
	return HX_CTEST_EXEC(hxCTestMath)
		&& HX_CTEST_EXEC(hxCTestClamp)
		&& HX_CTEST_EXEC(hxCTestSwap)
		&& HX_CTEST_EXEC(hxCTestMemory);
}
