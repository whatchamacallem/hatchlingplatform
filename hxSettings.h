#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#ifndef HATCHLING_H
#error "include hatchling.h"
#endif

// ----------------------------------------------------------------------------
// Compiler detection and some C++11 polyfill.

#ifdef _MSC_VER
#define HX_HAS_C_FILE 1
#define HX_HAS_CPP11_THREADS 1
#define HX_HAS_CPP11_TIME 1

#define HX_RESTRICT __restrict
#define HX_INLINE __forceinline
#define HX_LINK_SCRATCHPAD
#define HX_ATTR_FORMAT(pos, start)
#define HX_DEBUG_BREAK __debugbreak()

#ifdef __cplusplus
#define HX_ATTR_NORETURN [[noreturn]]
#else // !__cplusplus
#define HX_ATTR_NORETURN
#endif

// Allow use of fopen and strncpy.  fopen_s and strncpy_s are not C99.
#pragma warning(disable: 4996)

#else // !HX_HOST
// "Some C++98 embedded compiler" (currently gcc.)

#define HX_TARGET 1
#define HX_HAS_C_FILE 1

#define HX_RESTRICT __restrict
#define HX_INLINE inline __attribute__((always_inline))
#define HX_LINK_SCRATCHPAD // TODO
#define HX_ATTR_FORMAT(pos, start) __attribute__((format(printf, pos, start)))
#define HX_ATTR_NORETURN __attribute__((noreturn))
#define HX_DEBUG_BREAK ((void)0) // TODO

#ifdef __cplusplus
// C++11 polyfill
#define static_assert(x,...) typedef int HX_CONCATENATE(hxStaticAssertFail_,__LINE__) [(x) ? 1 : -1]
#define override
#endif // __cplusplus
#endif // !HX_HOST

#define null 0

// ----------------------------------------------------------------------------
// HX_MEM_DIAGNOSTIC_LEVEL.  Memory management debug mode.
//
// -1: remove code entirely
//  0: normal target operation
//  1: enable checking g_hxSettings.disableMemoryManager.
//  2: log allocator scopes
//  3: also log heap utilization
//
#define HX_MEM_DIAGNOSTIC_LEVEL ((HX_RELEASE) < 2) ? 1 : 0

// Memory manager pool sizes
#define HX_KB (1u << 10) // 1024u
#define HX_MB (1u << 20) // 1048576u
#define HX_MEMORY_BUDGET_PERMANENT        5u * HX_KB
#define HX_MEMORY_BUDGET_TEMPORARY_STACK  1u * HX_MB
#define HX_MEMORY_BUDGET_SCRATCH_PAGE    10u * HX_KB
#define HX_MEMORY_BUDGET_SCRATCH_TEMP    60u * HX_KB

// ----------------------------------------------------------------------------
// HX_PROFILE: 0 disables code for capturing profiling data
//             1 compiles in code.  See hxProfileScope().
#ifndef HX_PROFILE
#define HX_PROFILE (HX_RELEASE) < 2
#endif

// The profiler doesn't reallocate.  This is the maximum.
#define HX_PROFILER_MAX_RECORDS 1000

// ----------------------------------------------------------------------------
// HX_DEBUG_DMA.  Internal validation, set to 1 or 0 as needed
#ifndef HX_DEBUG_DMA
#define HX_DEBUG_DMA (HX_RELEASE) < 1
#endif

// ----------------------------------------------------------------------------
// HX_GOOGLE_TEST:  In case you need to use GoogleTest.
#ifndef HX_GOOGLE_TEST
#define HX_GOOGLE_TEST 0
#endif

// ----------------------------------------------------------------------------
// HX_RADIX_SORT_*.  Tuning radix sort algorithm.
// These need to be determined by benchmarking on the target platform.  The 8-
// bit version tries to be memory efficient, the 11-bit version optimizes for
// speed over memory.
#define HX_RADIX_SORT_BITS 8 // either 8 or 11.
#define HX_RADIX_SORT_MIN_SIZE 50u // uses std::sort() below this.

// ----------------------------------------------------------------------------
// Default undetected HX_HAS_* features to 0.

#ifndef HX_TARGET
#define HX_TARGET 0
#endif

#ifndef HX_HAS_C_FILE
#define HX_HAS_C_FILE 0
#endif

#ifndef HX_HAS_CPP11_THREADS
#define HX_HAS_CPP11_THREADS 0
#endif

#ifndef HX_HAS_CPP11_TIME
#define HX_HAS_CPP11_TIME 0
#endif

// ----------------------------------------------------------------------------
// hxSettings.  Constructed by first call to hxInit() which happens when or
// before the memory allocator constructs.
//
// See also hxConsoleVariable().
#ifdef __cplusplus
struct hxSettings {
public:
	void construct();

	static const uint32_t c_settingsIntegrityCheck = 0xe28575c3u;
	uint32_t settingsIntegrityCheck;

	int32_t logLevelConsole;
	int32_t logLevelFile; // logFile
	const char* logFile;
	bool isShuttingDown; // Allows destruction of permanent resources.
#if (HX_RELEASE) < 1
	int32_t assertsToBeSkipped; // Allows testing asserts.
#endif
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	bool disableMemoryManager;
#endif
};

// Constructed by hxInit().
extern hxSettings g_hxSettings;
#endif // __cpp_exceptions
