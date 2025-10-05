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
///
/// Available utilities are: `hxnull`, `hxnullptr`, `hxmove`, `hxforward`,
/// `hxmin`, `hxmax`, `hxabs`, `hxclamp`, `hxswap`, `hxhex_dump`,
/// `hxfloat_dump`.

/// C Standard, Annex K is not portable. Asserts and `hxattr_nonnull` are used
/// instead.
#define __STDC_WANT_LIB_EXT1__ 0

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

/// `HATCHLING_VER` - Two digit major, minor and patch versions. Odd numbered
/// minor versions are development branches. Yes, this is actually that old.
#define HATCHLING_VER 31300l
/// `HATCHLING_TAG` - Major, minor and patch version tag name. Odd numbered
/// minor versions are development branches and their tags end in `-dev`.
#define HATCHLING_TAG "v3.13.0-dev"

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

/// `hxinit` - Initializes the platform.
#define hxinit() (void)(g_hxisinit || (hxinit_internal(), 0))

#if (HX_RELEASE) == 0 // debug facilities

/// `hxlog(...)` - Enters formatted messages in the system log. Does not add a
/// newline. This is only evaluated when `HX_RELEASE == 0`.
/// - `...` Variadic arguments for the formatted log message.
#define hxlog(...) hxloghandler(hxloglevel_log, __VA_ARGS__)

/// `hxassertmsg(bool x, ...)` - Does not evaluate message args unless condition
/// fails. This is only evaluated when `HX_RELEASE == 0`.
/// - `x` : The condition to evaluate.
/// - `...` Variadic arguments for the formatted log message.
#define hxassertmsg(x_, ...) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, __VA_ARGS__), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassert(bool x)` - Logs an error and terminates execution if `x` is false.
/// This is only evaluated when `HX_RELEASE == 0`.
/// - `x` : The condition to evaluate.
#define hxassert(x_) (void)((bool)(x_) \
	|| (hxloghandler(hxloglevel_assert, #x_), hxasserthandler(__FILE__, __LINE__)) \
	|| hxbreakpoint())

/// `hxassertrelease(bool x, ...)` - Logs an error and terminates execution if
/// `x` is false up to release level 2. This is only evaluated when `HX_RELEASE
/// < 3`.
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
void hxinit_internal(void) hxattr_cold;

/// `g_hxisinit` - Set to true by `hxinit`.
extern bool g_hxisinit;

/// `hxshutdown` - Terminates service. Releases all resources acquired by the
/// platform and confirms all memory allocations have been released. `HX_RELEASE
/// < 3`. Does not clear `g_hxisinit`, shutdown is final. Logging and asserts
/// are unaffected.
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

/// `hxhex_dump` - Prints an array of bytes formatted in hexadecimal. Additional
/// information provided when pretty is non-zero.
/// - `address` : Pointer to the start of the byte array.
/// - `bytes` : The number of bytes to print.
/// - `pretty` : Set non-zero to include extended visualization.
void hxhex_dump(const void* address_, size_t bytes_, int pretty_) hxattr_nonnull(1) hxattr_cold;

/// `hxfloat_dump` - Prints an array of floating point values.
/// - `address` : Pointer to the start of the float array.
/// - `floats` : The number of floats to print.
void hxfloat_dump(const float* address_, size_t floats_) hxattr_nonnull(1) hxattr_cold;

/// `hxbasename` - Returns a pointer to those characters following the last `\` or
/// `/` character or path if those are not present.
/// - `path` : The file path as a null-terminated string.
const char* hxbasename(const char* path_) hxattr_nonnull(1);

/// Implements standard `isgraph` for a locale where all non-ASCII characters
/// are considered graphical or mark making. This is compatable with scanf-style
/// parsing of UTF-8 string parameters. However, this is not `en_US.UTF-8` or
/// the default C locale.
inline bool hxisgraph(char ch_) {
	return ((unsigned char)ch_ - 0x21u) < 0x5Eu
		|| ((unsigned char)ch_ & 0x80u);
}

/// Implements standard `isspace` for a locale where all non-ASCII characters
/// are considered graphical or mark making. Returns nonzero for space and
/// `\t \n \v \f \r`. This is compatable with scanf-style parsing of
/// UTF-8 string parameters. However, this is not `en_US.UTF-8` or the default
/// C locale.
inline bool hxisspace(char ch_) {
	return ch_ == ' ' || ((unsigned char)ch_ - 0x09u) < 0x05u;
}

/// Returns `log2(n)` as an integer which is the power of 2 of the largest bit
/// in `n`. NOTA BENE: `hxlog2i(0)` is currently -127 and is undefined.
/// - `i` : A `size_t`.
inline int hxlog2i(size_t i_) {
	// Use the floating point hardware because this isn't important enough for
	// intrinsics.
	float f_ = (float)i_;
    uint32_t bits_;
    memcpy(&bits_, &f_, sizeof(float));
    return (int)((bits_ >> 23) & 0xffu) - 127;
}

/// Returns true if `x` is finite (not NaN or ±inf).
inline int hxisfinitef(float x_) {
    uint32_t u_;
    memcpy(&u_, &x_, sizeof u_);
    // Finite iff exponent != all 1s (0xFF)
    return (u_ & 0x7F800000u) != 0x7F800000u;
}

/// Returns true if `x` is finite (not NaN or ±inf).
inline int hxisfinitel(double x_) {
    uint64_t u_;
    memcpy(&u_, &x_, sizeof u_);
    // Finite iff exponent != all 1s (0x7FF)
    return (u_ & 0x7FF0000000000000ull) != 0x7FF0000000000000ull;
}

// ----------------------------------------------------------------------------
// C++ Template Utility API

#if HX_CPLUSPLUS
} // extern "C"

/// `hxnullptr_t` - A class that will only convert to a null `T` pointer. Useful
/// when an integer constant arg would be ambiguous or otherwise break template
/// code. `hxnullptr` is a `hxnullptr_t`. Use plain `hxnull` for comparisons.
class hxnullptr_t {
public:
	/// Null `T` pointer.
	template<typename T_> constexpr operator T_*() const { return 0; }
	/// Null `T` member function pointer.
	template<typename T_, typename M_> constexpr operator M_ T_::*() const { return 0; }
	/// No address-of operator.
	void operator&() const = delete;
};

/// `hxnullptr` - An instance of a class that will only convert to a null
/// pointer. Useful when an integer constant arg would be ambiguous or otherwise
/// break template code. `hxnullptr` is a `hxnullptr_t`. Use plain `hxnull` for
/// comparisons.
#define hxnullptr hxnullptr_t()

/// Internal. Implements `std::enable_if`. This is used instead of the `requires`
/// keyword when backwards compatibility is required. Used by `hxenable_if_t.`
template<bool condition_, typename type_=void> struct hxenable_if { };
template<typename type_> struct hxenable_if<true, type_> { using type = type_; };

/// hxenable_if_t<condition> - Implements `std::enable_if_t`. This is used
/// instead of the `requires` keyword when backwards compatibility is required.
template<bool condition_, typename type_=void>
using hxenable_if_t = typename hxenable_if<condition_, type_>::type;

/// Internal. Returns `T` with references removed as
/// `hxremove_reference<T>::type`. Used by `hxremove_reference_t`.
template<class T_> struct hxremove_reference       { using type = T_; };
template<class T_> struct hxremove_reference<T_&>  { using type = T_; };
template<class T_> struct hxremove_reference<T_&&> { using type = T_; };

/// hxremove_reference_t<T> - Returns `T` with references removed.
template<class T_> using hxremove_reference_t = typename hxremove_reference<T_>::type;

/// Internal. Implements `std::is_lvalue_reference`.
template<typename T_> struct hxis_lvalue_reference { enum { value = 0 }; };
template<typename T_> struct hxis_lvalue_reference<T_&> { enum { value = 1 }; };

/// Implements `std::move`. Converts either a `T&` or a `T&&` to a `T&&`. Do not
/// specify `T` explicitly as it will not work as expected. This uses the rules
/// about reference collapsing to handle both `T&` and `T&&`.
template<class T_>
constexpr hxremove_reference_t<T_>&& hxmove(T_&& t_) {
	return static_cast<hxremove_reference_t<T_>&&>(t_);
}

/// Implements `std::forward`. Call as `hxforward<T>(x)` **from inside a
/// templated forwarding function** where the parameter was declared `T&&` and
/// `T` was **deduced**. `T` must be explicitly specified. E.g.,
/// ```cpp
///   template<class T>
///   void forwards_temp(T&&x) { requires_temp(hxforward<T>(x)); }
/// ```
/// This is the `T&&` version of hxforward<T>.
template<class T_>
constexpr T_&& hxforward(typename hxremove_reference<T_>::type&& t) noexcept {
	static_assert(!hxis_lvalue_reference<T_>::value, "T must be a `T&&` reference.");
	return static_cast<T_&&>(t);
}

/// This is the `T&` version of hxforward<T>. It gets invoked when `T` turns out
/// to be an l-value. This happens then a `T&` is passed as a `T&&`.
template<class T_>
constexpr T_&& hxforward(typename hxremove_reference<T_>::type& t) noexcept {
	return static_cast<T_&&>(t);
}

/// `hxmin` - Returns the minimum value of `x` and `y` using a `<` comparison.
/// `operator<`. Returns the minimum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
constexpr const T_& hxmin(const T_& x_, const T_& y_) { return ((x_) < (y_)) ? (x_) : (y_); }

/// `hxmax` - Returns the maximum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
constexpr const T_& hxmax(const T_& x_, const T_& y_) { return ((y_) < (x_)) ? (x_) : (y_); }

/// `hxabs` - Returns the absolute value of `x` using a `<` comparison.
/// - `x` : The value to compute the absolute value for.
template<typename T_>
constexpr const T_ hxabs(const T_& x_) { return ((x_) < (T_)0) ? ((T_)0 - (x_)) : (x_); }

/// `hxclamp` - Returns `x` clamped between the `minimum` and `maximum` using `<`
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
template<typename T_>
constexpr const T_& hxclamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxassertmsg(!(maximum_ < minimum_), "minimum <= maximum");
	return (x_ < minimum_) ? minimum_ : ((maximum_ < x_) ? maximum_ : x_);
}

/// `hxswap` - Exchanges the contents of `x` and `y` using a temporary. If `T`
/// has `T::T(T&&)` or `T::operator=(T&&)` then those will be used.
template<typename T_>
constexpr void hxswap(T_& x_, T_& y_) {
	hxassertmsg(&x_ != &y_, "hxswap No swapping with self.");
	T_ t_(hxmove(x_));
	x_ = hxmove(y_);
	y_ = hxmove(t_);
}

/// `hxswap_memcpy` - Exchanges the contents of `x` and `y` using `memcpy` and a
/// stack temporary. This is intended for internal use where it is known to be
/// safe to do so. It is a cheap way to implement `T::operator=(T&&)`.
/// - `x` : First `T&`.
/// - `y` : Second `T&`.
template<typename T_>
constexpr void hxswap_memcpy(T_& x_, T_& y_) {
	char t_[sizeof x_];
	::memcpy(t_, &y_, sizeof x_);
	::memcpy((void*)&y_, &x_, sizeof x_);
	::memcpy((void*)&x_, t_, sizeof x_);
}

#else // !HX_CPLUSPLUS
// ----------------------------------------------------------------------------
// C Macro Utility API - Does it all backwards in heels.

/// `hxmin` - Returns the minimum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmin(x_, y_) ((x_) < (y_) ? (x_) : (y_))

/// `hxmax` - Returns the maximum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmax(x_, y_) ((y_) < (x_) ? (x_) : (y_))

/// `hxabs` - Returns the absolute value of `x` using a `<` comparison.
/// - `x` : The value to compute the absolute value for.
#define hxabs(x_) ((x_) < 0 ? (0 - (x_)) : (x_))

/// `hxclamp` - Returns `x` clamped between the `minimum` and `maximum` using `<`
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
#define hxclamp(x_, minimum_, maximum_) \
	((x_) < (minimum_) ? (minimum_) : ((maximum_) < (x_) ? (maximum_) : (x_)))

/// `hxswap_memcpy` - Exchanges the contents of `x` and `y` using `memcpy` and a
/// stack temporary. This is intended for internal use where it is known to be
/// safe to do so.
/// - `x` : First object.
/// - `y` : Second object.
#define hxswap_memcpy(x_,y_) do { \
	char t_[sizeof(x_) == sizeof(y_) ? (int)sizeof(x_) : -1]; \
	memcpy(t_, &(y_), sizeof(x_)); \
	memcpy(&(y_), &(x_), sizeof(x_)); \
	memcpy(&(x_), t_, sizeof(x_)); } while(0)

#endif // !HX_CPLUSPLUS
