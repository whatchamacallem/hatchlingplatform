#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxsettings.h Compiler detection and target specific C++11/C++14
/// polyfill. Use `#if (HX_...)` instead of `#ifdef(HX_...)` for all `HX_`* macros.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead.
#endif

#if !defined HX_RELEASE
#if defined NDEBUG
#define HX_RELEASE 1
#else
/// `HX_RELEASE` - C/C++ optimization level. See the README.md for levels 0..3.
#define HX_RELEASE 0
#endif
#endif

#if defined HX_DOXYGEN_PARSER
/// `HX_CPLUSPLUS` - A version of `__cplusplus` that is defined to `0` when
/// `__cplusplus` is undefined. Allows use in C preprocessor statements without
/// warnings when the compiler is configured to warn about undefined macros.
///
/// -  C++11: 201103L
/// -  C++14: 201402L
/// -  C++17: 201703L
/// -  C++20: 202002L
/// -  C++23: 202302L
#define HX_CPLUSPLUS 202002L // Sets C++ version for Doxygen parser.
#elif defined __cplusplus
#define HX_CPLUSPLUS __cplusplus
#else
#define HX_CPLUSPLUS 0
#endif

#if HX_CPLUSPLUS
extern "C" {
#endif

/// \cond HIDDEN
/// Rename the hxdetail_ namespace using the version number to something like
/// `hx31700_`. Also create an identifier that can be used to cause link errors
/// containing the expected version when linking against old code. This is done
/// to force updates in a binary release channel. This will not
// evaluate the macro `_`.
#define hxversion__(prefix_, x_) prefix_ ## x_ ## _
#define hxversion_(prefix_, x_) hxversion__(prefix_, x_)
#define g_hxinit_ver_ hxversion_(g_hxinit_ver, HATCHLING_VER)
#define hxdetail_ hxversion_(hx, HATCHLING_VER)
/// \endcond

// ----------------------------------------------------------------------------
// Target settings for Doxygen. See the Doxyfile. Run doxygen with no args.
#if defined HX_DOXYGEN_PARSER

/// `HX_USE_THREADS` - `11` indicates C11 threads are in use. `1` is for pthreads
/// and `0` is for no threading.
#define HX_USE_THREADS 11

/// `HX_NO_LIBCXX`: 1 - Set to 1 when libstdc++/libc++ are not present.
/// Indicates the implementation is incompatible with the standard C++ library
/// and may not have a complete C library or an operating system.
#define HX_NO_LIBCXX 0

/// `hxbreakpoint` - Can be conditionally evaluated with the `&&` and `||`
/// operators. Uses intrinsics when available. (E.g., clang's.) Raises `SIGTRAP`
/// when `__builtin_debugtrap` is not available.
#define hxbreakpoint() true

/// `hxrestrict` - A pointer attribute indicating that for the lifetime of that
/// pointer, it will be the sole means of accessing the object(s) it points to.
/// Prevents a write iterator from interfering with a read iterator.
#define hxrestrict

/// `hxattr_allocator` - Mark allocator/deallocator pairs for static analysis.
/// See the gcc manual. Must return non-null as well.
#define hxattr_allocator(...)

/// `hxattr_assume` - Tell the optimizer that `condition` is always true.
/// Similar to C++23 `[[assume(condition)]];`.
#define hxattr_assume(...) (void)0

/// `hxattr_cold` - Optimize a function for size.
#define hxattr_cold

/// `hxattr_hot` - Optimize a function more aggressively. Significantly increases
/// code utilization. Adjust implementation according to needs.
#define hxattr_hot

/// `hxattr_nodiscard` - Indicates the caller should not discard the return value.
#define hxattr_nodiscard

/// `hxattr_noexcept` - Use gcc/clang `nothrow` attribute. Unlike `noexcept`
/// this is undefined when violated.
#define hxattr_noexcept

/// `hxattr_nonnull` - Indicates that a function has args that should not be
/// null. Checked by `UBSan`.
#define hxattr_nonnull(...)

/// `hxattr_noreturn` - Indicates that a function will never return. E.g., by
/// calling `_Exit`.
#define hxattr_noreturn

/// `hxattr_printf` - Indicates to gcc that a function uses `printf`-style
/// formatting so it can type-check the format string.
#define hxattr_printf(...)

/// `hxattr_scanf` - Indicates to gcc that a function uses `scanf`-style
/// formatting so it can type-check the format string.
#define hxattr_scanf(...)

// ----------------------------------------------------------------------------
// Target settings for MSVC. MSVC doesn't support C++'s feature test macros very
// well.
#elif defined _MSC_VER

#if !defined __cpp_exceptions && !defined _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 0
#endif

#if !defined HX_USE_THREADS
#define HX_USE_THREADS 11
#endif

#define HX_NO_LIBCXX 0
#define hxbreakpoint() (__debugbreak(),true)
#define hxrestrict __restrict

// Standard C++ attributes are recommended for Windows instead.
#define hxattr_allocator(...)
#define hxattr_assume(condition_) __assume(condition_)
#define hxattr_cold
#define hxattr_hot
#define hxattr_nodiscard
#define hxattr_noexcept
#define hxattr_nonnull(...)
#define hxattr_noreturn
#define hxattr_printf(...)
#define hxattr_scanf(...)

// ----------------------------------------------------------------------------
// Target settings for clang and gcc. Further compilers will require
// customization.
#else // Assume gcc/clang.

// hxthreads.hpp should work in C++11 with pthread.h. WASM still needs to be
// hooked up. _POSIX_THREADS is the correct way to observe the -pthread compiler
// flag.
#if !defined HX_USE_THREADS
#if defined __has_include && __has_include(<threads.h>)
#define HX_USE_THREADS 11
#elif defined _POSIX_THREADS
#define HX_USE_THREADS 1
#else
#define HX_USE_THREADS 0
#endif
#endif

#if defined __GLIBCXX__ || defined _LIBCPP_VERSION
#define HX_NO_LIBCXX 0
#else
// Always true in C.
#define HX_NO_LIBCXX 1
#endif

// gcc can't set a breakpoint properly.
#if defined __has_builtin && __has_builtin(__builtin_debugtrap)
#define hxbreakpoint() (__builtin_debugtrap(),true)
#else
#define hxbreakpoint() (raise(SIGTRAP),true)
#endif

#define hxrestrict __restrict

// clang can't handle the malloc attributes args.
#if defined __clang__
#define hxattr_allocator(...) \
	__attribute__((returns_nonnull)) __attribute__((warn_unused_result))
#else
#define hxattr_allocator(...) __attribute__((malloc(__VA_ARGS__))) \
	__attribute__((returns_nonnull)) __attribute__((warn_unused_result))
#endif

#if defined(__clang__)
#define hxattr_assume(condition_) __builtin_assume(condition_)
#else
// The solution for gcc may have side effects. Use with caution.
// #define hxattr_assume(condition_) (void)((bool)(condition_) || (__builtin_unreachable(),0))
#define hxattr_assume(...) (void)0
#endif

#define hxattr_cold __attribute__((cold))
#define hxattr_hot __attribute__((hot)) __attribute__((flatten))
#define hxattr_nodiscard __attribute__((warn_unused_result))
#define hxattr_noexcept __attribute__((nothrow))
#define hxattr_nonnull(...)__attribute__((nonnull(__VA_ARGS__)))
#define hxattr_noreturn __attribute__((noreturn))
#define hxattr_printf(pos_, start_) __attribute__((format(printf, pos_, start_)))
#define hxattr_scanf(pos_, start_) __attribute__((format(scanf, pos_, start_)))

#endif // target specific settings

// ----------------------------------------------------------------------------
// Target independent.

/// \cond HIDDEN
// HX_APPEND_COUNTER2_ - Used to generate unique identifiers. This is weird
// because the ## operator happens before macro arg evaluation and both
// happen before general macro evaluation.
#define HX_APPEND_COUNTER2_(x_, y_) x_ ## y_
// This version does evaluate its macro args.
#define HX_APPEND_COUNTER_(x_, y_) HX_APPEND_COUNTER2_(x_, y_)
/// \endcond
/// `HX_APPEND_COUNTER` - Generates a semi-unique identifier by appending a
/// unique number for that translation unit to `x`.
/// - `x` : A C identifier to be made unique.
#define HX_APPEND_COUNTER(x_) HX_APPEND_COUNTER_(x_, __COUNTER__)

#if !defined HX_MAX_LINE
/// `HX_MAX_LINE` - Set to 512. Maximum length for formatted messages printed
/// with this platform. Stack space needs to be available for it.
#define HX_MAX_LINE 512
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
/// - `1` Compiles in code. See `hxprofile_scope`.
#define HX_PROFILE (HX_RELEASE) < 2
#endif

#if !defined HX_PROFILER_MAX_RECORDS
/// `HX_PROFILER_MAX_RECORDS` - Set to `4096` if not defined. The profiler doesn't
/// reallocate. This is the maximum.
#define HX_PROFILER_MAX_RECORDS 4096
#endif

#if !defined HX_USE_GOOGLE_TEST
/// `HX_USE_GOOGLE_TEST` - In case you need to use Google Test. Defaults to `0`.
#define HX_USE_GOOGLE_TEST 0
#endif

#if !defined HX_TEST_ERROR_HANDLING
/// `HX_TEST_ERROR_HANDLING` - Tests that the failure of tests is handled correctly.
/// Set to `0` if not defined. Set by `testerrorhandling.sh` and `coverage.sh`.
#define HX_TEST_ERROR_HANDLING 0
#endif

#if !defined HX_RADIX_SORT_MIN_SIZE
/// `HX_RADIX_SORT_MIN_SIZE` - Radix sort uses `hxinsertion_sort` below this size.
/// Set to `32` if not defined.
#define HX_RADIX_SORT_MIN_SIZE 32u
#endif

/// `hxsettings` - Constructed by first call to `hxinit` which happens when or
/// before the system memory allocators construct.
struct hxsettings {
	/// Logging level for the application (e.g., verbosity of logs).
	uint8_t log_level;

	/// Allows deallocation of permanent resources at system shut down.
	bool deallocate_permanent;

#if (HX_RELEASE) < 1
	/// Number of asserts to skip, useful for testing assert behavior.
	int asserts_to_be_skipped;
#endif
};

/// `g_hxsettings` - Global class constructed by `hxinit`.
extern struct hxsettings g_hxsettings;

/// Internal. Used to reset settings at startup.
void hxsettings_construct(void);

#if HX_CPLUSPLUS
} // extern "C"
#endif
