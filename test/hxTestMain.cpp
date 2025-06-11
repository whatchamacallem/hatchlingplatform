// Copyright 2017-2025 Adrian Johnston

// Include everything first to catch conflicts.
#include <hx/hatchling.h>
#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxTest.hpp> // May include Google Test.

#include <stdio.h>

#include "hxCTest.h"

HX_REGISTER_FILENAME_HASH

// Run all the C tests.
TEST(hxCTest, AllTests) {
	ASSERT_TRUE(hxCTestAll());
}

// These two tests test the test framework by failing.
#if (HX_TEST_ERROR_HANDLING)
TEST(hxDeathTest, Fail) {
	hxLog("TEST_EXPECTING_ASSERTS:\n");
	SUCCEED();
	for (int i = 10; i--;) {
		FAIL() << "this message is intentionally blank.\n";
	}
}
TEST(hxDeathTest, NothingAsserted) {
	hxLog("TEST_EXPECTING_ASSERT:\n");
}
#endif

bool hxRunAllTests(void) {
	::testing::InitGoogleTest();

	hxLogConsole("hatchling platform 🐉🐉🐉 " HATCHLING_TAG "\n");
	hxLogConsole("release: %d profile: %d " __DATE__ " " __TIME__ "\n",
		(int)(HX_RELEASE), (int)(HX_PROFILE));
	hxWarnMsg(HX_HATCHLING_PCH_USED, "note pch not used");

	// RUN_ALL_TESTS is a Google Test symbol.
	size_t testsFailing = (size_t)RUN_ALL_TESTS();

#if (HX_TEST_ERROR_HANDLING)
	hxAssertRelease(testsFailing == 2, "expected 2 tests to fail");
	// there are no asserts at level 3.
	hxLogHandler(hxLogLevel_Warning, "expected 2 tests to fail");
	return testsFailing == 2;
#else
	return testsFailing == 0;
#endif
}

hxConsoleCommandNamed(hxRunAllTests, runtests);

// hxtest - Command line console command dispatcher. Each parameter is treated
// as a separate command.
int main(int argc, char**argv) {
	hxInit();

	if(argc != 0) {
		for(int i=1; i<argc; ++i) {
			hxConsoleExecLine(argv[i]);
		}
	}
	else {
		hxLogWarning("usage: hxtest <command>...");
		hxLogWarning("try: hxtest help");
	}

	// Logging and asserts are actually unaffected by a shutdown.
#if (HX_RELEASE) < 3
	hxShutdown();
#endif
}
