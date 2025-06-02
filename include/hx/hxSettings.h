#pragma once
// Copyright 2017-2025 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

// HX_RELEASE: 0 is a debug build with all asserts and verbose strings.
//             1 is a release build with critical asserts and verbose warnings.
//               E.g. for CMake's "RelWithDebInfo".
//             2 is a release build with only critical asserts using minimal strings.
//               E.g. for CMake's "MinSizeRel".
//             3 no asserts or tear down and very minimal logging.
//
#if !defined(HX_RELEASE)
#if defined(NDEBUG)
#define HX_RELEASE 1
#else
#define HX_RELEASE 0
#endif
#endif

// Allows calling code to check if hatchlingPch.h was used correctly.
#if !defined(HX_HATCHLING_PCH_USED)
#define HX_HATCHLING_PCH_USED 0
#endif

// Compiler detection and target specific C++11/C++14 polyfill.
// Use #if(HX_...) instead of #ifdef(HX_...) for all HX_* macros.

#if defined(__cplusplus)
#define HX_CPLUSPLUS __cplusplus
#else
#define HX_CPLUSPLUS 0
#endif

// ----------------------------------------------------------------------------
// MSVC doesn't support C++ feature test macros very well.
#if defined(_MSC_VER)

#if !defined(__cpp_exceptions) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0 // must be included before standard headers
#endif

#define HX_USE_WASM 0

#if !defined(HX_USE_CPP_THREADS)
#define HX_USE_CPP_THREADS __STDCPP_THREADS__
#endif
#if !defined(HX_USE_CHRONO)
#define HX_USE_CHRONO (_MSC_VER >= 1900)
#endif

#define HX_RESTRICT __restrict
#define HX_LINK_SCRATCHPAD
#define HX_ATTR_FORMAT(pos_, start_)
#define HX_DEBUG_BREAK __debugbreak()
#define HX_ATTR_NORETURN

// Unlike noexcept this is undefined when violated.
#if HX_CPLUSPLUS >= 201103L
#define HX_NOEXCEPT __declspec(nothrow)
#else
#define HX_NOEXCEPT
#endif

// ----------------------------------------------------------------------------
#else // target settings (clang and gcc.)
// Other compilers will require customization.

// WebAssembly.  Support use with any compiler front end.
#if !defined(HX_USE_WASM)
#if defined(__EMSCRIPTEN__)
#define HX_USE_WASM 1
#else
#define HX_USE_WASM 0
#endif
#endif

#if !defined(HX_USE_CPP_THREADS)
#define HX_USE_CPP_THREADS (HX_CPLUSPLUS >= 201103L)
#endif
#if !defined(HX_USE_CHRONO)
#define HX_USE_CHRONO (HX_CPLUSPLUS >= 201103L)
#endif

#define HX_RESTRICT __restrict
#define HX_LINK_SCRATCHPAD // TODO: Configure for target.  A linker section is required.

#define HX_ATTR_FORMAT(pos_, start_) __attribute__((format(printf, pos_, start_)))
#define HX_ATTR_NORETURN __attribute__((noreturn))

// HX_DEBUG_BREAK can be conditionally evaluated with the && and || operators.
#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
#define HX_DEBUG_BREAK (__builtin_debugtrap(),false)
#else
#define HX_DEBUG_BREAK (raise(SIGTRAP),false)
#endif

// Unlike noexcept this is undefined when violated.
#if HX_CPLUSPLUS >= 201103L
#define HX_NOEXCEPT __attribute__((nothrow))
#else
#define HX_NOEXCEPT
#endif

#endif // target settings

// ----------------------------------------------------------------------------
// Target independent C++11/C++14 polyfill.

#if HX_CPLUSPLUS >= 201103L
#define HX_STATIC_ASSERT(x_,...) static_assert((bool)(x_), __VA_ARGS__)
#define HX_OVERRIDE override
#define HX_DELETE_FN = delete
#else // !HX_CPLUSPLUS
#define HX_STATIC_ASSERT(x_,...) typedef int HX_CONCATENATE(hxStaticAssertFail_,__COUNTER__) [!!(x_) ? 1 : -1]
#define HX_OVERRIDE
#define HX_DELETE_FN
#endif

// HX_CONSTEXPR_FN indicates that a function is intended to be a C++14 constexpr
// function when available.
#if HX_CPLUSPLUS >= 201402L
#define HX_CONSTEXPR_FN constexpr
#else
#define HX_CONSTEXPR_FN inline
#endif

// HX_THREAD_LOCAL.  A version of thread_local that compiles out when there is
// no threading.
#if HX_USE_CPP_THREADS && !defined(HX_THREAD_LOCAL)
#define HX_THREAD_LOCAL thread_local
#else
#define HX_THREAD_LOCAL // single threaded operation can ignore thread_local
#endif

// ----------------------------------------------------------------------------
// Maximum length for formatted messages printed with this platform.
#if !defined(HX_MAX_LINE)
#define HX_MAX_LINE 500
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
#if !defined(HX_MEM_DIAGNOSTIC_LEVEL)
#define HX_MEM_DIAGNOSTIC_LEVEL ((HX_RELEASE) < 2) ? 1 : 0
#endif

#define HX_KIB (1u << 10) // A KiB is 1024 bytes.
#define HX_MIB (1u << 20) // A MiB is 1,048,576 bytes.

// Pool sizes.
#if !defined(HX_MEMORY_BUDGET_PERMANENT)
#define HX_MEMORY_BUDGET_PERMANENT        (5u * HX_KIB)
#endif
#if !defined(HX_MEMORY_BUDGET_TEMPORARY_STACK)
#define HX_MEMORY_BUDGET_TEMPORARY_STACK  (1u * HX_MIB)
#endif
#if !defined(HX_MEMORY_BUDGET_SCRATCH_PAGE)
#define HX_MEMORY_BUDGET_SCRATCH_PAGE    (10u * HX_KIB)
#endif
#if !defined(HX_MEMORY_BUDGET_SCRATCH_TEMP)
#define HX_MEMORY_BUDGET_SCRATCH_TEMP    (60u * HX_KIB)
#endif

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
#if !defined(HX_PROFILER_MAX_RECORDS)
#define HX_PROFILER_MAX_RECORDS 4096
#endif

// ----------------------------------------------------------------------------
// HX_DEBUG_DMA.  Internal validation, set to 1 or 0 as needed
#if !defined(HX_DEBUG_DMA)
#define HX_DEBUG_DMA (HX_RELEASE) < 1
#endif

// Number of DMA operations tracked.
#if !defined(HX_DEBUG_DMA_RECORDS)
#define HX_DEBUG_DMA_RECORDS 16
#endif

// ----------------------------------------------------------------------------
// HX_USE_GOOGLE_TEST:  In case you need to use Google Test.
#if !defined(HX_USE_GOOGLE_TEST)
#define HX_USE_GOOGLE_TEST 0
#endif

// Set by coverage.sh
#if !defined(HX_TEST_DIE_AT_THE_END)
#define HX_TEST_DIE_AT_THE_END 0
#endif

// ----------------------------------------------------------------------------
// HX_RADIX_SORT_*.  Tuning radix sort algorithm.
// These need to be determined by benchmarking on the target platform.  The 8-
// bit version tries to be memory efficient, the 11-bit version optimizes for
// speed over memory.
#if !defined(HX_RADIX_SORT_BITS)
#define HX_RADIX_SORT_BITS 8 // either 8 or 11.
#endif

#if !defined(HX_RADIX_SORT_MIN_SIZE)
#define HX_RADIX_SORT_MIN_SIZE 8u // uses hxInsertionSort() below this.
#endif

// ----------------------------------------------------------------------------
// Default undetected HX_USE_* features.

#if !defined(HX_USE_DMA_HARDWARE)
#define HX_USE_DMA_HARDWARE 0
#endif

// ----------------------------------------------------------------------------
// hxSettings.  Constructed by first call to hxInit() which happens when or
// before the memory allocator constructs.

#if HX_CPLUSPLUS
extern "C" {
#endif

struct hxSettings {
    uint8_t logLevel;              // Logging level for the application (e.g., verbosity of logs).
    uint8_t deallocatePermanent;   // Allows deallocation of permanent resources at system shut down.

#if (HX_MEM_DIAGNOSTIC_LEVEL) > 0
    uint8_t disableMemoryManager;  // Disables the memory manager when set, useful for debugging memory issues.
#endif
#if (HX_RELEASE) < 1
    int assertsToBeSkipped;        // Number of asserts to skip, useful for testing assert behavior.
    float lightEmittingDiode;      // Placeholder for testing.
#endif
};

// g_hxSettings.  Global struct constructed by hxInit().
extern struct hxSettings g_hxSettings;

#if HX_CPLUSPLUS
} // extern "C"
#endif
