#pragma once

// Hatchling Platform. <hx/hatchling.h> requires C99. If compiled as C++98 it
// will include C99 headers. Those headers were only added to C++11. All the
// C++98 compilers allow C99 headers without complaint. In particular this
// codebase relies on stdint.h for fixed width integers which is not in C++98.
// Features from C++17 and a few compiler intrinsics are used when available.
//
// Copyright 2017-2025 Adrian Johnston
// https://github.com/whatchamacallem/HatchlingPlatform

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h> // Requires C99, not just C++98.
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// HATCHLING_VER - Major, minor and patch versions.
#define HATCHLING_VER 0x020120u
// HATCHLING_TAG - Major, minor and patch version tag.
#define HATCHLING_TAG "v2.1.20"

#include <hx/hxSettings.h>
#include <hx/hxMemoryManager.h>
#include <hx/hxStringLiteralHash.h>

#if HX_CPLUSPLUS
extern "C" {
#endif

// Hatchling Platform C and C++ API. Above headers are C and C++ too.

// hxLogLevel - Runtime setting for verbosity of log messages. Independently
// controls what messages are compiled in. See g_hxSettings.logLevel.
enum hxLogLevel {
	hxLogLevel_Log,     // Verbose informative messages. No automatic newline.
	hxLogLevel_Console, // Responses to console commands. No automatic newline.
	hxLogLevel_Warning, // Warnings about serious problems.
	hxLogLevel_Assert   // Reason for abnormal termination or test failure.
};

// hxnull - The null pointer value for a given pointer type represented by the
// numeric constant 0. This header is C. Use nullptr if you prefer it.
#define hxnull 0

// HX_QUOTE - Macro for adding quotation marks. Evaluates __LINE__ as a string
// containing a number instead of as "__LINE__".
// - x: The value to be quoted.
#define HX_QUOTE(x_) HX_QUOTE_(x_)
#define HX_QUOTE_(x_) #x_

// HX_CONCATENATE - Macro for concatenating arguments. Evaluates __LINE__ as a
// number instead of as __LINE__.
// - x: The first value to concatenate.
// - y: The second value to concatenate.
#define HX_CONCATENATE(x_, y_) HX_CONCATENATE_(x_, y_)
#define HX_CONCATENATE_(x_, y_) x_ ## y_

// HX_CONCATENATE_3 - Macro for concatenating 3 arguments.
#define HX_CONCATENATE_3(x_, y_, z_) x_ ## y_ ## z_

// HX_STATIC_ASSERT - Compile-time assertion for HX_RELEASE range.
HX_STATIC_ASSERT((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE: Must be [0..3]");

// hxInit() - Initializes the platform.
#define hxInit() (void)(g_hxIsInit || (hxInitInternal(), 0))

#if (HX_RELEASE) == 0 // debug facilities

// hxLog(...) - Enters formatted messages in the system log. Does not add a newline.
// This is only evaluated when HX_RELEASE == 0.
// - ...: Variadic arguments for the formatted log message.
#define hxLog(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// hxAssertMsg(bool x, ...) - Does not evaluate message args unless condition fails. This is
// only evaluated when HX_RELEASE == 0.
// - x: The condition to evaluate.
// - ...: Variadic arguments for the formatted log message.
#define hxAssertMsg(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_BREAKPOINT())

// hxAssert(bool x) - Logs an error and terminates execution if x is false. This is only
// evaluated when HX_RELEASE == 0.
// - ...: The condition to evaluate.
#define hxAssert(...) (void)(!!(__VA_ARGS__) || (hxLogHandler(hxLogLevel_Assert, HX_QUOTE(__VA_ARGS__)), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_BREAKPOINT())

// hxAssertRelease(bool x, ...) - Logs an error and terminates execution if x is false up to
// release level 2. This is only evaluated when HX_RELEASE < 3.
// - x: The condition to evaluate.
// - ...: Variadic arguments for the formatted log message.
#define hxAssertRelease(x_, ...) (void)(!!(x_) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_BREAKPOINT()))

// Assert handler. Do not call directly, signature changes and then is removed.
HX_NOEXCEPT_INTRINSIC int hxAssertHandler(const char* file_, size_t line_);

#else // HX_RELEASE >= 1
#define hxLog(...) ((void)0)
#define hxAssertMsg(x_, ...) ((void)0)
#define hxAssert(x_) ((void)0)
HX_NOEXCEPT_INTRINSIC HX_NORETURN void hxAssertHandler(uint32_t file_, size_t line_);
#endif

#if (HX_RELEASE) <= 1
// hxLogRelease(...) - Enters formatted messages in the system log up to release
// level 1. No automatic newline. This is only evaluated when HX_RELEASE <= 1.
// - ...: Variadic arguments for the formatted log message.
#define hxLogRelease(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// hxLogConsole(...) - Enters formatted messages in the console system log. This is
// only evaluated when HX_RELEASE <= 1.
// - ...: Variadic arguments for the formatted console log message.
#define hxLogConsole(...) hxLogHandler(hxLogLevel_Console, __VA_ARGS__)

// hxLogWarning(...) - Enters formatted warnings in the system log. This is only
// evaluated when HX_RELEASE <= 1.
// - ...: Variadic arguments for the formatted warning message.
#define hxLogWarning(...) hxLogHandler(hxLogLevel_Warning, __VA_ARGS__)

// hxWarnMsg(bool x, ...) - Enters formatted warnings in the system log when
// x is false. This is only evaluated when HX_RELEASE <= 1.
// - x: The condition to evaluate.
// - ...: Variadic arguments for the formatted warning message.
#define hxWarnMsg(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Warning, __VA_ARGS__), 0))

#else // HX_RELEASE >= 2
#define hxLogRelease(...) ((void)0)
#define hxLogConsole(...) ((void)0)
#define hxLogWarning(...) ((void)0)
#define hxWarnMsg(x_, ...) ((void)0)
#endif

// hxAssertRelease has 4 variations. See above. It is only evaluated when HX_RELEASE < 3.
#if (HX_RELEASE) == 1
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 2
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 3
#define hxAssertRelease(x_, ...) ((void)0)
#endif

// hxInitInternal - Use hxInit instead. It checks g_hxIsInit.
void hxInitInternal(void);

// int g_hxIsInit - Set to true by hxInitInternal.
extern int g_hxIsInit;

// hxShutdown - Terminates service. Releases all resources acquired by the
// platform and confirms all memory allocations have been released. HX_RELEASE < 3.
// Does not clear g_hxIsInit, shutdown is final. Logging and asserts are unaffected.
void hxShutdown(void);

// hxLogHandler - Enters formatted messages in the system log. This is the only
// access to logging when HX_RELEASE > 2.
// - level: The log level (e.g., hxLogLevel_Log, hxLogLevel_Warning).
// - format: A printf-style format string.
// - ...: Additional arguments for the format string.
HX_NOEXCEPT_INTRINSIC void hxLogHandler(enum hxLogLevel level_, const char* format_, ...) HX_ATTR_FORMAT(2, 3);

// hxLogHandlerV - A va_list version of hxLogHandler. This is the only access to
// logging when HX_RELEASE > 2.
// - level: The log level (e.g., hxLogLevel_Log, hxLogLevel_Warning).
// - format: A printf-style format string.
// - args: A va_list containing the arguments for the format string.
HX_NOEXCEPT_INTRINSIC void hxLogHandlerV(enum hxLogLevel level_, const char* format_, va_list args_);

// hxHexDump - Prints an array of bytes formatted in hexadecimal. Additional
// information provided when pretty is non-zero.
// - address: Pointer to the start of the byte array.
// - bytes: The number of bytes to print.
// - pretty: Set non-zero to include extended visualization.
void hxHexDump(const void* address_, size_t bytes_, int pretty_);

// hxFloatDump - Prints an array of floating point values.
// - address: Pointer to the start of the float array.
// - floats: The number of floats to print.
void hxFloatDump(const float* address_, size_t floats_);

// hxBasename - Returns a pointer to those characters following the last '\' or
// '/' character or path if those are not present.
// - path: The file path as a null-terminated string.
const char* hxBasename(const char* path_);

// ----------------------------------------------------------------------------
// C++ utility template API
#if HX_CPLUSPLUS
} // extern "C"

// hxmin - More portable versions of min(), max(), abs() and clamp() using the <
// operator. Returns the minimum value of x and y using a < comparison.
// - x: The first value.
// - y: The second value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxmin(const T_& x_, const T_& y_) { return ((x_) < (y_)) ? (x_) : (y_); }

// hxmax - Returns the maximum value of x and y using a < comparison.
// - x: The first value.
// - y: The second value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxmax(const T_& x_, const T_& y_) { return ((y_) < (x_)) ? (x_) : (y_); }

// hxabs - Returns the absolute value of x using a < comparison.
// - x: The value to compute the absolute value for.
template<typename T_>
HX_CONSTEXPR_FN const T_ hxabs(const T_& x_) { return ((x_) < (T_)0) ? ((T_)0 - (x_)) : (x_); }

// hxclamp - Returns x clamped between the minimum and maximum using <
// comparisons.
// - x: The value to clamp.
// - minimum: The minimum allowable value.
// - maximum: The maximum allowable value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxclamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxAssert(!((maximum_) < (minimum_)));
	return ((x_) < (minimum_)) ? (minimum_) : (((maximum_) < (x_)) ? (maximum_) : (x_));
}

// hxswap - Exchanges the contents of x and y using a temporary.
template<typename T_>
HX_CONSTEXPR_FN void hxswap(T_& x_, T_& y_) {
	T_ t_(x_);
	x_ = y_;
	y_ = t_;
}

#else // !HX_CPLUSPLUS
// ----------------------------------------------------------------------------
// C utility macro API - Does it all backwards in heels.

// hxmin - More portable versions of min(), max(), abs() and clamp() using the <
// operator. Returns the minimum value of x and y using a < comparison.
// - x: The first value.
// - y: The second value.
#define hxmin(x_, y_) ((x_) < (y_) ? (x_) : (y_))

// hxmax - Returns the maximum value of x and y using a < comparison.
// - x: The first value.
// - y: The second value.
#define hxmax(x_, y_) ((y_) < (x_) ? (x_) : (y_))

// hxabs - Returns the absolute value of x using a < comparison.
// - x: The value to compute the absolute value for.
#define hxabs(x_) ((x_) < 0 ? (0 - (x_)) : (x_))

// hxclamp - Returns x clamped between the minimum and maximum using <
// comparisons.
// - x: The value to clamp.
// - minimum: The minimum allowable value.
// - maximum: The maximum allowable value.
#define hxclamp(x_, minimum_, maximum_) \
    ((x_) < (minimum_) ? (minimum_) : ((maximum_) < (x_) ? (maximum_) : (x_)))

// hxswap - Exchanges the contents of x and y using a temporary.
#define hxswap(x_,y_) do { \
	char t_[sizeof(x_) == sizeof(y_) ? (int)sizeof(x_) : -1]; \
	memcpy((t_), &(y_), sizeof(x_)); \
	memcpy(&(y_), &(x_), sizeof(x_)); \
	memcpy(&(x_), (t_), sizeof(x_)); } while(0)
#endif
