// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxMemoryManager.h>
#include <hx/hxDma.h>
#include <hx/hxProfiler.h>
#include <hx/hxFile.h>
#include <hx/hxConsole.h>
#include <hx/hxHashTableNodes.h>

#include <stdio.h>

// ----------------------------------------------------------------------------
// HX_IS_DEBUGGER_PRESENT()

#if (HX_RELEASE) < 3
#ifdef _MSC_VER
#include <Windows.h>

#define HX_IS_DEBUGGER_PRESENT IsDebuggerPresent
#elif __linux__ && !(HX_TEST_DIE_AT_THE_END)
#include <sys/ptrace.h>

static bool HX_IS_DEBUGGER_PRESENT() {
	static bool val = ::ptrace(PTRACE_TRACEME, 0, 1, 0) == -1;
	return val;
}
#else
#define HX_IS_DEBUGGER_PRESENT() false
#endif
#endif

// ----------------------------------------------------------------------------
// HX_REGISTER_FILENAME_HASH

#if (HX_RELEASE) < 1
HX_REGISTER_FILENAME_HASH

hxHashTable<hxHashTableNodeStringLiteral, 7>& hxStringLiteralHashes() {
	static hxHashTable<hxHashTableNodeStringLiteral, 7> s_hxStringLiteralHashes;
	return s_hxStringLiteralHashes;
}
hxRegisterFileConstructor::hxRegisterFileConstructor(const char* s) {
	hxStringLiteralHashes().insert_unique(s, hxMemoryManagerId_Heap);
}
#endif

// ----------------------------------------------------------------------------
// init, shutdown, exit, assert and logging.

void hxSettingsConstruct();

static const char* s_hxInitFile = ""; // For trapping code running before hxMain.
static uint32_t s_hxInitLine = 0;

extern "C"
void hxInitAt(const char* file, uint32_t line) {
	hxAssertRelease(!g_hxIsInit, "internal error");
	g_hxIsInit = 1;

	if (file) { s_hxInitFile = file; }
	s_hxInitLine = line;

	hxSettingsConstruct();
	hxMemoryManagerInit();
	hxDmaInit();

#if __cpp_exceptions
	hxout << "WARNING: disable exceptions\n"; // Exceptions add unnecessary overhead.
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
#if (HX_RELEASE) < 1
	hxLog("filename hash codes:\n");
	for (hxHashTable<hxHashTableNodeStringLiteral, 7>::iterator it = hxStringLiteralHashes().begin();
			it != hxStringLiteralHashes().end(); ++it) {
		hxLog("  %08x %s\n", hxStringLiteralHashDebug(it->key), it->key);
	}
	hxStringLiteralHashes().clear();
#endif

	hxProfilerStop();
	hxDmaShutDown();

	g_hxSettings.isShuttingDown = true;
	hxConsoleDeregisterAll(); // Free console allocations.
	hxMemoryManagerShutDown();
	hxout.close(); // will continue to echo to stdout.

#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
}

extern "C"
void hxExit(const char* format, ...) {
	char buf[HX_MAX_LINE] = "exit format\n";
	if (format != hxnull) {
		va_list args;
		va_start(args, format);
		::vsnprintf_(buf, HX_MAX_LINE, format, args);
		va_end(args);
	}

	hxFile& f = hxout;
	if (f.is_open() || f.is_echo()) {
		f << "EXIT: " << buf;
	}

	if (!(HX_TEST_DIE_AT_THE_END) && HX_IS_DEBUGGER_PRESENT()) {
		// Stop here before the call stack gets lost inside _Exit.  This is not for
		// normal termination on an embedded target.
		HX_DEBUG_BREAK;
	}

#if (HX_RELEASE) < 1
	// Code coverage runs at exit.  A death test will return EXIT_SUCCESS.
	::exit(g_hxSettings.deathTest ? EXIT_SUCCESS : EXIT_FAILURE);
#else
	// WARNING: All of the global C++ destructors may be registered with atexit()
	// which wastes a little memory.
	::_Exit(EXIT_FAILURE);
#endif
}

extern "C"
#if (HX_RELEASE) < 1
int hxAssertHandler(const char* file, uint32_t line) {
	hxInit();
	const char* f = hxBasename(file);
	if (g_hxSettings.assertsToBeSkipped-- > 0) {
		hxLogHandler(hxLogLevel_Assert, "(skipped) %s(%u) hash %08x", f, (unsigned int)line,
			(unsigned int)hxStringLiteralHashDebug(file));
		return 1;
	}
	hxLogHandler(hxLogLevel_Assert, "%s(%u) hash %08x\n", f, (unsigned int)line,
		(unsigned int)hxStringLiteralHashDebug(file));

	if ((HX_TEST_DIE_AT_THE_END) || !HX_IS_DEBUGGER_PRESENT()) {
		hxExit("no debugger\n");
	}

	// return to HX_DEBUG_BREAK at calling line.
	return 0;
}
#else
void hxAssertHandler(uint32_t file, uint32_t line) {
	hxExit("ASSERT_FAIL: %08x(%u)\n", (unsigned int)file, (unsigned int)line);
}
#endif

// ----------------------------------------------------------------------------
// hxLogHandlerV

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	hxInit();
	if (level < g_hxSettings.logLevel) {
		return;
	}

	char buf[HX_MAX_LINE+1];
	int sz = format ? ::vsnprintf_(buf, HX_MAX_LINE, format, args) : 0;
	hxAssertMsg(sz >= 0, "format error: %s", format ? format : "(null)");
	if (sz <= 0) {
		return;
	}
	sz = hxMin(sz, HX_MAX_LINE);
	if (level == hxLogLevel_Warning || level == hxLogLevel_Assert) {
		buf[sz++] = '\n';
	}

	if (level >= g_hxSettings.logLevel) {
		hxFile& f = hxout;
		if (f.is_open() || f.is_echo()) {
			if (level == hxLogLevel_Warning) {
				f << "WARNING: ";
			}
			else if (level == hxLogLevel_Assert) {
				f << "ASSERT_FAIL: ";
			}
			f.write(buf, sz);
		}
	}
}

#else // HX_RELEASE == 3 facilities

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	char buf[HX_MAX_LINE];
	int sz = format ? ::vsnprintf_(buf, HX_MAX_LINE, format, args) : 0;
	if (sz <= 0) {
		return;
	}	
	sz = hxMin(sz, HX_MAX_LINE);
#if HX_USE_C_FILE
	::fwrite(buf, 1, sz, stdout);
#else // !HX_USE_C_FILE
#error "TODO: file I/O"
#endif
}

#endif

