// Copyright 2017-2025 Adrian Johnston

// Include everything to catch conflicts.
#include <hx/hatchling_pch.hpp>

// Include standard headers last as a test.
#include <stdio.h>

HX_REGISTER_FILENAME_HASH

#if (HX_TEST_DIE_AT_THE_END)
TEST(hxDeathTest, Fail) {
	hxLog("TEST_EXPECTING_ASSERTS:\n");
	SUCCEED();
	for (int i = 10; i--;) {
		FAIL() << "this message is logged on repeated assert failures.\n";
	}
}
TEST(hxDeathTest, NothingAsserted) {
	hxLog("TEST_EXPECTING_ASSERT:\n");
}
#endif

size_t hxTestMain() {
	hxInit();

	hxLogConsole("hatchling platform " HATCHLING_TAG "\n");
	hxLogConsole("release %d profile %d flags %d%d%d%d build: " __DATE__ " " __TIME__ "\n",
		(int)(HX_RELEASE), (int)(HX_PROFILE), (int)(HX_USE_CPP11_THREADS),
		(int)(HX_USE_CPP11_TIME), (int)(HX_USE_CPP14_CONSTEXPR), (int)(HX_MEM_DIAGNOSTIC_LEVEL));
	hxLogConsole("size_t bytes %d\n", (int)sizeof(size_t));

	char bytes[48] = { };
	snprintf(bytes, 48, "%s", "The quick brown fox jumps over the lazy dog....");
	hxHexDump(bytes, 48, 1);

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float));

	hxConsoleHelp();

	// RUN_ALL_TESTS is a Google Test symbol.
	size_t testsFailing = (size_t)RUN_ALL_TESTS();

#if (HX_RELEASE) < 3
	hxShutdown();
#endif
	return testsFailing;
}

int main() {
	hxPrintFileHashes();

	::testing::InitGoogleTest();

	size_t testsFailing = hxTestMain();

#if (HX_TEST_DIE_AT_THE_END)
	hxAssertRelease(testsFailing == 2, "expected 2 tests to fail");
#if (HX_RELEASE) < 3
	g_hxSettings.deathTest = 1;
	hxExit("Expected failures correct.\n");
#else
	// there are no asserts at level 4.
	return (testsFailing == 2) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
#else
	return (testsFailing == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
