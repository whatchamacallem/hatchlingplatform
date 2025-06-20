#pragma once
// Copyright 2017-2025 Adrian Johnston

// hxsettings.h - Compiler detection and target specific C++11/C++14 polyfill.
// Use #if(HX_...) instead of #ifdef(HX_...) for all HX_* macros.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

// HX_RELEASE - C/C++ optimization level. See the README.md for levels 0..3.
#if !defined HX_RELEASE
#if defined NDEBUG
#define HX_RELEASE 1
#else
#define HX_RELEASE 0
#endif
#endif

// HX_HATCHLING_PCH_USED - Allows checking if hatchling_pch.h was used as expected.
#if !defined HX_HATCHLING_PCH_USED
#define HX_HATCHLING_PCH_USED 0
#endif

// HX_CPLUSPLUS - A version of __cplusplus that is defined to 0 when __cplusplus
// is undefined. Allows use in logical operations in preprocessor statements without
// warnings when the compiler is configured to warn about undefined parameters.
#if defined __cplusplus
#define HX_CPLUSPLUS __cplusplus
#else
#define HX_CPLUSPLUS 0
#endif

// ----------------------------------------------------------------------------
// MSVC doesn't support C++ feature test macros very well.
#if defined _MSC_VER

// _HAS_EXCEPTIONS - _MSC_VER only. Disables exception handling. Must be
// included before standard headers.
#if !defined __cpp_exceptions && !defined _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 0
#endif

#if !defined HX_USE_CPP_THREADS
// HX_USE_CPP_THREADS - _MSC_VER specific thread support macro.
#define HX_USE_CPP_THREADS __STDCPP_THREADS__
#endif

// A hosted environment has an OS and C++ standard library.
#define HX_HOSTED 1 // HX_HOSTED - 1: _MSC_VER indicates a hosted environment.

#define HX_RESTRICT __restrict
#define HX_ATTR_FORMAT(pos_, start_)
#define hxbreakpoint() (__debugbreak(),false)
#define HX_NORETURN

// Unlike noexcept this is undefined when violated.
#if HX_CPLUSPLUS >= 201103L
#define HX_NOEXCEPT_INTRINSIC __declspec(nothrow) // HX_NOEXCEPT_INTRINSIC - _MSC_VER's nothrow attribute.
#define HX_NOEXCEPT noexcept // HX_NOEXCEPT - Use noexcept for C++11+.
#else
#define HX_NOEXCEPT_INTRINSIC // HX_NOEXCEPT_INTRINSIC - No-op for pre-C++11.
#define HX_NOEXCEPT // HX_NOEXCEPT - No-op for pre-C++11.
#endif

// ----------------------------------------------------------------------------
#else // target settings (clang and gcc.)
// Other compilers will require customization.

// C++11 threads in WASM needs more glue.
#if !defined HX_USE_CPP_THREADS
#ifdef __EMSCRIPTEN__
// HX_USE_CPP_THREADS - Set to 0 for Emscripten.
#define HX_USE_CPP_THREADS 0
#else
// HX_USE_CPP_THREADS - Enable threads if C++11 or newer.
#define HX_USE_CPP_THREADS (HX_CPLUSPLUS >= 201103L)
#endif
#endif

#ifdef __GLIBC__
// HX_HOSTED - Assume standard C++ headers and library are only available when
// on windows or when glibc is. This is a "hosted" environment.
#define HX_HOSTED 1
#else
// HX_HOSTED - Set to 0 when compiling with gcc/clang as glibc is not present.
#define HX_HOSTED 0
#endif

#define HX_RESTRICT __restrict
#define HX_ATTR_FORMAT(pos_, start_) __attribute__((format(printf, pos_, start_)))
#define HX_NORETURN __attribute__((noreturn))

// hxbreakpoint - Can be conditionally evaluated with the && and || operators.
// Uses intrinsics when available. (E.g. Clang.)
#if defined __has_builtin && __has_builtin(__builtin_debugtrap)
#define hxbreakpoint() (__builtin_debugtrap(),false)
#else
#define hxbreakpoint() (raise(SIGTRAP),false) // hxbreakpoint - Use SIGTRAP if debugtrap is not available.
#endif

// HX_NOEXCEPT_INTRINSIC - Use GCC/Clang nothrow attribute. Unlike noexcept this
// is undefined when violated. HX_NOEXCEPT - Use noexcept when available.
#if HX_CPLUSPLUS >= 201103L
#define HX_NOEXCEPT_INTRINSIC __attribute__((nothrow))
#define HX_NOEXCEPT noexcept
#else
#define HX_NOEXCEPT_INTRINSIC
#define HX_NOEXCEPT
#endif

#endif // target settings

// ----------------------------------------------------------------------------
// Target independent C++11/C++14 polyfill.

#if HX_CPLUSPLUS >= 201103L
// HX_STATIC_ASSERT - Use static_assert when available, polyfill otherwise.
#define HX_STATIC_ASSERT(x_,...) static_assert((bool)(x_), __VA_ARGS__)
// HX_OVERRIDE - Use override when available.
#define HX_OVERRIDE override
 // HX_DELETE_FN - Use delete when available.
#define HX_DELETE_FN = delete
#else // !HX_CPLUSPLUS
#define HX_STATIC_ASSERT(x_,...) typedef int HX_CONCATENATE(hxstatic_assert_fail_,__COUNTER__) [!!(x_) ? 1 : -1] // HX_STATIC_ASSERT - Fallback static assert for pre-C++11.
#define HX_OVERRIDE
#define HX_DELETE_FN
#endif

// HX_CONSTEXPR_FN - When available, indicates that a function is intended to
// be a C++14 constexpr function. Falls back to inline otherwise.
#if HX_CPLUSPLUS >= 201402L
#define HX_CONSTEXPR_FN constexpr
#else
#define HX_CONSTEXPR_FN inline
#endif

// HX_THREAD_LOCAL - A version of thread_local that compiles to nothing when
// there is no threading.
#if HX_USE_CPP_THREADS && !defined HX_THREAD_LOCAL
#define HX_THREAD_LOCAL thread_local
#else
#define HX_THREAD_LOCAL // single threaded operation can ignore thread_local
#endif

// HX_MAX_LINE - Set to 500 if not defined. Maximum length for formatted messages
// printed with this platform.
#if !defined HX_MAX_LINE
#define HX_MAX_LINE 500
#endif

// HX_MEMORY_MANAGER_DISABLE - Disables memory management for debugging and for
// platforms like wasm where extra system allocations are probably cheaper than
// code size.
//  0: normal target operation
//  1: remove code entirely
#if !defined HX_MEMORY_MANAGER_DISABLE
#define HX_MEMORY_MANAGER_DISABLE 0
#endif

#define HX_KIB (1u << 10) // HX_KIB - A Ki_b is 1024 bytes.
#define HX_MIB (1u << 20) // HX_MIB - A Mi_b is 1,048,576 bytes.

// HX_MEMORY_BUDGET_PERMANENT - Pool sizes. Set to 5 Ki_b if not defined.
#if !defined HX_MEMORY_BUDGET_PERMANENT
#define HX_MEMORY_BUDGET_PERMANENT        (5u * HX_KIB)
#endif

// HX_MEMORY_BUDGET_TEMPORARY_STACK - Set to 1 Mi_b if not defined.
#if !defined HX_MEMORY_BUDGET_TEMPORARY_STACK
#define HX_MEMORY_BUDGET_TEMPORARY_STACK  (1u * HX_MIB)
#endif

// HX_PROFILE - 0: disables code for capturing profiling data
//              1: compiles in code. See hxprofile_scope().
#if !defined HX_PROFILE
#define HX_PROFILE (HX_RELEASE) < 2
#endif

// HX_PROFILER_MAX_RECORDS - Set to 4096 if not defined. The profiler doesn't
// reallocate. This is the maximum.
#if !defined HX_PROFILER_MAX_RECORDS
#define HX_PROFILER_MAX_RECORDS 4096
#endif

// HX_DEBUG_DMA - Enable DMA debug if not defined and not a release build. Set
// to 1 or 0 as needed.
#if !defined HX_DEBUG_DMA
#define HX_DEBUG_DMA (HX_RELEASE) < 1
#endif

// HX_DEBUG_DMA_RECORDS - Number of debug DMA operations tracked. Set to 16 if
// not defined.
#if !defined HX_DEBUG_DMA_RECORDS
#define HX_DEBUG_DMA_RECORDS 16
#endif

// HX_USE_GOOGLE_TEST: In case you need to use Google Test. Defaults to 0.
#if !defined HX_USE_GOOGLE_TEST
#define HX_USE_GOOGLE_TEST 0
#endif

// HX_TEST_ERROR_HANDLING - Tests that the failure of tests is handled correctly.
// Set to 0 if not defined. Set by testerrorhandling.sh and coverage.sh
#if !defined HX_TEST_ERROR_HANDLING
#define HX_TEST_ERROR_HANDLING 0
#endif

// HX_RADIX_SORT_BITS. Radix sort algorithm configuration parameter. The 8-
// bit version tries to be memory efficient, the 11-bit version might make
// sense for large data sets. Set to 8 if not defined (either 8 or 11).
#if !defined HX_RADIX_SORT_BITS
#define HX_RADIX_SORT_BITS 8 // either 8 or 11.
#endif

// HX_RADIX_SORT_MIN_SIZE - Radix sort uses hxinsertion_sort() below this size.
// Set to 8 if not defined.
#if !defined HX_RADIX_SORT_MIN_SIZE
#define HX_RADIX_SORT_MIN_SIZE 8u
#endif

#if HX_CPLUSPLUS
extern "C" {
#endif

// hxsettings. Constructed by first call to hxinit() which happens when or
// before the memory allocator constructs.
struct hxsettings {
    uint8_t log_level;              // Logging level for the application (e.g., verbosity of logs).
    uint8_t deallocate_permanent;   // Allows deallocation of permanent resources at system shut down.

#if (HX_RELEASE) < 1
    int asserts_to_be_skipped;        // Number of asserts to skip, useful for testing assert behavior.
#endif
};

// g_hxsettings. Global class constructed by hxinit().
extern struct hxsettings g_hxsettings;

#if HX_CPLUSPLUS
} // extern "C"
#endif
