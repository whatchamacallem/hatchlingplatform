// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxArray.h>
#include <hx/hxConsole.h>
#include <hx/hxDma.h>
#include <hx/hxFile.h>
#include <hx/hxHashTableNodes.h>
#include <hx/hxProfiler.h>
#include <hx/hxSort.h>

#if HX_USE_STDIO_H
#include <stdio.h>
#endif

// ----------------------------------------------------------------------------
// Implements HX_REGISTER_FILENAME_HASH.  See hxStringLiteralHash.h.

#if (HX_RELEASE) < 1
HX_REGISTER_FILENAME_HASH
typedef hxHashTable<hxHashTableNodeStringLiteral, 5> hxHashStringLiteral;

struct hxFilenameLess {
	HX_INLINE bool operator()(const char*& lhs, const char*& rhs) const {
		return hxStringLiteralHashDebug(lhs) < hxStringLiteralHashDebug(rhs);
	}
};

hxHashStringLiteral& hxStringLiteralHashes() {
	static hxHashStringLiteral s_hxStringLiteralHashes;
	return s_hxStringLiteralHashes;
}
hxRegisterFileConstructor::hxRegisterFileConstructor(const char* s) {
	hxInit();
	hxStringLiteralHashes().insert_unique(s, hxMemoryManagerId_Heap);
}

static void hxPrintFileHashes() {
	hxLog("filenames in hash order:\n");

	typedef hxArray<const char*> Filenames;
	Filenames filenames;
	filenames.reserve(hxStringLiteralHashes().size());

	hxHashStringLiteral::iterator it = hxStringLiteralHashes().begin();
	hxHashStringLiteral::const_iterator end = hxStringLiteralHashes().cend();
	for (; it != end; ++it) {
		filenames.push_back(it->key);
	}

	hxInsertionSort(filenames.begin(), filenames.end(), hxFilenameLess());

	for (Filenames::iterator f = filenames.begin(); f != filenames.end(); ++f) {
		hxLog("  %08x %s\n", hxStringLiteralHashDebug(*f), *f);
	}

	hxStringLiteralHashes().clear();
}
#else
#define hxPrintFileHashes() ((void)0)
#endif

// ----------------------------------------------------------------------------
// init, shutdown, exit, assert and logging.

void hxSettingsConstruct();

static const char* s_hxInitFile = ""; // For trapping code running before hxMain.
static size_t s_hxInitLine = 0;

#if HX_USE_CPP11_TIME
std::chrono::high_resolution_clock::time_point g_hxTimeStart;
#endif

extern "C"
void hxInitAt(const char* file, size_t line) {
	hxAssertRelease(!g_hxIsInit, "internal error");
	g_hxIsInit = 1;

	if (file) { s_hxInitFile = file; }
	s_hxInitLine = line;

#if HX_USE_CPP11_TIME
	g_hxTimeStart = std::chrono::high_resolution_clock::now();
#endif

	hxSettingsConstruct();
	hxMemoryManagerInit();
	hxDmaInit();

#if defined(__cpp_exceptions)
	hxout << "WARNING disable exceptions\n"; // Exceptions add unnecessary overhead.
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
	hxAssert(g_hxIsInit);
	hxPrintFileHashes();
	hxProfilerStop();
	hxDmaShutDown();

	g_hxSettings.isShuttingDown = true;
	hxConsoleDeregisterAll(); // Free console allocations.
	hxMemoryManagerShutDown();
	hxCloseOut(); // Assert messages will still be written to stdout.
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
}

extern "C"
void hxExit(const char* format, ...) {
	hxFile& f = hxout;
	if (f.is_open() || f.is_echo()) {
		f << "EXIT ";

		char buf[HX_MAX_LINE] = "\n";
		if (format != hxnull) {
			va_list args;
			va_start(args, format);
			vsnprintf(buf, HX_MAX_LINE, format, args);
			va_end(args);
		}

		f << buf;
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
int hxAssertHandler(const char* file, size_t line) {
	const char* f = hxBasename(file);
	if (!g_hxIsInit || g_hxSettings.assertsToBeSkipped-- > 0) {
		hxLogHandler(hxLogLevel_Assert, "(skipped) %s(%u) hash %08x", f, (unsigned int)line,
			(unsigned int)hxStringLiteralHashDebug(file));
		return 1;
	}
	hxLogHandler(hxLogLevel_Assert, "%s(%u) hash %08x\n", f, (unsigned int)line,
		(unsigned int)hxStringLiteralHashDebug(file));

	// return to HX_DEBUG_BREAK at calling line.
	return 0;
}
#else
void hxAssertHandler(uint32_t file, size_t line) {
	hxExit("ASSERT_FAIL %08x(%u)\n", (unsigned int)file, (unsigned int)line);
}
#endif

// ----------------------------------------------------------------------------
// hxLogHandlerV

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	hxFile& f = hxout;

	if (g_hxIsInit && level < g_hxSettings.logLevel) {
		return;
	}

	if (f.is_open() || f.is_echo()) {
		char buf[HX_MAX_LINE+1];
		int sz = format ? vsnprintf(buf, HX_MAX_LINE, format, args) : -1;
		hxAssertMsg(sz >= 0, "format error: %s", format ? format : "(null)");
		if (sz <= 0) {
			return;
		}
		sz = hxmin(sz, HX_MAX_LINE);
		if (level == hxLogLevel_Warning || level == hxLogLevel_Assert) {
			buf[sz++] = '\n';
		}

		if (level == hxLogLevel_Warning) {
			f << "WARNING ";
		}
		else if (level == hxLogLevel_Assert) {
			f << "ASSERT_FAIL ";
		}
		f.write(buf, sz);
	}
}

#else // HX_RELEASE == 3
extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {

	if (!format || (g_hxIsInit && level < g_hxSettings.logLevel)) {
		return;
	}
	char buf[HX_MAX_LINE+1];
	int sz = vsnprintf(buf, HX_MAX_LINE, format, args);
	if (sz > 0) {
		sz = hxmin(sz, HX_MAX_LINE);
		if (level == hxLogLevel_Warning || level == hxLogLevel_Assert) {
			buf[sz++] = '\n';
		}

		HX_STATIC_ASSERT(HX_USE_STDIO_H, "TODO: Logging I/O");
		::fwrite(buf, 1, sz, stdout);
	}
}
#endif
