#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hatchling.h
/// Hatchling Platform. Requires C99. C++11 is minimum. Supports up to C++20.
/// Inclusion on the compiler search path is not required. However, the
/// headers are intended to be included as follows: `#include <hx/hatchling.h>`
///
/// Defines logging macros `hxlog`, `hxlogrelease`, `hxlogconsole`,
/// `hxlogwarning` which vary by `HX_RELEASE` level (0–3) and defines log
/// verbosity { log, console, warning, assert }.
///
/// Assertion macros `hxassert`, `hxassertmsg`, `hxassertrelease` are provided
/// for debugging, active when `HX_RELEASE < 3`. `hxinit` initializes the
/// platform and `hxshutdown` releases resources when `HX_RELEASE < 3`.

#ifndef __STDC_WANT_LIB_EXT1__
/// C Standard, Annex K is not portable. Asserts and `hxattr_nonnull` are used
/// instead.
#define __STDC_WANT_LIB_EXT1__ 0
#endif

// Use minimal C style headers. The std:: namespace may not exist. "You can't
// get there from here."
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined __STDC_VERSION__ && __STDC_VERSION__ < 202311l
#include <stdbool.h>
#endif

/// `int HATCHLING_VER` - Two digit major, minor and patch versions. Odd numbered
/// minor versions are development branches.
#define HATCHLING_VER 31701

/// `HATCHLING_TAG` - Major, minor and patch version tag name. Odd numbered
/// minor versions are development branches and their tags end in `-dev`.
#define HATCHLING_TAG "v3.17.1-dev"

#include "hxsettings.h"
#include "hxmemory_manager.h"
#include "hxstring_literal_hash.h"

#if HX_CPLUSPLUS
extern "C" {
#endif

// Hatchling Platform C and C++ API. Above headers are C and C++ too.

/// `hxloglevel_t` - Runtime setting for verbosity of log messages.
/// Independently controls what messages are compiled in. See
/// `g_hxsettings.log_level`.
enum hxloglevel_t {
	/// Written to `hxout`. Structured output. No automatic newline.
	hxloglevel_log,
	/// Written to `hxerr`. Unstructured informative output including error
	/// messages regarding console commands and `hxtest` results. No automatic
	/// newline. No news is good news.
	hxloglevel_console,
	/// Written to `hxerr`. Warnings about serious problems.
	hxloglevel_warning,
	/// Written to `hxerr`. Reason for abnormal termination or test failure.
	hxloglevel_assert
};

/// `hxnull` - The null pointer value for a given pointer type represented by
/// the numeric constant `0`. The C/C++ language standards explicitly define the
/// meaning of `0` in pointer context as a null pointer of the expected type.
/// However they do not define whether `NULL` is `0` or `((void*)0)`. `hxnull`
/// fills that gap by having an unambiguous type. See `hxnullptr`/`hxnullptr_t` if
/// you need a `std::nullptr_t` replacement.
#define hxnull 0

/// Compile-time assertion for `HX_RELEASE` [0..3] range.
#if (HX_RELEASE) < 0 || (HX_RELEASE) >= 4
#error HX_RELEASE must be [0..3].
#endif

/// `hxinit` - Initializes the platform. Designed to trigger a link error when
/// used against previous versions. `hxversion_` is renamed to encode the
/// current version. As well, `HATCHLING_VER` is checked against the value
/// compiled into hxinit_internal.
#define hxinit() (void)(g_hxisinit || (hxinit_internal(hxversion_), 0))

#if (HX_RELEASE) == 0 // debug facilities

/// `hxlog(...)` - Enters formatted messages in the system log. Does not add a
/// newline. This is only evaluated when `HX_RELEASE == 0`.
/// - `...` Variadic arguments for the formatted log message.
#define hxlog(...) hxloghandler(hxloglevel_log, __VA_ARGS__)

/// `hxassertmsg(bool x, ...)` - Does not evaluate message args unless condition
/// fails. This is only evaluated when `HX_RELEASE == 0`. Always evaluates to an
/// expression of type `void`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted log message.
#define hxassertmsg(x_, ...) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, __VA_ARGS__), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassert(bool x)` - Logs an error and terminates execution if `x` is false.
/// This is only evaluated when `HX_RELEASE == 0`. Always evaluates to an
/// expression of type `void`.
/// - `x` : The condition to evaluate.
#define hxassert(x_) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, #x_), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassertrelease(bool x, ...)` - Logs an error and terminates execution if
/// `x` is false up to release level 2. This is only evaluated when `HX_RELEASE
/// < 3`. Always evaluates to an expression of type `void`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted log message.
#define hxassertrelease(x_, ...) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, __VA_ARGS__), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// Assert handler. Do not call directly, signature changes and then is removed.
bool hxasserthandler(const char* file_, size_t line_) hxattr_noexcept hxattr_nonnull(1) hxattr_cold;

#else // HX_RELEASE >= 1
#define hxlog(...) ((void)0)
#define hxassertmsg(x_, ...) ((void)0)
#define hxassert(x_) ((void)0)
void hxasserthandler(hxhash_t file_, size_t line_) hxattr_noexcept hxattr_noreturn hxattr_cold;
#endif

#if (HX_RELEASE) <= 1
/// `hxlogrelease(...)` - Enters formatted messages in the system log up to
/// release level 1. No automatic newline. This is only evaluated when
/// `HX_RELEASE <= 1`.
/// - `...` Variadic arguments for the formatted log message.
#define hxlogrelease(...) hxloghandler(hxloglevel_log, __VA_ARGS__)

/// `hxlogconsole(...)` - Enters formatted messages in the console system log.
/// This is only evaluated when `HX_RELEASE <= 1`.
/// - `...` Variadic arguments for the formatted console log message.
#define hxlogconsole(...) hxloghandler(hxloglevel_console, __VA_ARGS__)

/// `hxlogwarning(...)` - Enters formatted warnings in the system log. This is
/// only evaluated when `HX_RELEASE <= 1`.
/// - `...` Variadic arguments for the formatted warning message.
#define hxlogwarning(...) hxloghandler(hxloglevel_warning, __VA_ARGS__)

/// `hxwarnmsg(bool x, ...)` - Enters formatted warnings in the system log when
/// `x` is false. This is only evaluated when `HX_RELEASE <= 1`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted warning message.
#define hxwarnmsg(x_, ...) (void)((bool)(x_) || (hxloghandler(hxloglevel_warning, __VA_ARGS__), 0))

#else // HX_RELEASE >= 2
#define hxlogrelease(...) ((void)0)
#define hxlogconsole(...) ((void)0)
#define hxlogwarning(...) ((void)0)
#define hxwarnmsg(x_, ...) ((void)0)
#endif

// hxassertrelease has 4 variations. See above. It is only evaluated when
// HX_RELEASE < 3.
#if (HX_RELEASE) == 1
#define hxassertrelease(x_, ...) (void)((bool)(x_) || (hxloghandler(hxloglevel_assert, __VA_ARGS__), \
	hxasserthandler(hxstring_literal_hash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 2
#define hxassertrelease(x_, ...) (void)((bool)(x_) \
	|| (hxasserthandler(hxstring_literal_hash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 3
#define hxassertrelease(x_, ...) ((void)0)
#endif

/// `hxinit_internal` - Use `hxinit` instead. It checks `g_hxisinit`.
void hxinit_internal(int version_) hxattr_cold;

/// `g_hxisinit` - Set to true by `hxinit`.
extern bool g_hxisinit;

/// `hxshutdown` - Terminates service. Releases all resources acquired by the
/// platform and confirms all memory allocations have been released. `HX_RELEASE
/// < 3`.
void hxshutdown(void) hxattr_cold;

/// `hxloghandler` - Enters formatted messages in the system log. This is the
/// only access to logging when `HX_RELEASE > 2`.
/// - `level` : The log level (e.g., `hxloglevel_log`, `hxloglevel_warning`).
/// - `format` : A `printf`-style format string.
/// - `...` Additional arguments for the format string.
void hxloghandler(enum hxloglevel_t level_, const char* format_, ...) hxattr_noexcept hxattr_format_printf(2, 3);

/// `hxloghandler_v` - A `va_list` version of `hxloghandler`. This is the only
/// access to logging when `HX_RELEASE > 2`.
/// - `level` : The log level (e.g., `hxloglevel_log`, `hxloglevel_warning`).
/// - `format` : A `printf`-style format string.
/// - `args` : A `va_list` containing the arguments for the format string.
void hxloghandler_v(enum hxloglevel_t level_, const char* format_, va_list args_) hxattr_noexcept hxattr_nonnull(2);

#if HX_CPLUSPLUS
} // extern "C"
#endif
