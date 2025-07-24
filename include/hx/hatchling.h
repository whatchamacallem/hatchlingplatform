#pragma once

// Hatchling Platform. <hx/hatchling.h> requires C99. If compiled as C++98 it
// will include C99 headers. Those headers were only added to C++11. All the
// C++98 compilers allow C99 headers without complaint. In particular this
// codebase relies on stdint.h for fixed width integers which is not in C++98.
// Features from C++17 and a few compiler intrinsics are used when available.
//
// Copyright 2017-2025 Adrian Johnston
// https://github.com/whatchamacallem/HatchlingPlatform

// This is also C99, not just C++98.
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined __STDC_VERSION__ && __STDC_VERSION__ < 202311l
#include <stdbool.h>
#endif

/// `HATCHLING_VER` - Major, minor and patch versions.
#define HATCHLING_VER 0x030103u
/// `HATCHLING_TAG` - Major, minor and patch version tag.
#define HATCHLING_TAG "v3.1.3"

#include <hx/hxsettings.h>
#include <hx/hxmemory_manager.h>
#include <hx/hxstring_literal_hash.h>

#if HX_CPLUSPLUS
extern "C" {
#endif

// Hatchling Platform C and C++ API. Above headers are C and C++ too.

/// `hxloglevel_t` - Runtime setting for verbosity of log messages. Independently
/// controls what messages are compiled in. See `g_hxsettings.log_level`.
enum hxloglevel_t {
	/// Verbose informative messages. No automatic newline.
	hxloglevel_log,
	/// Responses to console commands. No automatic newline.
	hxloglevel_console,
	/// Warnings about serious problems.
	hxloglevel_warning,
	/// Reason for abnormal termination or test failure.
	hxloglevel_assert
};

/// `hxnull` - The null pointer value for a given pointer type represented by the
/// numeric constant `0`. This header is C. Use `nullptr` if you prefer it.
#define hxnull 0

/// `hxstatic_assert` - Compile-time assertion for `HX_RELEASE` range.
hxstatic_assert((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE must be [0..3]");

/// `hxinit()` - Initializes the platform.
#define hxinit() (void)(g_hxisinit || (hxinit_internal(), 0))

#if (HX_RELEASE) == 0 // debug facilities

/// `hxlog(...)` - Enters formatted messages in the system log. Does not add a newline.
/// This is only evaluated when `HX_RELEASE == 0`.
/// - `...` Variadic arguments for the formatted log message.
#define hxlog(...) hxloghandler(hxloglevel_log, __VA_ARGS__)

/// `hxassertmsg(bool x, ...)` - Does not evaluate message args unless condition fails. This is
/// only evaluated when `HX_RELEASE == 0`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted log message.
#define hxassertmsg(x_, ...) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, __VA_ARGS__), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassert(bool x)` - Logs an error and terminates execution if `x` is false. This is only
/// evaluated when `HX_RELEASE == 0`.
/// - `x` : The condition to evaluate.
#define hxassert(x_) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, #x_), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassertrelease(bool x, ...)` - Logs an error and terminates execution if `x` is false
/// up to release level 2. This is only evaluated when `HX_RELEASE < 3`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted log message.
#define hxassertrelease(x_, ...) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, __VA_ARGS__), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// Assert handler. Do not call directly, signature changes and then is removed.
hxnoexcept_intrinsic int hxasserthandler(const char* file_, size_t line_);

#else // HX_RELEASE >= 1
#define hxlog(...) ((void)0)
#define hxassertmsg(x_, ...) ((void)0)
#define hxassert(x_) ((void)0)
hxnoexcept_intrinsic hxnoreturn void hxasserthandler(hxhash_t file_, size_t line_);
#endif

#if (HX_RELEASE) <= 1
/// `hxlogrelease(...)` - Enters formatted messages in the system log up to release
/// level 1. No automatic newline. This is only evaluated when `HX_RELEASE <= 1`.
/// - `...` Variadic arguments for the formatted log message.
#define hxlogrelease(...) hxloghandler(hxloglevel_log, __VA_ARGS__)

/// `hxlogconsole(...)` - Enters formatted messages in the console system log. This is
/// only evaluated when `HX_RELEASE <= 1`.
/// - `...` Variadic arguments for the formatted console log message.
#define hxlogconsole(...) hxloghandler(hxloglevel_console, __VA_ARGS__)

/// `hxlogwarning(...)` - Enters formatted warnings in the system log. This is only
/// evaluated when `HX_RELEASE <= 1`.
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

// hxassertrelease has 4 variations. See above. It is only evaluated when HX_RELEASE < 3.
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
void hxinit_internal(void);

/// `g_hxisinit` - Set to true by `hxinit`.
extern bool g_hxisinit;

/// `hxshutdown` - Terminates service. Releases all resources acquired by the
/// platform and confirms all memory allocations have been released. `HX_RELEASE < 3`.
/// Does not clear `g_hxisinit`, shutdown is final. Logging and asserts are unaffected.
void hxshutdown(void);

/// `hxloghandler` - Enters formatted messages in the system log. This is the only
/// access to logging when `HX_RELEASE > 2`.
/// - `level` : The log level (e.g., hxloglevel_log, hxloglevel_warning).
/// - `format` : A printf-style format string.
/// - `...` Additional arguments for the format string.
hxnoexcept_intrinsic void hxloghandler(enum hxloglevel_t level_, const char* format_, ...) hxattr_format(2, 3);

/// `hxloghandler_v` - A `va_list` version of `hxloghandler`. This is the only access to
/// logging when `HX_RELEASE > 2`.
/// - `level` : The log level (e.g., `hxloglevel_log`, `hxloglevel_warning`).
/// - `format` : A `printf`-style format string.
/// - `args` : A `va_list` containing the arguments for the format string.
hxnoexcept_intrinsic void hxloghandler_v(enum hxloglevel_t level_, const char* format_, va_list args_);

/// `hxhex_dump` - Prints an array of bytes formatted in hexadecimal. Additional
/// information provided when pretty is non-zero.
/// - `address` : Pointer to the start of the byte array.
/// - `bytes` : The number of bytes to print.
/// - `pretty` : Set non-zero to include extended visualization.
void hxhex_dump(const void* address_, size_t bytes_, int pretty_);

/// `hxfloat_dump` - Prints an array of floating point values.
/// - `address` : Pointer to the start of the float array.
/// - `floats` : The number of floats to print.
void hxfloat_dump(const float* address_, size_t floats_);

/// `hxbasename` - Returns a pointer to those characters following the last '\' or
/// '/' character or path if those are not present.
/// - `path` : The file path as a null-terminated string.
const char* hxbasename(const char* path_);

/// ----------------------------------------------------------------------------
/// C++ utility template API

#if HX_CPLUSPLUS
} // extern "C"

#if HX_CPLUSPLUS >= 201103L
/// Converts a `T&` to a `T&&`. Provides C++98 polyfill.
template<typename T_> hxconstexpr_fn T_&& hxmove(T_& x_) { return static_cast<T_&&>(x_); }
#else
template<typename T_> inline T_& hxmove(T_& x_) { return x_; }
#endif

// More portable versions of min, max, abs and clamp using only operator<.

/// `hxmin` - Returns the minimum value of `x` and `y` using a `<` comparison.
/// `operator<`. Returns the minimum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
hxconstexpr_fn const T_& hxmin(const T_& x_, const T_& y_) { return ((x_) < (y_)) ? (x_) : (y_); }

/// `hxmax` - Returns the maximum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
hxconstexpr_fn const T_& hxmax(const T_& x_, const T_& y_) { return ((y_) < (x_)) ? (x_) : (y_); }

/// `hxabs` - Returns the absolute value of `x` using a `<` comparison.
/// - `x` : The value to compute the absolute value for.
template<typename T_>
hxconstexpr_fn const T_ hxabs(const T_& x_) { return ((x_) < (T_)0) ? ((T_)0 - (x_)) : (x_); }

/// `hxclamp` - Returns `x` clamped between the `minimum` and `maximum` using `<`
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
template<typename T_>
hxconstexpr_fn const T_& hxclamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxassertmsg(!(maximum_ < minimum_), "minimum <= maximum");
	return (x_ < minimum_) ? minimum_ : ((maximum_ < x_) ? maximum_ : x_);
}

/// `hxswap` - Exchanges the contents of `x` and `y` using a temporary. If `T`
/// has `T::T(T&&)` or `T::operator=(T&&)` then those will be used.
template<typename T_>
hxconstexpr_fn void hxswap(T_& x_, T_& y_) {
	T_ t_(hxmove<T_>(x_));
	x_ = hxmove<T_>(y_);
	y_ = hxmove<T_>(t_);
}

/// `hxswap` - Exchanges the contents of `x` and `y` using `memcpy` and a stack temporary.
/// This is intended for internal use where it is known to be safe to do so.
template<typename T_>
hxconstexpr_fn void hxswap_mcpy(T_& x_, T_& y_) {
	char t_[sizeof x_ == sizeof y_ ? (int)sizeof x_ : -1];
	::memcpy(t_, &y_, sizeof x_);
	::memcpy(&y_, &x_, sizeof x_);
	::memcpy(&x_, t_, sizeof x_);
}

#else // !HX_CPLUSPLUS
// ----------------------------------------------------------------------------
// C utility macro API - Does it all backwards in heels.

/// `hxmin` - More portable versions of min(), max(), abs() and clamp() using the <
/// operator. Returns the minimum value of x and y using a < comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmin(x_, y_) ((x_) < (y_) ? (x_) : (y_))

/// `hxmax` - Returns the maximum value of x and y using a < comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmax(x_, y_) ((y_) < (x_) ? (x_) : (y_))

/// `hxabs` - Returns the absolute value of x using a < comparison.
/// - `x` : The value to compute the absolute value for.
#define hxabs(x_) ((x_) < 0 ? (0 - (x_)) : (x_))

/// `hxclamp` - Returns x clamped between the minimum and maximum using <
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
#define hxclamp(x_, minimum_, maximum_) \
    ((x_) < (minimum_) ? (minimum_) : ((maximum_) < (x_) ? (maximum_) : (x_)))

/// `hxswap_memcpy` - Exchanges the contents of x and y using a temporary.
#define hxswap_memcpy(x_,y_) do { \
	char t_[sizeof(x_) == sizeof(y_) ? (int)sizeof(x_) : -1]; \
	memcpy((t_), &(y_), sizeof(x_)); \
	memcpy(&(y_), &(x_), sizeof(x_)); \
	memcpy(&(x_), (t_), sizeof(x_)); } while(0)
#endif
