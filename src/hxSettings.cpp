// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxConsole.hpp>

HX_REGISTER_FILENAME_HASH

// hxSettings - g_hxSettings is declared in hxCUtils.c for maximum portability.

namespace {
#if (HX_RELEASE) < 1
// Confirm the correct number of asserts were triggered.
static bool checkasserts(void) {
	int unusedAsserts = g_hxSettings.assertsToBeSkipped;
	g_hxSettings.assertsToBeSkipped = 0;
	hxAssertMsg(unusedAsserts == 0, "expected more asserts");
	return unusedAsserts == 0;
}

hxConsoleCommand(checkasserts);
hxConsoleVariableNamed(g_hxSettings.assertsToBeSkipped, skipasserts);
#endif

hxConsoleVariableNamed(g_hxSettings.logLevel, loglevel);
} // namespace

void hxSettingsConstruct() {
	g_hxSettings.logLevel = hxLogLevel_Log;
	g_hxSettings.deallocatePermanent = false;

#if (HX_RELEASE) < 1
	g_hxSettings.assertsToBeSkipped = 0;
#endif
}
