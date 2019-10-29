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

static const char* s_hxInitFile = ""; // For trapping code running before hxMain.
static uint32_t s_hxInitLine = 0;

#if (HX_RELEASE) < 1
static hxHashTable<hxHashTableNodeStaticString, 7> s_hxStringLiteralHashes;
hxRegisterFileConstructor::hxRegisterFileConstructor(const char* s) { s_hxStringLiteralHashes.insert_unique(s, hxMemoryManagerId_Heap); }
HX_REGISTER_FILENAME_HASH;
#endif

extern "C"
void hxInitAt(const char* file, uint32_t line) {
	hxAssertRelease(!g_hxIsInit, "internal error");
	g_hxIsInit = 1;

	if (file) { s_hxInitFile = file; }
	s_hxInitLine = line;

	g_hxSettings.construct();

	hxLog("Hatchling Platform\n");
	hxConsolePrint("Release %d Profile %d Flags %d %d %d Build: " __DATE__ " " __TIME__ "\n",
		HX_RELEASE, HX_PROFILE, HX_HAS_CPP11_THREADS, HX_HAS_CPP11_TIME, HX_HAS_CPP14_CONSTEXPR);

	hxMemoryManagerInit();
	hxDmaInit();

#ifdef __cpp_rtti
	hxWarn("RTTI is enabled"); // RTTI is not advised.
#endif
#ifdef __cpp_exceptions
	hxWarn("Exceptions are enabled"); // Exceptions are not used.
#endif
#if !HX_HAS_CPP11_THREADS
	hxWarn("Single threaded only"); // Audit HX_HAS_CPP11_THREADS if you need custom threading.
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
// Wrapped to ensure correct construction order.
static hxFile& hxLogFile() {
	static hxFile f(hxFile::out | hxFile::fallible, "%s", g_hxSettings.logFile);
	return f;
}

extern "C"
void hxShutdown() {
	hxAssertRelease(g_hxIsInit, "hxShutdown unexpected\n");
	hxLogRelease("hxShutdown...\n");

#if (HX_RELEASE) < 1
	hxLog("filename hash codes:\n");
	for (hxHashTable<hxHashTableNodeStaticString, 7>::iterator it = s_hxStringLiteralHashes.begin();
			it != s_hxStringLiteralHashes.end(); ++it) {
		hxLog("  %08x %s\n", hxHashStringLiteralDebug(it->key), it->key);
	}
	s_hxStringLiteralHashes.clear();
#endif

	hxProfilerShutdown();
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
	if (format != null) {
		va_list args;
		va_start(args, format);
		sz = ::vsnprintf(buf, HX_MAX_LINE, format, args);
		va_end(args);
	}

#if HX_HAS_C_FILE
	if (sz < 0) {
		::fwrite("exit format error\n", 1, sizeof "exit format error\n" - 1, stdout);
	}
	else {
		::fwrite(buf, 1, (size_t)hxMin(sz, HX_MAX_LINE), stdout);
	}
	::fflush(stdout); 
#endif

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
		hxLogHandler(hxLogLevel_Assert, "(skipped) %s(%u) hash %08x\n", f, (unsigned int)line, (unsigned int)hxHashStringLiteralDebug(file));
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
	if (level < g_hxSettings.logLevelConsole && level < g_hxSettings.logLevelFile) {
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

#if HX_HAS_C_FILE
#define HX_FWRITE_STR(x) ::fwrite(x, 1, sizeof x - 1, stdout);
	if (level >= g_hxSettings.logLevelConsole) {
		if (level == hxLogLevel_Warning) {
			HX_FWRITE_STR("WARNING: ");
		}
		else if (level == hxLogLevel_Assert) {
			HX_FWRITE_STR("ASSERT_FAIL: ");
		}
		::fwrite(buf, 1, sz, stdout);
	}
#undef HX_FWRITE_STR
#endif

	if (level >= g_hxSettings.logLevelFile) {
		hxFile& f = hxLogFile();
		if (f.is_open()) {
			if (level == hxLogLevel_Warning) {
				f.print("WARNING: ");
			}
			else if (level == hxLogLevel_Assert) {
				f.print("ASSERT_FAIL: ");
			}

			f.write(buf, sz);
		}
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

