#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#ifndef HATCHLING_H
#error #include <hx/hatchling.h>
#endif

// ----------------------------------------------------------------------------
// Compiler detection and some C++11 polyfill.

#ifdef _MSC_VER

#if !defined(__cpp_exceptions) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0 // must be included before standard headers
#endif

// MSVC doesn't support C++ feature test macros, and sets __cplusplus wrong by default
#define HX_USE_C_FILE 1
#define HX_USE_CPP11_THREADS 1
#define HX_USE_CPP11_TIME 1
#define HX_USE_CPP14_CONSTEXPR 0 // silently generating horrible code as of MSVC 15.8.9.

#define HX_RESTRICT __restrict
#define HX_INLINE __forceinline
#define HX_LINK_SCRATCHPAD
#define HX_ATTR_FORMAT(pos, start)
#define HX_DEBUG_BREAK __debugbreak()
#define HX_STATIC_ASSERT(x,...) static_assert((bool)(x), __VA_ARGS__)
#define HX_OVERRIDE override

#if __cplusplus
#define HX_ATTR_NORETURN [[noreturn]]
#else // !__cplusplus
#define HX_ATTR_NORETURN
#endif

// Allow use of fopen and strncpy.  fopen_s and strncpy_s are not C99.
#pragma warning(disable: 4996)

#else // !_MSC_VER
// "Some C++98 embedded compiler" (currently gcc.)

#define HX_USE_C_FILE 1

#ifndef HX_USE_CPP11_THREADS
#define HX_USE_CPP11_THREADS (__cplusplus >= 201103L)
#endif

#ifndef HX_USE_CPP11_TIME
#define HX_USE_CPP11_TIME (__cplusplus >= 201103L)
#endif

#ifndef HX_USE_CPP14_CONSTEXPR
#define HX_USE_CPP14_CONSTEXPR (__cplusplus >= 201402L) // "__cpp_constexpr >= 201304" may not compile as C++
#endif

#define HX_RESTRICT __restrict
#define HX_INLINE inline __attribute__((always_inline))
#define HX_LINK_SCRATCHPAD // TODO
#define HX_ATTR_FORMAT(pos, start) __attribute__((format(printf, pos, start)))
#define HX_ATTR_NORETURN __attribute__((noreturn))
#define HX_DEBUG_BREAK __builtin_trap()

#if defined(__cplusplus) && __cplusplus < 201103L // C++98
#define HX_STATIC_ASSERT(x,...) typedef int HX_CONCATENATE(hxStaticAssertFail_,__LINE__) [(bool)(x) ? 1 : -1]
#define HX_OVERRIDE
#else // !C++98
#define HX_STATIC_ASSERT(x,...) static_assert(x, __VA_ARGS__)
#define HX_OVERRIDE override
#endif
#endif // !_MSC_VER

// ----------------------------------------------------------------------------

#if HX_USE_CPP11_THREADS
#define HX_THREAD_LOCAL thread_local
#else
// single threaded operation can ignore thread_local
#define HX_THREAD_LOCAL
#endif

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
#define HX_PROFILER_MAX_RECORDS 4096

// ----------------------------------------------------------------------------
// HX_DEBUG_DMA.  Internal validation, set to 1 or 0 as needed
#ifndef HX_DEBUG_DMA
#define HX_DEBUG_DMA (HX_RELEASE) < 1
#endif

// Number of DMA operations tracked.
#define HX_DEBUG_DMA_RECORDS 16

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
#define HX_RADIX_SORT_MIN_SIZE 50u // uses hxInsertionSort() below this.

// ----------------------------------------------------------------------------
// PRINTF_* See <hx/printf.h>

// support for the floating point type (%f)
#ifndef PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_SUPPORT_FLOAT
#endif

// support for the long long types (%llu or %p)
#ifndef PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_LONG_LONG
#endif

// support for the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
#ifndef PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define PRINTF_SUPPORT_PTRDIFF_T
#endif

// ----------------------------------------------------------------------------
// Default undetected HX_USE_* features.

#ifndef HX_USE_C_FILE
#define HX_USE_C_FILE 1
#endif

// size_t is used regardless because it is expected to be 32-bit on the target.
#ifndef HX_USE_64_BIT_TYPES
#define HX_USE_64_BIT_TYPES 1
#endif

// ----------------------------------------------------------------------------
// hxSettings.  Constructed by first call to hxInit() which happens when or
// before the memory allocator constructs.

struct hxSettings {
	const char* logFile;
	uint8_t logLevel; // logFile
	uint8_t isShuttingDown; // Allows destruction of permanent resources.

#if (HX_MEM_DIAGNOSTIC_LEVEL) > 0
	uint8_t disableMemoryManager;
#endif
#if (HX_RELEASE) < 1
	uint32_t assertsToBeSkipped; // Allows testing asserts.
#endif
};

// Constructed by hxInit().
#if __cplusplus
extern "C" {
#endif
extern struct hxSettings g_hxSettings;
#if __cplusplus
}
#endif
