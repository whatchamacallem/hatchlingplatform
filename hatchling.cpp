// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxMemoryManager.h"
#include "hxDma.h"
#include "hxProfiler.h"
#include "hxFile.h"
#include "hxConsole.h"
#include "hxHashTableNodes.h"

#include <stdio.h>

// ----------------------------------------------------------------------------

void hxSettingsConstruct();

static const char* s_hxInitFile = ""; // For trapping code running before hxMain.
static uint32_t s_hxInitLine = 0;

#if (HX_RELEASE) < 1
static hxHashTable<hxHashTableNodeStaticString, 7> s_hxStringLiteralHashes;
hxRegisterFileConstructor::hxRegisterFileConstructor(const char* s) { s_hxStringLiteralHashes.insert_unique(s, hxMemoryManagerId_Heap); }
HX_REGISTER_FILENAME_HASH
#endif

extern "C"
void hxInitAt(const char* file, uint32_t line) {
	hxAssertRelease(!g_hxIsInit, "internal error");
	g_hxIsInit = 1;

	if (file) { s_hxInitFile = file; }
	s_hxInitLine = line;

	hxSettingsConstruct();

	hxLogConsole("Hatchling Platform\n");
	hxLogConsole("Release %d Profile %d Flags %d%d%d Build: " __DATE__ " " __TIME__ "\n",
		(int)(HX_RELEASE), (int)(HX_PROFILE), (int)(HX_HAS_CPP11_THREADS), (int)(HX_HAS_CPP11_TIME), (int)(HX_HAS_CPP14_CONSTEXPR));

	hxMemoryManagerInit();
	hxDmaInit();

#ifdef __cpp_rtti
	hxout << "WARNING: RTTI is enabled\n"; // RTTI is not advised.
#endif
#ifdef __cpp_exceptions
	hxout << "WARNING: Exceptions are enabled\n"; // Exceptions are not used.
#endif
#if !HX_HAS_CPP11_THREADS
	hxout << "WARNING: Single threaded only\n"; // Audit HX_HAS_CPP11_THREADS if you need custom threading.
#endif
}

extern "C"
void hxLogHandler(enum hxLogLevel level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	hxLogHandlerV(level, format, args);
	va_end(args);
}

// ----------------------------------------------------------------------------
// HX_RELEASE < 3 facilities 

#if (HX_RELEASE) < 3

extern "C"
void hxShutdown() {
	hxAssertRelease(g_hxIsInit, "hxShutdown unexpected\n");
	hxLog("hxShutdown...\n");

#if (HX_RELEASE) < 1
	hxLog("filename hash codes:\n");
	for (hxHashTable<hxHashTableNodeStaticString, 7>::iterator it = s_hxStringLiteralHashes.begin();
			it != s_hxStringLiteralHashes.end(); ++it) {
		hxLog("  %08x %s\n", hxHashStringLiteralDebug(it->key), it->key);
	}
	s_hxStringLiteralHashes.clear();
#endif

	hxProfilerStop();
	hxDmaShutDown();

	g_hxSettings.isShuttingDown = true;
	hxConsoleDeregisterAll(); // Free console allocations.
	hxMemoryManagerShutDown();
	hxLogFile().close();
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
}

extern "C"
void hxExit(const char* format, ...) {
	char buf[HX_MAX_LINE];
	int sz = -1;
	if (format != hxnull) {
		va_list args;
		va_start(args, format);
		sz = ::vsnprintf(buf, HX_MAX_LINE, format, args);
		va_end(args);
	}

	hxFile& f = hxLogFile();
	if (f.is_open()) {
		f.print("%s", (sz < 0) ? "exit format error\n" : buf);
		f.close();
	}

	// Stop here before the callstack gets lost inside _Exit.  This is not for
	// normal termination on an embedded target.
	HX_DEBUG_BREAK;
	::_Exit(EXIT_FAILURE);
}

extern "C"
#if (HX_RELEASE) < 1
void hxAssertHandler(const char* file, uint32_t line) {
	hxInit();
	const char* f = hxBasename(file);
	if (g_hxSettings.assertsToBeSkipped-- > 0) {
		hxLogHandler(hxLogLevel_Assert, "(skipped) %s(%u) hash %08x", f, (unsigned int)line, (unsigned int)hxHashStringLiteralDebug(file));
		return;
	}
	hxExit("ASSERT_FAIL: %s(%u) hash %08x\n", f, (unsigned int)line, (unsigned int)hxHashStringLiteralDebug(file));
}
#else
void hxAssertHandler(uint32_t file, uint32_t line) {
	hxExit("ASSERT_FAIL: %08x(%u)\n", (unsigned int)file, (unsigned int)line);
}
#endif

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	hxInit();
	if (level < g_hxSettings.logLevel) {
		return;
	}

	char buf[HX_MAX_LINE+1];
	int sz = format ? ::vsnprintf(buf, HX_MAX_LINE, format, args) : 0;
	hxAssertMsg(sz >= 0, "format error: %s", format ? format : "<null>");
	if (sz <= 0) {
		return;
	}
	sz = hxMin(sz, HX_MAX_LINE);
	if (level == hxLogLevel_Warning || level == hxLogLevel_Assert) {
		buf[sz++] = '\n';
	}

	if (level >= g_hxSettings.logLevel) {
		hxFile& f = hxLogFile();
		if (f.is_open()) {
			if (level == hxLogLevel_Warning) {
				f << "WARNING: ";
			}
			else if (level == hxLogLevel_Assert) {
				f << "ASSERT_FAIL: ";
			}

			f.write(buf, sz);
		}
#if HX_HAS_C_FILE
		else if (level >= hxLogLevel_Console) {
			::fwrite(buf, sz, 1, stdout);
		}
#endif
	}
}

#else
// ----------------------------------------------------------------------------
// HX_RELEASE == 3 facilities

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
#if HX_HAS_C_FILE
	::vfprintf(stdout, format, args);
#else // !HX_HAS_C_FILE
#error "TODO: file I/O"
#endif
}

#endif
// ----------------------------------------------------------------------------

