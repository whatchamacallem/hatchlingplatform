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
#define HATCHLING_VER 0x20102
#define HATCHLING_STR "v2.1.2"

// HX_RELEASE: 0 is a debug build with all asserts and verbose strings.
//             1 is a release build with critical asserts and verbose warnings.
//               E.g. for CMake's "RelWithDebInfo".
//             2 is a release build with only critical asserts using minimal strings.
//               E.g. for CMake's "MinSizeRel".
//             3 no asserts or tear down and very minimal logging.
//
#ifndef HX_RELEASE
#ifdef NDEBUG
#define HX_RELEASE 1
#else
#define HX_RELEASE 0
#endif
#endif

#include <hx/hxSettings.h>
#include <hx/hxMemoryManager.h>
#include <hx/hxStringLiteralHash.h>
#include <hx/printf.h>

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
	hxLogLevel_Warning, // Warnings about serious problems
	hxLogLevel_Assert   // Reason for abnormal termination.
};

// The null pointer value for a given pointer type represented by the numeric
// constant 0.
#define hxnull 0

// Macro for adding quotation marks.  Evaluates __LINE__ as a string containing
// a number instead of as "__LINE__".
#define HX_QUOTE(x) HX_QUOTE_(x)
#define HX_QUOTE_(x) #x

// Macro for concatenating arguments.  Evaluates __LINE__ as a number instead
// of as __LINE__.
#define HX_CONCATENATE(x, y) HX_CONCATENATE_(x, y)
#define HX_CONCATENATE_(x, y) x ## y

HX_STATIC_ASSERT((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE: Must be [0..3]");

#if (HX_RELEASE) < 1
// Initializes the platform.
#define hxInit() (void)(g_hxIsInit || (hxInitAt(__FILE__, __LINE__), 0))

// Enters formatted messages in the system log.  Does not add a newline.
// HX_RELEASE < 1
#define hxLog(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Does not evaluate message args unless condition fails.  HX_RELEASE < 1
#define hxAssertMsg(x, ...) (void)(!!(x) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))

// Logs an error and terminates execution if x is false.  HX_RELEASE < 1
#define hxAssert(x) (void)((!!(x)) || ((hxLogHandler(hxLogLevel_Assert, HX_QUOTE(x)), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))

// Assert handler.  Do not call directly, signature changes and then is removed.
// HX_RELEASE < 3
int hxAssertHandler(const char* file, uint32_t line);

#else // !(HX_RELEASE < 1)
#define hxInit() (void)(g_hxIsInit || (hxInitAt(0, 0), 0))
#define hxLog(...) ((void)0)
#define hxAssertMsg(x, ...) ((void)0)
#define hxAssert(x) ((void)0)
HX_ATTR_NORETURN void hxAssertHandler(uint32_t file, uint32_t line);
#endif

#if (HX_RELEASE) < 2
// Enters formatted messages in the system log up to release level 1.  No automatic
// newline.  HX_RELEASE < 2
#define hxLogRelease(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__)

// Enters formatted messages in the console system log.  HX_RELEASE < 2
#define hxLogConsole(...) hxLogHandler(hxLogLevel_Console, __VA_ARGS__)

// Enters formatted warnings in the system log.  HX_RELEASE < 2
#define hxWarn(...) hxLogHandler(hxLogLevel_Warning, __VA_ARGS__)

// Enters formatted warnings in the system log when x is false.  HX_RELEASE < 2
#define hxWarnCheck(x, ...) (void)(!!(x) || (hxLogHandler(hxLogLevel_Warning, __VA_ARGS__), 0))

#if (HX_RELEASE) < 1
// Logs an error and terminates execution if x is false up to release level 3.
// HX_RELEASE < 4
#define hxAssertRelease(x, ...) (void)(!!(x) || ((hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(__FILE__, __LINE__)) || (HX_DEBUG_BREAK,0)))
#else
#define hxAssertRelease(x, ...) (void)(!!(x) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), \
	hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#endif
#else // !(HX_RELEASE < 2)
#define hxLogRelease(...) ((void)0)
#define hxLogConsole(...) ((void)0)
#define hxWarn(...) ((void)0)
#define hxWarnCheck(x, ...) ((void)0)
#if (HX_RELEASE) < 3
#define hxAssertRelease(x, ...) (void)(!!(x) || (hxAssertHandler(hxStringLiteralHash(__FILE__), __LINE__), 0))
#else
#define hxAssertRelease(x, ...) ((void)0)
#endif
#endif

// Use hxInit() instead.
void hxInitAt(const char* file, uint32_t line);

// Set to true by hxInit().
extern int g_hxIsInit;

#if (HX_RELEASE) < 3
// Terminates service.  Releases all resources acquired by the platform and
// confirms all memory allocations have been released. HX_RELEASE < 3.
// Does not clear g_hxIsInit, shutdown is final.
void hxShutdown();

// Stops execution with a formatted message.  HX_RELEASE < 3.
HX_ATTR_NORETURN void hxExit(const char* format, ...);
#endif

// Enters formatted messages in the system log.
void hxLogHandler(enum hxLogLevel level, const char* format, ...) HX_ATTR_FORMAT(2, 3);

// va_list version of hxLogHandler.
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args);

// Prints an array of bytes formatted in hexadecimal.  Additional information
// provided when pretty is non-zero.
void hxHexDump(const void* address, uint32_t bytes, int pretty);

// Prints an array of floating point values.
void hxFloatDump(const float* address, uint32_t floats);

// Returns a pointer to those characters following the last '\' or '/' character
// or path if those are not present.
const char* hxBasename(const char* path);

// Calculates a string hash at runtime that is the same as hxStringLiteralHash.
uint32_t hxStringLiteralHashDebug(const char* string);

// ----------------------------------------------------------------------------
#if __cplusplus
} // extern "C"

// HX_THREAD_LOCAL.  A version of thread_local that compiles out when there is
// no threading.
#if HX_USE_CPP11_THREADS
#define HX_THREAD_LOCAL thread_local
#else
#define HX_THREAD_LOCAL // single threaded operation can ignore thread_local
#endif

// More portable versions of min(), max(), abs() and clamp().

// Returns the absolute value of x using a < comparison.
template<typename T> HX_INLINE HX_CONSTEXPR const T hxAbs(const T& x) { return ((T)0 < x) ? x : ((T)0 - x); }

// Returns the maximum value of x and y using a < comparison.
template<typename T> HX_INLINE HX_CONSTEXPR const T& hxMax(const T& x, const T& y) { return (y < x) ? x : y; }

// Returns the minimum value of x and y using a < comparison.
template<typename T> HX_INLINE HX_CONSTEXPR const T& hxMin(const T& x, const T& y) { return (x < y) ? x : y; }

// Returns x clamped between the minimum and maximum using < comparisons.
template<typename T> HX_INLINE HX_CONSTEXPR const T& hxClamp(const T& x, const T& minimum, const T& maximum) {
	hxAssert(minimum <= maximum);
	return (x < minimum) ? minimum : ((maximum < x) ? maximum : x);
}

#else
#define hxAbs(x) ((0 < (x)) ? (x) : (0 - (x)))
#define hxMax(x, y) (((y) < (x)) ? (x) : (y))
#define hxMin(x, y) (((x) < (y)) ? (x) : (y))
#define hxClamp(x, minimum, maximum) (((x) < (minimum)) ? (minimum) : (((maximum) < (x)) ? (maximum) : (x)))
#endif
