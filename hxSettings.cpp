// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxTest.h"
#include "hxConsole.h"

HX_REGISTER_FILENAME_HASH

hxSettings g_hxSettings;

hxConsoleVariableNamed(g_hxSettings.logLevel, logLevel);

#if (HX_RELEASE) < 1
hxConsoleVariableNamed(g_hxSettings.assertsToBeSkipped, assertsToBeSkipped);
#endif

void hxSettingsConstruct() {
	g_hxSettings.logLevel = hxLogLevel_Log;
	g_hxSettings.logFile = ((HX_RELEASE) < 1) ? "log.txt" : hxnull;
	g_hxSettings.isShuttingDown = false;
#if (HX_RELEASE) < 1
	g_hxSettings.assertsToBeSkipped = 0;
#endif
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = false;
#endif
}
