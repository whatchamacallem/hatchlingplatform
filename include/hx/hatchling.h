#pragma once

// Hatchling Platform.  <hx/hatchling.h> is both C and C++.
//
// Copyright 2017-2025 Adrian Johnston
// https://github.com/whatchamacallem/HatchlingPlatform

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Major, minor and patch versions.
#define HATCHLING_VER 0x20119
#define HATCHLING_TAG "v2.1.19"

#include <hx/hxSettings.h>
#include <hx/hxMemoryManager.h>
#include <hx/hxStringLiteralHash.h>

#if HX_CPLUSPLUS
extern "C" {
#endif

// ----------------------------------------------------------------------------
// Hatchling Platform C API

// hxLogLevel: Runtime setting for verbosity of log messages.  Independently
// controls what messages are compiled in.  See g_hxSettings.logLevel.
enum hxLogLevel {
	hxLogLevel_Log,     // Verbose informative messages. No automatic newline.
	hxLogLevel_Console, // Responses to console commands. No automatic newline.
	hxLogLevel_Warning, // Warnings about serious problems.
	hxLogLevel_Assert   // Reason for abnormal termination or test failure.
};

// The null pointer value for a given pointer type represented by the numeric
// constant 0.  This header is C.  Use nullptr if you prefer it.
#define hxnull 0

// Macro for adding quotation marks. Evaluates __LINE__ as a string containing
// a number instead of as "__LINE__".
// Parameters:
// - x_: The value to be quoted.
#define HX_QUOTE(x_) HX_QUOTE_(x_)
#define HX_QUOTE_(x_) #x_

// Macro for concatenating arguments. Evaluates __LINE__ as a number instead
// of as __LINE__.
// Parameters:
// - x_: The first value to concatenate.
// - y_: The second value to concatenate.
#define HX_CONCATENATE(x_, y_) HX_CONCATENATE_(x_, y_)
#define HX_CONCATENATE_(x_, y_) x_ ## y_

// Macro for concatenating 3 arguments.
#define HX_CONCATENATE_3(x_, y_, z_) x_ ## y_ ## z_

HX_STATIC_ASSERT((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE: Must be [0..3]");

// Initializes the platform.
#define hxInit() (void)(g_hxIsInit || (hxInitInternal(), 0))

#if (HX_RELEASE) == 0 // debug facilities

// Enters formatted messages in the system log. Does not add a newline.
// This is only evaluated when HX_RELEASE == 0.
// Parameters:
// - ...: Variadic arguments for the formatted log message.
#define hxLog(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Does not evaluate message args unless condition fails.
// This is only evaluated when HX_RELEASE == 0.
// Parameters:
// - x_: The condition to evaluate.
// - ...: Variadic arguments for the formatted log message.
#define hxAssertMsg(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_DEBUG_BREAK)

// Logs an error and terminates execution if x_ is false.
// This is only evaluated when HX_RELEASE == 0.
// Parameters:
// - x_: The condition to evaluate.
#define hxAssert(x_) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, HX_QUOTE(x_)), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_DEBUG_BREAK)

// Assert handler.  Do not call directly, signature changes and then is removed.
int hxAssertHandler(const char* file_, size_t line_) HX_NOEXCEPT;

// Logs an error and terminates execution if x_ is false up to release level 2.
// This is only evaluated when HX_RELEASE < 3.
// Parameters:
// - x_: The condition to evaluate.
// - ...: Variadic arguments for the formatted log message.
#define hxAssertRelease(x_, ...) (void)(!!(x_) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || HX_DEBUG_BREAK))

#else // HX_RELEASE > 1
#define hxLog(...) ((void)0)
#define hxAssertMsg(x_, ...) ((void)0)
#define hxAssert(x_) ((void)0)
HX_NORETURN void hxAssertHandler(uint32_t file_, size_t line_) HX_NOEXCEPT;
#endif

#if (HX_RELEASE) <= 1
// Enters formatted messages in the system log up to release level 1. No automatic
// newline.
// This is only evaluated when HX_RELEASE <= 1.
// Parameters:
// - ...: Variadic arguments for the formatted log message.
#define hxLogRelease(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Enters formatted messages in the console system log.
// This is only evaluated when HX_RELEASE <= 1.
// Parameters:
// - ...: Variadic arguments for the formatted console log message.
#define hxLogConsole(...) hxLogHandler(hxLogLevel_Console, __VA_ARGS__)

// Enters formatted warnings in the system log.
// This is only evaluated when HX_RELEASE <= 1.
// Parameters:
// - ...: Variadic arguments for the formatted warning message.
#define hxWarn(...) hxLogHandler(hxLogLevel_Warning, __VA_ARGS__)

// Enters formatted warnings in the system log when x_ is false.
// This is only evaluated when HX_RELEASE <= 1.
// Parameters:
// - x_: The condition to evaluate.
// - ...: Variadic arguments for the formatted warning message.
#define hxWarnCheck(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Warning, __VA_ARGS__), 0))

#else // HX_RELEASE >= 2
#define hxLogRelease(...) ((void)0)
#define hxLogConsole(...) ((void)0)
#define hxWarn(...) ((void)0)
#define hxWarnCheck(x_, ...) ((void)0)
#endif

// hxAssertRelease has 4 variations.  See above.
// This is only evaluated when HX_RELEASE <= 2.
#if (HX_RELEASE) == 1
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 2
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#elif (HX_RELEASE) == 3
#define hxAssertRelease(x_, ...) ((void)0) // no asserts at level 3.
#endif

// Use hxInit() instead. It checks g_hxIsInit.
void hxInitInternal(void);

// Set to true by hxInitInternal().
extern int g_hxIsInit;

#if (HX_RELEASE) < 3
// Terminates service.  Releases all resources acquired by the platform and
// confirms all memory allocations have been released. HX_RELEASE < 3.
// Does not clear g_hxIsInit, shutdown is final.
void hxShutdown(void);
#endif

// Enters formatted messages in the system log.
// This is the only access to logging when when HX_RELEASE > 2.
// Parameters:
// - level_: The log level (e.g., hxLogLevel_Log, hxLogLevel_Warning).
// - format_: A printf-style format string.
// - ...: Additional arguments for the format string.
void hxLogHandler(enum hxLogLevel level_, const char* format_, ...) HX_NOEXCEPT HX_ATTR_FORMAT(2, 3);

// A va_list version of hxLogHandler.
// This is the only access to logging when when HX_RELEASE > 2.
// Parameters:
// - level_: The log level (e.g., hxLogLevel_Log, hxLogLevel_Warning).
// - format_: A printf-style format string.
// - args_: A va_list containing the arguments for the format string.
void hxLogHandlerV(enum hxLogLevel level_, const char* format_, va_list args_) HX_NOEXCEPT;

// Prints an array of bytes formatted in hexadecimal. Additional information
// provided when pretty is non-zero.
// Parameters:
// - address_: Pointer to the start of the byte array.
// - bytes_: The number of bytes to print.
// - pretty_: Non-zero to include additional formatting information.
void hxHexDump(const void* address_, size_t bytes_, int pretty_);

// Prints an array of floating point values.
// Parameters:
// - address_: Pointer to the start of the float array.
// - floats_: The number of floats to print.
void hxFloatDump(const float* address_, size_t floats_);

// Returns a pointer to those characters following the last '\' or '/' character
// or path if those are not present.
// Parameters:
// - path_: The file path as a null-terminated string.
const char* hxBasename(const char* path_);

// Calculates a string hash at runtime that is the same as hxStringLiteralHash.
// Parameters:
// - string_: The null-terminated string to hash.
uint32_t hxStringLiteralHashDebug(const char* string_);

#if (HX_RELEASE) < 1
// Prints file name hashes registered with HX_REGISTER_FILENAME_HASH. Use after main().
void hxPrintFileHashes(void) HX_NOEXCEPT;
#else
#define hxPrintFileHashes() ((void)0)
#endif

// ----------------------------------------------------------------------------
#if HX_CPLUSPLUS
} // extern "C"

// More portable versions of min(), max(), abs() and clamp() using the < operator.

// Returns the minimum value of x_ and y_ using a < comparison.
// Parameters:
// - x_: The first value.
// - y_: The second value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxmin(const T_& x_, const T_& y_) { return ((x_) < (y_)) ? (x_) : (y_); }

// Returns the maximum value of x_ and y_ using a < comparison.
// Parameters:
// - x_: The first value.
// - y_: The second value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxmax(const T_& x_, const T_& y_) { return ((y_) < (x_)) ? (x_) : (y_); }

// Returns the absolute value of x_ using a < comparison.
// Parameters:
// - x_: The value to compute the absolute value for.
template<typename T_>
HX_CONSTEXPR_FN const T_ hxabs(const T_& x_) { return ((x_) < (T_)0) ? ((T_)0 - (x_)) : (x_); }

// Returns x_ clamped between the minimum and maximum using < comparisons.
// Parameters:
// - x_: The value to clamp.
// - minimum_: The minimum allowable value.
// - maximum_: The maximum allowable value.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxclamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxAssert(!((maximum_) < (minimum_)));
	return ((x_) < (minimum_)) ? (minimum_) : (((maximum_) < (x_)) ? (maximum_) : (x_));
}

// Exchanges the contents of x_ and y_ using a temporary.
template<typename T_>
HX_CONSTEXPR_FN void hxswap(T_& x_, T_& y_) {
	T_ t_(x_);
	x_ = y_;
	y_ = t_;
}

#else
#define hxmin(x_, y_) ((x_) < (y_) ? (x_) : (y_))
#define hxmax(x_, y_) ((y_) < (x_) ? (x_) : (y_))
#define hxabs(x_) ((x_) < 0 ? (0 - (x_)) : (x_))
#define hxclamp(x_, minimum_, maximum_) \
    ((x_) < (minimum_) ? (minimum_) : ((maximum_) < (x_) ? (maximum_) : (x_)))
#define hxswap(x_,y_) do { \
	 char t_[sizeof(x_) == sizeof(y_) ? (int)sizeof(x_) : -1]; \
	memcpy((t_), &(y_), sizeof(x_)); \
	memcpy(&(y_), &(x_), sizeof(x_)); \
	memcpy(&(x_), (t_), sizeof(x_)); } while(0)
#endif
