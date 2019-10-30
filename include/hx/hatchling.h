#pragma once

// Hatchling Platform.  <hx/hatchling.h> is both C and C++.
//
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion
// https://github.com/adrian3git/HatchlingPlatform

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Major, minor and patch versions.
#define HATCHLING_VER 0x20115
#define HATCHLING_TAG "v2.1.15"

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

#include <hx/hxSettings.h>
#include <hx/hxMemoryManager.h>
#include <hx/hxStringLiteralHash.h>
#include <hx/hxprintf.h>

#if __cplusplus
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
// constant 0.
#define hxnull 0

// Macro for adding quotation marks.  Evaluates __LINE__ as a string containing
// a number instead of as "__LINE__".
#define HX_QUOTE(x_) HX_QUOTE_(x_)
#define HX_QUOTE_(x_) #x_

// Macro for concatenating arguments.  Evaluates __LINE__ as a number instead
// of as __LINE__.
#define HX_CONCATENATE(x_, y_) HX_CONCATENATE_(x_, y_)
#define HX_CONCATENATE_(x_, y_) x_ ## y_

HX_STATIC_ASSERT((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE: Must be [0..3]");

#if (HX_RELEASE) < 1
// Initializes the platform.
#define hxInit() (void)(g_hxIsInit || (hxInitAt(__FILE__, __LINE__), 0))

// Enters formatted messages in the system log.  Does not add a newline.
// HX_RELEASE < 1
#define hxLog(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Does not evaluate message args unless condition fails.  HX_RELEASE < 1
#define hxAssertMsg(x_, ...) (void)(!!(x_) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))

// Logs an error and terminates execution if x_ is false.  HX_RELEASE < 1
#define hxAssert(x_) (void)((!!(x_)) || ((hxLogHandler(hxLogLevel_Assert, HX_QUOTE(x_)), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))

// Assert handler.  Do not call directly, signature changes and then is removed.
// HX_RELEASE < 3
int hxAssertHandler(const char* file_, uint32_t line_);

#else // !(HX_RELEASE < 1)
#define hxInit() (void)(g_hxIsInit || (hxInitAt(hxnull, 0), 0))
#define hxLog(...) ((void)0)
#define hxAssertMsg(x_, ...) ((void)0)
#define hxAssert(x_) ((void)0)
HX_ATTR_NORETURN void hxAssertHandler(uint32_t file_, uint32_t line_);
#endif

#if (HX_RELEASE) < 2
// Enters formatted messages in the system log up to release level 1.  No automatic
// newline.  HX_RELEASE < 2
#define hxLogRelease(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Enters formatted messages in the console system log.  HX_RELEASE < 2
#define hxLogConsole(...) hxLogHandler(hxLogLevel_Console, __VA_ARGS__)

// Enters formatted warnings in the system log.  HX_RELEASE < 2
#define hxWarn(...) hxLogHandler(hxLogLevel_Warning, __VA_ARGS__)

// Enters formatted warnings in the system log when x_ is false.  HX_RELEASE < 2
#define hxWarnCheck(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Warning, __VA_ARGS__), 0))

#if (HX_RELEASE) < 1
// Logs an error and terminates execution if x is false up to release level 2.
// HX_RELEASE < 3
#define hxAssertRelease(x_, ...) (void)(!!(x_) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))
#else
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#endif
#else // !(HX_RELEASE < 2)
#define hxLogRelease(...) ((void)0)
#define hxLogConsole(...) ((void)0)
#define hxWarn(...) ((void)0)
#define hxWarnCheck(x_, ...) ((void)0)
#if (HX_RELEASE) < 3
#define hxAssertRelease(x_, ...) (void)(!!(x_) || (hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#else
#define hxAssertRelease(x_, ...) ((void)0)
#endif
#endif

// Use hxInit() instead.
void hxInitAt(const char* file_, uint32_t line_);

// Set to true by hxInit().
extern int g_hxIsInit;

#if (HX_RELEASE) < 3
// Terminates service.  Releases all resources acquired by the platform and
// confirms all memory allocations have been released. HX_RELEASE < 3.
// Does not clear g_hxIsInit, shutdown is final.
void hxShutdown();

// HX_RELEASE < 3.  Stops execution with a formatted message.  Format must end
// with a \n.
HX_ATTR_NORETURN void hxExit(const char* format_, ...);
#endif

// Enters formatted messages in the system log.
void hxLogHandler(enum hxLogLevel level_, const char* format_, ...) HX_ATTR_FORMAT(2, 3);

// va_list version of hxLogHandler.
void hxLogHandlerV(enum hxLogLevel level_, const char* format_, va_list args_);

// Prints an array of bytes formatted in hexadecimal.  Additional information
// provided when pretty is non-zero.
void hxHexDump(const void* address_, uint32_t bytes_, int pretty_);

// Prints an array of floating point values.
void hxFloatDump(const float* address_, uint32_t floats_);

// Returns a pointer to those characters following the last '\' or '/' character
// or path if those are not present.
const char* hxBasename(const char* path_);

// Calculates a string hash at runtime that is the same as hxStringLiteralHash.
uint32_t hxStringLiteralHashDebug(const char* string_);

// ----------------------------------------------------------------------------
#if __cplusplus
} // extern "C"

// More portable versions of min(), max(), abs() and clamp() using the < operator.

// Returns the minimum value of x and y using a < comparison.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxMin(const T_& x_, const T_& y_) { return (x_ < y_) ? x_ : y_; }

// Returns the maximum value of x and y using a < comparison.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxMax(const T_& x_, const T_& y_) { return (y_ < x_) ? x_ : y_; }

// Returns the absolute value of x using a < comparison.
template<typename T_>
HX_CONSTEXPR_FN const T_ hxAbs(const T_& x_) { return (x_ < (T_)0) ? ((T_)0 - x_) : x_; }

// Returns x clamped between the minimum and maximum using < comparisons.
template<typename T_>
HX_CONSTEXPR_FN const T_& hxClamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxAssert(!(maximum_ < minimum_));
	return (x_ < minimum_) ? minimum_ : ((maximum_ < x_) ? maximum_ : x_);
}
#endif
