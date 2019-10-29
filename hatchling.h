#pragma once

#define HATCHLING_H

// hatchling.h.  Hatchling Platform.  See README.txt.
//
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion
//
// Build configuration.  See also hxSettings.h.
//
// HX_RELEASE: 0 is a debug build with all asserts and messages.
//             1 is a release build with critical asserts and verbose warnings.
//               E.g. for CMake's "RelWithDebInfo".
//             2 is a release build with only critical asserts using minimal
//               strings.  E.g. for CMake's "MinSizeRel".
//             3 no asserts or teardown and very minimal logging.
//

#ifndef HX_RELEASE
#ifdef NDEBUG
#define HX_RELEASE 1
#else
#define HX_RELEASE 0
#endif
#endif

// ----------------------------------------------------------------------------

#ifndef __cpp_exceptions
#define _HAS_EXCEPTIONS 0 // For Visual Studio.
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "hxSettings.h"
#include "hxMemoryManager.h"

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

enum hxLogLevel {
    hxLogLevel_Log,     // No automatic newline. May compile out.
    hxLogLevel_Console, // No automatic newline. Always prints.
    hxLogLevel_Warning,
    hxLogLevel_Assert
};

#define hxConsolePrint(...) hxLogHandler(hxLogLevel_Console, __VA_ARGS__)

#if (HX_RELEASE) < 1
#define hxInit() (void)(g_hxIsInit || (hxInitAt(__FILE__, __LINE__), 0))
#define hxLog(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__) // No automatic newline
// Does not evaluate message args unless condition fails.
#define hxAssertMsg(x, ...) (void)(!!(x) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), hxAssertHandler(__FILE__, __LINE__), 0))
#define hxAssert(x) (void)((!!(x)) || (hxLogHandler(hxLogLevel_Assert, HX_QUOTE(x)), hxAssertHandler(__FILE__, __LINE__), 0))
void hxAssertHandler(const char* file, uint32_t line);
#else // !(HX_RELEASE < 1)
#define hxInit() (void)(g_hxIsInit || (hxInitAt(0, 0), 0))
#define hxLog(...) ((void)0)
#define hxAssertMsg(x, ...) ((void)0)
#define hxAssert(x) ((void)0)
HX_ATTR_NORETURN void hxAssertHandler(const char* file, uint32_t line);
#endif

#if (HX_RELEASE) < 2
#define hxLogRelease(...) hxLogHandler(hxLogLevel_Log, __VA_ARGS__) // No automatic newline
#define hxWarn(...) hxLogHandler(hxLogLevel_Warning, __VA_ARGS__)
#define hxWarnCheck(x, ...) (void)(!!(x) || (hxLogHandler(hxLogLevel_Warning, __VA_ARGS__), 0))
#define hxAssertRelease(x, ...) (void)(!!(x) || (hxLogHandler(hxLogLevel_Assert, __VA_ARGS__), hxAssertHandler(__FILE__, __LINE__), 0))
#else // !(HX_RELEASE < 2)
#define hxLogRelease(...) ((void)0)
#define hxWarn(...) ((void)0)
#define hxWarnCheck(x, ...) ((void)0)
#if (HX_RELEASE) < 3
#define hxAssertRelease(x, ...) (void)(!!(x) || (hxAssertHandler(0, 0), 0))
#else
#define hxAssertRelease(x, ...) ((void)0)
#endif
#endif

#define HX_MAX_LINE 280
#define HX_QUOTE_(x) #x // evaluates __LINE__ as a number instead of "__LINE__".
#define HX_QUOTE(x) HX_QUOTE_(x)
#define HX_CONCATENATE_(x, y) x ## y
#define HX_CONCATENATE(x, y) HX_CONCATENATE_(x, y)

// hatchling.cpp
void hxInitAt(const char* file, uint32_t line); // Use hxInit.
void hxLogHandler(enum hxLogLevel level, const char* format, ...) HX_ATTR_FORMAT(2, 3);
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args);
#if (HX_RELEASE) < 3
void hxShutdown(); // Expects all non-debug allocations to be released.
HX_ATTR_NORETURN void hxExit(const char* format, ...);
#endif

// hxCUtils.c
extern int g_hxIsInit;
void hxHexDump(const void* p, uint32_t bytes, const char* label);
void hxFloatDump(const float* ptr, uint32_t count, const char* label);
const char* hxBasename(const char* path);
char* hxStringDuplicate(const char* s, enum hxMemoryManagerId allocatorId /*=hxMemoryManagerId_Heap*/);

// ----------------------------------------------------------------------------
#ifdef __cplusplus
} // extern "C"

static_assert((HX_RELEASE) >= 0 && (HX_RELEASE) <= 3, "HX_RELEASE: [0..3]");

template<typename T> HX_INLINE T hxAbs(T x) { return (x >= (T)0) ? x : ((T)0 - x); }
template<typename T> HX_INLINE T hxMax(T x, T y) { return (x > y) ? x : y; }
template<typename T> HX_INLINE T hxMin(T x, T y) { return (x < y) ? x : y; }
template<typename T> HX_INLINE T hxClamp(T x, T min_, T max_) { return (x <= min_) ? min_ : ((x >= max_) ? max_ : x); }

#endif // defined(__cplusplus)
