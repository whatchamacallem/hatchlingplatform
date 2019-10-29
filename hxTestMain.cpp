// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxTest.h"
#include "hxConsole.h"

// HX_HOST may be configured to use GoogleTest instead.
#if !(HX_GOOGLE_TEST)

// Ensures constructor runs before tests are registered by global constructors.
hxTestRunner& hxTestRunner::get() {
	static hxTestRunner s_hxTestRunner;
	return s_hxTestRunner;
}

void hxTestMain() {
	hxInit();
	// g_hxSettings may have a compiler generated default constructor clearing it to
	// zero.  Or something much much worse is going on.
	hxAssertRelease(g_hxSettings.settingsIntegrityCheck == hxSettings::c_settingsIntegrityCheck, "g_hxSettings overwritten");

	const char bytes[] = "hatchling platform";
	hxHexDump(bytes, sizeof bytes, "\"hatchling platform\"");

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float), "{ 0, 1, 2 }");

	hxConsoleHelp();

	// Filter using e.g. hxTestRunner::get().setFilterStaticString("hxArrayTest").
	// All tests already registered by global constructors.
	hxTestRunner::get().executeAllTests();

	// Allow freeing up allocations marked as permanent.
	hxShutdown();
}

extern "C"
int main() {
	hxTestMain();
	return 0;
}

#endif // !HX_GOOGLE_TEST
