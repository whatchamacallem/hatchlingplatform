// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxArray.hpp>
#include <hx/hxConsole.hpp>
#include <hx/hxDma.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTableNodes.hpp>
#include <hx/hxProfiler.hpp>
#include <hx/hxSort.hpp>
#include <hx/hxSort.hpp>

#include <stdio.h>
#include <math.h>

HX_REGISTER_FILENAME_HASH

// HX_FLOATING_POINT_TRAPS traps (FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW) in debug
// so you can safely run without checks for them in release. Use -ffast-math in
// release or -DHX_FLOATING_POINT_TRAPS=0 to disable this debug facility. There
// is no C++ standard conforming way to disable floating point error checking.
// It is a gcc/clang extension. -fno-math-errno -fno-trapping-math will work if
// you require C++ conforming accuracy without the overhead of error checking.
// You need the math library -lm. Causing.. or implicit checking for in a
// release build.. or explicit checking for... floating point exceptions is not
// recommended.
#if (HX_CPLUSPLUS >= 201103L) && defined(__GLIBC__) && !defined(__FAST_MATH__)
#include <fenv.h>
#if !defined(HX_FLOATING_POINT_TRAPS)
#define HX_FLOATING_POINT_TRAPS 1
#endif
#else
#undef HX_FLOATING_POINT_TRAPS
#define HX_FLOATING_POINT_TRAPS 0
#endif

// New rule. Use -ffast-math in release. Or set -DHX_FLOATING_POINT_TRAPS=0.
HX_STATIC_ASSERT((HX_RELEASE) < 1 || !(HX_FLOATING_POINT_TRAPS),
	"floating point exceptions in release. use -ffast-math.");

// Use -fno-exceptions. Exceptions add overhead to c++ and add untested pathways.
// In this codebase memory allocation cannot fail. It is designed to force you
// to allocate enough memory for everything in advance. There are no exceptions
// to handle. There are exception handling intrinsics in use in case they are on.
#if defined(__cpp_exceptions) && !defined(__INTELLISENSE__)
HX_STATIC_ASSERT(0, "exceptions should not be enabled");
#endif

// ----------------------------------------------------------------------------
#if (HX_RELEASE) < 1
// Implements HX_REGISTER_FILENAME_HASH in debug. See hxStringLiteralHash.h.
namespace {
struct hxHashStringLiteral_ : public hxHashTable<hxRegisterStringLiteralHash, 5> {
	// the nodes are static global. do not free.
	~hxHashStringLiteral_(void) { releaseAll(); }
};

struct hxFilenameLess {
	inline bool operator()(const char* lhs, const char* rhs) const {
		return hxStringLiteralHashDebug(lhs) < hxStringLiteralHashDebug(rhs);
	}
};

hxHashStringLiteral_& hxStringLiteralHashes_() {
	static hxHashStringLiteral_ s_hxStringLiteralHashes_;
	return s_hxStringLiteralHashes_;
}

} // namespace {

// The key for the table is a string hash.
hxRegisterStringLiteralHash::hxRegisterStringLiteralHash(const char* str_)
        : m_hashNext_(0), m_hash_(hxStringLiteralHashDebug(str_)), m_str_(str_) {
    hxStringLiteralHashes_().insertNode(this);
}

// The hash table code expects to be able to hash a Key and compare it equal
// to the Node::hash value. That results in double hashing here. It is just
// another multiply.
uint32_t hxRegisterStringLiteralHash::hash() const {
	return hxKeyHash(m_hash_);
};

void hxPrintHashes(void) {
	hxInit();

	// sort by hash.
	hxLogConsole("string literals in hash order:\n");
	hxMemoryAllocatorScope temporaryStack(hxMemoryAllocator_TemporaryStack);

	typedef hxArray<const char*> Filenames;
	Filenames filenames; filenames.reserve(hxStringLiteralHashes_().size());

	hxHashStringLiteral_::constIterator it = hxStringLiteralHashes_().cBegin();
	hxHashStringLiteral_::constIterator end = hxStringLiteralHashes_().cEnd();
	for (; it != end; ++it) {
		filenames.pushBack(it->str());
	}

	hxInsertionSort(filenames.begin(), filenames.end(), hxFilenameLess());

	for (Filenames::iterator f = filenames.begin(); f != filenames.end(); ++f) {
		hxLog("  %08x %s\n", hxStringLiteralHashDebug(*f), *f);
	}
}

void hxCheckHash(hxconsolehex_t hash_) {
	hxRegisterStringLiteralHash* node = hxStringLiteralHashes_().find(hash_);
	if(node) {
		while(node) {
			hxLogConsole("%08x: %s\n", (unsigned int)hash_, node->str());
			node = hxStringLiteralHashes_().find(hash_, node);
		}
	}
	else {
		hxLogConsole("%08x: not found\n", (unsigned int)hash_);
	}
}

// use the debug console to emit and check file hashes.
hxConsoleCommandNamed(hxPrintHashes, printhashes);
hxConsoleCommandNamed(hxCheckHash, checkhash);

#endif

// ----------------------------------------------------------------------------
// init, shutdown, exit, assert and logging.

void hxSettingsConstruct();

extern "C"
void hxInitInternal(void) {
	hxAssertRelease(!g_hxIsInit, "call hxInit() instead");
	hxSettingsConstruct();
	g_hxIsInit = 1;

#if HX_FLOATING_POINT_TRAPS
	// You need the math library -lm. This is nonstandard glibc/_GNU_SOURCE.
    ::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

	hxMemoryManagerInit();

	// No DMA in a web browser.  The DMA code is just scaffolding.
#ifndef __EMSCRIPTEN__
	hxDmaInit();
#endif
}

extern "C"
HX_NOEXCEPT void hxLogHandler(hxLogLevel level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	hxLogHandlerV(level, format, args);
	va_end(args);
}

#define HX_STDOUT_STR_(x) ::fwrite(x, (sizeof x) - 1, 1, stdout)

extern "C"
HX_NOEXCEPT void hxLogHandlerV(hxLogLevel level, const char* format, va_list args) {
	if(g_hxIsInit && g_hxSettings.logLevel > level) {
		return;
	}

	char buf[HX_MAX_LINE+1];
	int sz = format ? vsnprintf(buf, HX_MAX_LINE, format, args) : -1;
	hxAssertRelease(sz >= 0, "format error: %s", format ? format : "(null)");
	if (sz <= 0) {
		return;
	}
	if (level == hxLogLevel_Warning) {
		HX_STDOUT_STR_("WARNING ");
		buf[sz++] = '\n';
	}
	else if (level == hxLogLevel_Assert) {
		HX_STDOUT_STR_("ASSERT_FAIL ");
		buf[sz++] = '\n';
	}
	::fwrite(buf, 1, sz, stdout);
}

// HX_RELEASE < 3 facilities for testing tear down. Just call _Exit() otherwise.
extern "C"
void hxShutdown(void) {
	if(g_hxIsInit) {
#if (HX_RELEASE) < 3
		// Will trap further activity and leaks.
		hxMemoryManagerShutDown();
#endif
	}
}

#if (HX_RELEASE) == 0
extern "C"
HX_NOEXCEPT int hxAssertHandler(const char* file, size_t line) {
	const char* f = hxBasename(file);
	if (g_hxIsInit && g_hxSettings.assertsToBeSkipped > 0) {
		--g_hxSettings.assertsToBeSkipped;
		hxLogHandler(hxLogLevel_Assert, "(skipped) %s(%u) hash %08x", f, (unsigned int)line,
			(unsigned int)hxStringLiteralHashDebug(file));
		return 1;
	}
	hxLogHandler(hxLogLevel_Assert, "%s(%u) hash %08x Triggering Breakpoint\n", f, (unsigned int)line,
		(unsigned int)hxStringLiteralHashDebug(file));

	// return to HX_DEBUG_BREAK at calling line.
	return 0;
}
#else
extern "C"
HX_NOEXCEPT HX_NORETURN void hxAssertHandler(uint32_t file, size_t line) {
	hxLogHandler(hxLogLevel_Assert, "file %08x line %u\n", (unsigned int)file, (unsigned int)line);
	_Exit(EXIT_FAILURE);
}
#endif
