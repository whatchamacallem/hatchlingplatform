// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxConsole.h>
#include "hxTest.h"

HX_REGISTER_FILENAME_HASH

#if !(HX_GOOGLE_TEST)

// Ensures constructor runs before tests are registered by global constructors.
hxTestRunner& hxTestRunner::get() {
	static hxTestRunner s_hxTestRunner;
	return s_hxTestRunner;
}

bool hxTestMain() {
	hxInit();

	const char bytes[] = "The quick brown fox jumps over the lazy dog....";
	hxHexDump(bytes, 48, 1);

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float));

	hxConsoleHelp();

	// Filter using e.g. hxTestRunner::get().setFilterStaticString("hxArrayTest").
	// All tests already registered by global constructors.
	bool testsOk = hxTestRunner::get().executeAllTests();

#if (HX_RELEASE) < 3
	hxShutdown();
#endif
	return testsOk;
}

extern "C"
int main() {
	bool testsOk = hxTestMain();
	return testsOk ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif // !HX_GOOGLE_TEST
