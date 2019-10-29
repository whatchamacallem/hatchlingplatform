// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxConsole.h>
#include <hx/hxTest.h> // May include Google Test.

HX_REGISTER_FILENAME_HASH

#if (HX_RELEASE) < 1 && HX_TEST_DIE_AT_THE_END
TEST(hxDeathTest, Fail) {
	hxLog("TEST_EXPECTING_ASSERT:\n");
	SUCCEED();
	FAIL();
}
TEST(hxDeathTest, NothingAsserted) {
	hxLog("TEST_EXPECTING_ASSERT:\n");
}
#endif

int32_t hxTestMain() {
	hxInit();

	const char bytes[] = "The quick brown fox jumps over the lazy dog....";
	hxHexDump(bytes, 48, 1);

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float));

	hxConsoleHelp();

	// RUN_ALL_TESTS is a Google Test symbol.
	int32_t testsFailing = RUN_ALL_TESTS();

#if (HX_RELEASE) < 3
	hxShutdown();
#endif
	return testsFailing;
}

extern "C"
int main() {
	::testing::InitGoogleTest();

	int32_t testsFailing = hxTestMain();

#if (HX_RELEASE) < 1 && HX_TEST_DIE_AT_THE_END
	// Will EXIT_SUCCESS.
	hxAssertMsg(testsFailing == 2, "expected 2 tests to fail");
	g_hxSettings.deathTest = 1;
	hxAssertMsg(0, "HX_TEST_DIE_AT_THE_END");
#endif
	return testsFailing == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
