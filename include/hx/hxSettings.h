#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#if !HATCHLING_VER
#error #include <hx/hatchling.h>
#endif

// ----------------------------------------------------------------------------
// Compiler detection and target specific C++11/C++14 polyfill.

#if defined(_MSC_VER)

#if !defined(__cpp_exceptions) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0 // must be included before standard headers
#endif

// MSVC doesn't support C++ feature test macros and sets __cplusplus wrong by
// default.
#define HX_USE_STDIO_H 1
#define HX_USE_CPP11_THREADS __STDCPP_THREADS__ 
#define HX_USE_CPP11_TIME 1
#define HX_USE_CPP14_CONSTEXPR 0 // silently generating horrible code as of MSVC 15.8.9.

#define HX_RESTRICT __restrict
#define HX_INLINE __forceinline
#define HX_LINK_SCRATCHPAD
#define HX_ATTR_FORMAT(pos, start)
#define HX_DEBUG_BREAK __debugbreak()

#if defined(__cplusplus) && __cplusplus >= 201103L
#define HX_STATIC_ASSERT(x,...) static_assert((bool)(x), __VA_ARGS__)
#define HX_OVERRIDE override
#define HX_ATTR_NORETURN [[noreturn]]
#else // !__cplusplus
#define HX_STATIC_ASSERT(x,...) typedef int HX_CONCATENATE(hxStaticAssertFail_,__LINE__) [!!(x) ? 1 : -1]
#define HX_OVERRIDE
#define HX_ATTR_NORETURN
#endif

#else // target settings
// This is configured for gcc and clang.  Other compilers will require customization.

#define HX_USE_STDIO_H 1

#if !defined(HX_USE_CPP11_THREADS)
#define HX_USE_CPP11_THREADS (__cplusplus >= 201103L)
#endif

#if !defined(HX_USE_CPP11_TIME)
#define HX_USE_CPP11_TIME (__cplusplus >= 201103L)
#endif

#if !defined(HX_USE_CPP14_CONSTEXPR)
// "__cpp_constexpr >= 201304" may not compile as C++
#define HX_USE_CPP14_CONSTEXPR (__cplusplus >= 201402L)
#endif

#define HX_RESTRICT __restrict
#define HX_INLINE inline __attribute__((always_inline))
#define HX_LINK_SCRATCHPAD // TODO: Configure for target.  A linker section is required.

#define HX_ATTR_FORMAT(pos, start) __attribute__((format(printf, pos, start)))
#define HX_ATTR_NORETURN __attribute__((noreturn))
#define HX_DEBUG_BREAK __builtin_trap()

#if defined(__cplusplus) && __cplusplus >= 201103L
#define HX_STATIC_ASSERT(x,...) static_assert(x, __VA_ARGS__)
#define HX_OVERRIDE override
#else // !C++98
#define HX_STATIC_ASSERT(x,...) typedef int HX_CONCATENATE(hxStaticAssertFail_,__LINE__) [!!(x) ? 1 : -1]
#define HX_OVERRIDE
#endif
#endif // target settings

// ----------------------------------------------------------------------------
// Target independent C++11/C++14 polyfill.

// HX_CONSTEXPR_FN indicates that a function is intended to be a C++14 constexpr
// function when available and inlined otherwise.  The expectation here is that
// by inlining a compiler will be able to perform similar optimizations when
// C++14 is not present and that having the rules for C++14's constexpr checked
// when available is useful.  Always check your generated assembly.
#if HX_USE_CPP14_CONSTEXPR
#define HX_CONSTEXPR_FN constexpr
#else
#define HX_CONSTEXPR_FN HX_INLINE
#endif

// HX_THREAD_LOCAL.  A version of thread_local that compiles out when there is
// no threading.
#if HX_USE_CPP11_THREADS
#define HX_THREAD_LOCAL thread_local
#else
#define HX_THREAD_LOCAL // single threaded operation can ignore thread_local
#endif

// ----------------------------------------------------------------------------
// Maximum length for formatted messages printed with this platform.
#define HX_MAX_LINE 180

// ----------------------------------------------------------------------------
// HX_MEM_DIAGNOSTIC_LEVEL.  Memory management debug mode.
//
// -1: remove code entirely
//  0: normal target operation
//  1: enable checking g_hxSettings.disableMemoryManager.
//  2: log allocator scopes
//  3: also log heap utilization
//
#if !defined(HX_MEM_DIAGNOSTIC_LEVEL)
#define HX_MEM_DIAGNOSTIC_LEVEL ((HX_RELEASE) < 2) ? 1 : 0
#endif

// Memory manager pool sizes
#define HX_KB (1u << 10) // 1024u
#define HX_MB (1u << 20) // 1048576u
#define HX_MEMORY_BUDGET_PERMANENT        5u * HX_KB
#define HX_MEMORY_BUDGET_TEMPORARY_STACK  1u * HX_MB
#define HX_MEMORY_BUDGET_SCRATCH_PAGE    10u * HX_KB
#define HX_MEMORY_BUDGET_SCRATCH_TEMP    60u * HX_KB

// When set to 0 HX_USE_MEMORY_SCRATCH will disable the scratchpad code.
#if !defined(HX_USE_MEMORY_SCRATCH)
#define HX_USE_MEMORY_SCRATCH ((HX_MEMORY_BUDGET_SCRATCH_PAGE + HX_MEMORY_BUDGET_SCRATCH_TEMP) != 0u)
#endif

// ----------------------------------------------------------------------------
// HX_PROFILE: 0 disables code for capturing profiling data
//             1 compiles in code.  See hxProfileScope().
#if !defined(HX_PROFILE)
#define HX_PROFILE (HX_RELEASE) < 2
#endif

// The profiler doesn't reallocate.  This is the maximum.
#define HX_PROFILER_MAX_RECORDS 4096

// ----------------------------------------------------------------------------
// HX_DEBUG_DMA.  Internal validation, set to 1 or 0 as needed
#if !defined(HX_DEBUG_DMA)
#define HX_DEBUG_DMA (HX_RELEASE) < 1
#endif

// Number of DMA operations tracked.
#define HX_DEBUG_DMA_RECORDS 16

// ----------------------------------------------------------------------------
// HX_USE_GOOGLE_TEST:  In case you need to use Google Test.
#if !defined(HX_USE_GOOGLE_TEST)
#define HX_USE_GOOGLE_TEST 0
#endif

// Set by etc/coverage.sh
#if !defined(HX_TEST_DIE_AT_THE_END)
#define HX_TEST_DIE_AT_THE_END 0
#endif

// ----------------------------------------------------------------------------
// HX_RADIX_SORT_*.  Tuning radix sort algorithm.
// These need to be determined by benchmarking on the target platform.  The 8-
// bit version tries to be memory efficient, the 11-bit version optimizes for
// speed over memory.
#define HX_RADIX_SORT_BITS 8 // either 8 or 11.
#define HX_RADIX_SORT_MIN_SIZE 50u // uses hxInsertionSort() below this.

// ----------------------------------------------------------------------------
// Default undetected HX_USE_* features.

#if !defined(HX_USE_STDIO_H)
#define HX_USE_STDIO_H 1
#endif

// size_t is used regardless because it is expected to be 32-bit on the target.
#if !defined(HX_USE_64_BIT_TYPES)
#define HX_USE_64_BIT_TYPES 1
#endif

#if !defined(HX_USE_DMA_HARDWARE)
#define HX_USE_DMA_HARDWARE 0
#endif

// ----------------------------------------------------------------------------
// hxSettings.  Constructed by first call to hxInit() which happens when or
// before the memory allocator constructs.

struct hxSettings {
	const char* logFile;
	uint8_t logLevel; // For logFile
	uint8_t isShuttingDown; // Allows destruction of permanent resources.

#if (HX_MEM_DIAGNOSTIC_LEVEL) > 0
	uint8_t disableMemoryManager;
#endif
#if (HX_RELEASE) < 1
	int32_t assertsToBeSkipped; // Allows testing asserts.
	uint8_t deathTest;
	float lightEmittingDiode;
#endif
};

// Constructed by hxInit().
#if defined(__cplusplus)
extern "C" {
#endif
extern struct hxSettings g_hxSettings;
#if defined(__cplusplus)
}
#endif
