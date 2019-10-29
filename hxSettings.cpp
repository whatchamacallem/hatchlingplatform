// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxTest.h"
#include "hxConsole.h"

HX_REGISTER_FILENAME_HASH;

hxSettings g_hxSettings;

hxConsoleVariableNamed(g_hxSettings.logLevelConsole, logLevelConsole);
hxConsoleVariableNamed(g_hxSettings.logLevelFile, logLevelFile);

#if (HX_RELEASE) < 1
hxConsoleVariableNamed(g_hxSettings.assertsToBeSkipped, assertsToBeSkipped);
#endif

void hxSettings::construct() {
	settingsIntegrityCheck = c_settingsIntegrityCheck;

	logLevelConsole = hxLogLevel_Log;
	logLevelFile = hxLogLevel_Log;
	logFile = "hx_log.txt"; // set to hx_null here to disable
	isShuttingDown = false;
#if (HX_RELEASE) < 1
	assertsToBeSkipped = 0;
#endif
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	disableMemoryManager = false;
#endif
}
