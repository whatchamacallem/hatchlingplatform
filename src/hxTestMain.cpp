// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxConsole.h>
#include <hx/hxTest.h> // May include Google Test.

HX_REGISTER_FILENAME_HASH

bool hxTestMain() {
	hxInit();

	const char bytes[] = "The quick brown fox jumps over the lazy dog....";
	hxHexDump(bytes, 48, 1);

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float));

	hxConsoleHelp();

	// RUN_ALL_TESTS is a Google Test symbol.
	bool testsOk = RUN_ALL_TESTS();

#if (HX_RELEASE) < 3
	hxShutdown();
#endif
	return testsOk;
}

extern "C"
int main() {
	::testing::InitGoogleTest();

	bool testsOk = hxTestMain();
	return testsOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
