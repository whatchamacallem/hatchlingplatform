#pragma once
// Copyright 2017-2025 Adrian Johnston

// hxsettings.h - Compiler detection and target specific C++11/C++14 polyfill.
// Use #if(HX_...) instead of #ifdef(HX_...) for all HX_* macros.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

/// `HX_RELEASE` - C/C++ optimization level. See the README.md for levels 0..3.
#if !defined HX_RELEASE
#if defined NDEBUG
#define HX_RELEASE 1
#else
#define HX_RELEASE 0
#endif
#endif

/// `HX_HATCHLING_PCH_USED` - Allows checking if `hatchling_pch.h` was used as expected.
#if !defined HX_HATCHLING_PCH_USED
#define HX_HATCHLING_PCH_USED 0
#endif

/// `HX_CPLUSPLUS` - A version of `__cplusplus` that is defined to `0` when `__cplusplus`
/// is undefined. Allows use in logical operations in preprocessor statements without
/// warnings when the compiler is configured to warn about undefined parameters.
#if defined __cplusplus
#define HX_CPLUSPLUS __cplusplus
#else
#define HX_CPLUSPLUS 0
#endif

// ----------------------------------------------------------------------------
// MSVC doesn't support C++ feature test macros very well.
#if defined _MSC_VER

/// `_HAS_EXCEPTIONS` - _MSC_VER only. Disables exception handling. Must be
/// included before standard headers.
#if !defined __cpp_exceptions && !defined _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 0
#endif

#if !defined HX_USE_THREADS
/// `HX_USE_THREADS` - `_MSC_VER` indicates pthread support.
#define HX_USE_THREADS 1
#endif

/// A hosted environment has an OS and C++ standard library.
#define HX_HOSTED 1 /// `HX_HOSTED` - `1` : _MSC_VER indicates a hosted environment.

#define hxrestrict __restrict
#define hxattr_format(pos_, start_)
#define hxbreakpoint() (__debugbreak(),false)
#define hxnoreturn

/// Unlike noexcept this is undefined when violated.
#if HX_CPLUSPLUS >= 201103L
#define hxnoexcept_intrinsic __declspec(nothrow) /// `hxnoexcept_intrinsic` - `_MSC_VER's` `nothrow` attribute.
#define hxnoexcept noexcept /// `hxnoexcept` - Use `noexcept` for C++11+.
#else
#define hxnoexcept_intrinsic /// `hxnoexcept_intrinsic` - No-op for pre-C++11.
#define hxnoexcept /// `hxnoexcept` - No-op for pre-C++11.
#endif

// ----------------------------------------------------------------------------
#else // target settings (clang and gcc.)
// Other compilers will require customization.

// hxthreads.hpp should work in C++11 with pthread.h. WASM still needs to be
// hooked up. _POSIX_THREADS is the correct way to observe the -pthread compiler
// flag.
#if !defined HX_USE_THREADS
#if defined _POSIX_THREADS
#define HX_USE_THREADS 1
#else
#define HX_USE_THREADS 0
#endif
#endif

#ifdef __GLIBC__
/// `HX_HOSTED` - Assume standard C++ headers and library are only available when
/// on windows or when glibc is. This is a "hosted" environment.
#define HX_HOSTED 1
#else
/// `HX_HOSTED` - Set to 0 when compiling with gcc/clang as glibc is not present.
#define HX_HOSTED 0
#endif

#define hxrestrict __restrict
#define hxattr_format(pos_, start_) __attribute__((format(printf, pos_, start_)))
#define hxnoreturn __attribute__((noreturn))

#if defined __has_builtin && __has_builtin(__builtin_debugtrap)
/// `hxbreakpoint` - Can be conditionally evaluated with the `&&` and `||` operators.
/// Uses intrinsics when available. (E.g. clang.)
#define hxbreakpoint() (__builtin_debugtrap(),false)
#else
#define hxbreakpoint() (raise(SIGTRAP),false) /// `hxbreakpoint` - Use SIGTRAP if debugtrap is not available.
#endif

#if HX_CPLUSPLUS >= 201103L
/// `hxnoexcept_intrinsic` - Use gcc/clang `nothrow` attribute. Unlike `noexcept` this
/// is undefined when violated.
#define hxnoexcept_intrinsic __attribute__((nothrow))
/// `hxnoexcept` - Uses noexcept when available.
#define hxnoexcept noexcept
#else
#define hxnoexcept_intrinsic
#define hxnoexcept
#endif

#endif // target settings

// ----------------------------------------------------------------------------
// Target independent C++11/C++14 polyfill.

// HX_APPEND_COUNTER2_ - Used to generate unique identifiers. This is weird
// because the ## operator happens before macro arg evaluation and both
// happen before general macro evaluation.
#define HX_APPEND_COUNTER2_(x_, y_) x_ ## y_
// This version does evaluates its macro args.
#define HX_APPEND_COUNTER1_(x_, y_) HX_APPEND_COUNTER2_(x_, y_)
/// `HX_APPEND_COUNTER` - Generates a unique identifier by appending a unique
/// number to `x`.
/// - `x` : A C identifier to be made unique.
#define HX_APPEND_COUNTER_(x_) HX_APPEND_COUNTER1_(x_, __COUNTER__)

#if HX_CPLUSPLUS >= 201103L
/// `hxstatic_assert` - Use static_assert when available, polyfill otherwise.
#define hxstatic_assert(x_,...) static_assert((bool)(x_), __VA_ARGS__)
/// `hxoverride` - Use override when available.
#define hxoverride override
 /// `hxdelete_fn` - Use delete when available.
#define hxdelete_fn = delete
#else // !HX_CPLUSPLUS
/// `hxstatic_assert` - Fallback static assert for pre-C++11.
#define hxstatic_assert(x_,...) typedef int HX_APPEND_COUNTER_(hxstatic_assert_fail_) [(bool)(x_) ? 1 : -1]
#define hxoverride
#define hxdelete_fn
#endif

#if HX_CPLUSPLUS >= 201402L
/// `hxconstexpr_fn` - When available, indicates that a function is intended to
/// be a C++14 `constexpr` function. Falls back to `inline` otherwise.
#define hxconstexpr_fn constexpr
#else
#define hxconstexpr_fn inline
#endif

#if !defined HX_MAX_LINE
/// `HX_MAX_LINE` - Set to 500 if not defined. Maximum length for formatted messages
/// printed with this platform.
#define HX_MAX_LINE 500
#endif

#if !defined HX_MEMORY_MANAGER_DISABLE
/// `HX_MEMORY_MANAGER_DISABLE` - Disables memory management for debugging and for
/// platforms like wasm where extra system allocations are probably cheaper than
/// code size.
/// - `0` : normal target operation
/// - `1` : remove code entirely
#define HX_MEMORY_MANAGER_DISABLE 0
#endif

/// `HX_KIB` - A KiB is 1024 bytes.
#define HX_KIB (1u << 10)

/// `HX_MIB` - A MiB is 1,048,576 bytes.
#define HX_MIB (1u << 20)

#if !defined HX_MEMORY_BUDGET_PERMANENT
/// `HX_MEMORY_BUDGET_PERMANENT` - Pool sizes. Set to 5 KiB if not defined.
#define HX_MEMORY_BUDGET_PERMANENT		(5u * HX_KIB)
#endif

#if !defined HX_MEMORY_BUDGET_TEMPORARY_STACK
/// `HX_MEMORY_BUDGET_TEMPORARY_STACK` - Set to 1 MiB if not defined.
#define HX_MEMORY_BUDGET_TEMPORARY_STACK  (1u * HX_MIB)
#endif

#if !defined HX_PROFILE
/// `HX_PROFILE`
/// - `0` Disables code for capturing profiling data.
/// - `1` Compiles in code. See hxprofile_scope().
#define HX_PROFILE (HX_RELEASE) < 2
#endif

#if !defined HX_PROFILER_MAX_RECORDS
/// `HX_PROFILER_MAX_RECORDS` - Set to 4096 if not defined. The profiler doesn't
/// reallocate. This is the maximum.
#define HX_PROFILER_MAX_RECORDS 4096
#endif

#if !defined HX_USE_GOOGLE_TEST
/// `HX_USE_GOOGLE_TEST` - In case you need to use Google Test. Defaults to 0.
#define HX_USE_GOOGLE_TEST 0
#endif

#if !defined HX_TEST_ERROR_HANDLING
/// `HX_TEST_ERROR_HANDLING` - Tests that the failure of tests is handled correctly.
/// Set to `0` if not defined. Set by `testerrorhandling.sh` and `coverage.sh`.
#define HX_TEST_ERROR_HANDLING 0
#endif

#if !defined HX_RADIX_SORT_BITS
/// `HX_RADIX_SORT_BITS`. Radix sort algorithm configuration parameter. The 8-
/// bit version tries to be memory efficient, the 11-bit version might make
/// sense for large data sets. Set to 8 if not defined (either 8 or 11).
#define HX_RADIX_SORT_BITS 8
#endif

#if !defined HX_RADIX_SORT_MIN_SIZE
/// `HX_RADIX_SORT_MIN_SIZE` - Radix sort uses `hxinsertion_sort` below this size.
/// Set to 8 if not defined.
#define HX_RADIX_SORT_MIN_SIZE 8u
#endif

#if HX_CPLUSPLUS
extern "C" {
#endif

/// hxsettings. Constructed by first call to hxinit() which happens when or
/// before the memory allocator constructs.
struct hxsettings {
	/// Logging level for the application (e.g., verbosity of logs).
	uint8_t log_level;

	/// Allows deallocation of permanent resources at system shut down.
	uint8_t deallocate_permanent;

#if (HX_RELEASE) < 1
	/// Number of asserts to skip, useful for testing assert behavior.
	int asserts_to_be_skipped;
#endif
};

/// `g_hxsettings` - Global class constructed by hxinit().
extern struct hxsettings g_hxsettings;

#if HX_CPLUSPLUS
} // extern "C"
#endif
