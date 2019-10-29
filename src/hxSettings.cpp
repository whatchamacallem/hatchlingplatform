// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxConsole.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxSettings

extern "C" {
hxSettings g_hxSettings;
}

hxConsoleVariableNamed(g_hxSettings.logLevel, loglevel);

#if (HX_RELEASE) < 1
hxConsoleVariableNamed(g_hxSettings.assertsToBeSkipped, skipasserts);
hxConsoleVariableNamed(g_hxSettings.lightEmittingDiode, lightEmittingDiode);
#endif

void hxSettingsConstruct() {
	g_hxSettings.logLevel = hxLogLevel_Log;
	g_hxSettings.logFile = ((HX_RELEASE) < 1) ? "log.txt" : hxnull;
	g_hxSettings.isShuttingDown = false;

#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = false;
#endif
#if (HX_RELEASE) < 1
	g_hxSettings.assertsToBeSkipped = 0;
	g_hxSettings.deathTest = 0;
	g_hxSettings.lightEmittingDiode = 0.7f;
#endif
}
