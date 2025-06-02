// Copyright 2017-2025 Adrian Johnston

// Include everything first to catch conflicts.
#include <hx/hatchling.h>
#include <hx/hxAllocator.hpp>
#include <hx/hxArray.hpp>
#include <hx/hxConsole.hpp>
#include <hx/hxDma.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxHashTableNodes.hpp>
#include <hx/hxMemoryManager.h>
#include <hx/hxProfiler.hpp>
#include <hx/hxSort.hpp>
#include <hx/hxStringLiteralHash.h>
#include <hx/hxTask.hpp>
#include <hx/hxTaskQueue.hpp>
#include <hx/hxTest.hpp> // May include Google Test.
#include <hx/hxTime.hpp>

#include <stdio.h>

#include "hxCTest.h"

HX_REGISTER_FILENAME_HASH

// Run all the C tests.
TEST(hxCTest, AllTests) {
	ASSERT_TRUE(hxCTestAll());
}

// These two tests test the test framework by failing.
#if (HX_TEST_DIE_AT_THE_END)
TEST(hxDeathTest, Fail) {
	hxLog("TEST_EXPECTING_ASSERTS:\n");
	SUCCEED();
	for (int i = 10; i--;) {
		FAIL() << "this message is omitted.\n";
	}
}
TEST(hxDeathTest, NothingAsserted) {
	hxLog("TEST_EXPECTING_ASSERT:\n");
}
#endif

size_t hxTestMain() {
	hxInit();

	hxLogConsole("hatchling platform " HATCHLING_TAG "\n");
	hxLogConsole("release %d profile %d flags %d%d%d build: " __DATE__ " " __TIME__ "\n",
		(int)(HX_RELEASE), (int)(HX_PROFILE), (int)(HX_USE_CPP_THREADS),
		(int)(HX_USE_CHRONO), (int)(HX_MEM_DIAGNOSTIC_LEVEL));
	hxLogConsole("sizeof(size_t)=%d, ", (int)sizeof(size_t));
	hxLogConsole("PCH used=%d, emoji=🐉🐉🐉\n", (int)HX_HATCHLING_PCH_USED);

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
	// there are no asserts at level 3.
	hxLogHandler(hxLogLevel_Warning, "expected 2 tests to fail");
	return (testsFailing == 2) ? EXIT_SUCCESS : EXIT_FAILURE;
#else
	return (testsFailing == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
