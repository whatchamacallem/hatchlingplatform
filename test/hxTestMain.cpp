// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxTest.h> // May include Google Test.

// Include everything to catch conflicts.
#include <hx/hxAllocator.h>
#include <hx/hxArray.h>
#include <hx/hxConsole.h>
#include <hx/hxDma.h>
#include <hx/hxFile.h>
#include <hx/hxHashTable.h>
#include <hx/hxHashTableNodes.h>
#include <hx/hxProfiler.h>
#include <hx/hxSort.h>
#include <hx/hxTaskQueue.h>

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
	::testing::InitGoogleTest();

	size_t testsFailing = hxTestMain();

#if (HX_TEST_DIE_AT_THE_END)
	hxAssertRelease(testsFailing == 2, "expected 2 tests to fail");
	g_hxSettings.deathTest = 1;
	hxExit("Expected failures correct.\n");
#endif

	return (testsFailing == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
