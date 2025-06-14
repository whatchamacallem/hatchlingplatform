#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

#if defined(__EMSCRIPTEN__)
// from "emscripten/emscripten.h"
extern "C" double emscripten_get_now(void);
#elif defined(__x86_64__) || defined(__i386__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

// hxcycles_t - Stores approx. 3 seconds to 300 years worth of processor cycles
// starting from an unspecified origin and wrapping using unsigned rules. This is
// intended for profiling, not calendaring.
typedef size_t hxcycles_t;

#if HX_PROFILE
#include <hx/internal/hxProfilerInternal.hpp>
#define HX_PROFILE_ONLY_(x_) x_
#else
#define HX_PROFILE_ONLY_(x_) ((void)0)
#endif

// c_hxCyclesPerSecond - TODO: customize for your processor speed. This assumes
// 2ghz. These are really only used with printf which promotes everything to
// double anyhow.
static const double c_hxCyclesPerSecond = 2.0e+9;
static const double c_hxMillisecondsPerCycle = 1.0e+3 / c_hxCyclesPerSecond;
static const double c_hxMicrosecondsPerCycle = 1.0e+6 / c_hxCyclesPerSecond;
static const hxcycles_t c_hxDefaultCyclesCutoff = 1000;

// hxTimeSampleCycles() - Set up the processor cycle counter for your
// architecture. This is callable without enabling HX_PROFILE.
static inline hxcycles_t hxTimeSampleCycles(void) {
    uint64_t cycles_ = 0; (void)cycles_;
#if defined(__EMSCRIPTEN__)
    double t_ = emscripten_get_now() * 1.0e+6;
    cycles_ = (uint64_t)t_;
#elif defined(__x86_64__) || defined(__i386__)
    cycles_ = __rdtsc();
#elif defined(__aarch64__)  // ARMv8-A 64-bit.
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles_));
#elif defined(__arm__)  // ARMv7-A 32-bit.
    uint32_t t_;
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(t_));
    cycles_ = (uint64_t)t_;
#elif defined(__riscv) && (__riscv_xlen == 64)
    __asm__ volatile("rdcycle %0" : "=r"(cycles_));
#elif defined(__powerpc__) || defined(__ppc__)
    __asm__ volatile("mftb %0" : "=r"(cycles_));
#else
HX_STATIC_ASSERT(0, "implement hxTimeSampleCycles");
#endif
    return (hxcycles_t)cycles_;
}

// hxProfileScope(const char* labelStringLiteral) - Declares an RAII-style
// profiling sample. WARNING: A pointer to labelStringLiteral is kept.
// c_hxProfilerDefaultSamplingCutoff is provided as a recommended MinCycles
// cutoff. Compiles to a NOP when not in use.
// - labelStringLiteral: A string literal label for the sample.
#define hxProfileScope(labelStringLiteral_) \
    HX_PROFILE_ONLY_( hxProfilerScopeInternal_<> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral_) )

// hxProfileScopeMin(const char* labelStringLiteral, hxcycles_t minCycles) -
// Declares an RAII-style profiling sample with a minimum cycle cutoff. Compiles
// to a NOP when not in use.
// - labelStringLiteral: A string literal label for the sample.
// - minCycles_: A minimum number of cycles required for a sample to be recorded.
#define hxProfileScopeMin(labelStringLiteral_, minCycles_) \
    HX_PROFILE_ONLY_( hxProfilerScopeInternal_<minCycles_> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral_) )

// hxProfilerBegin() - Clears samples and begins sampling. Compiles to a NOP when
// not in use.
#define hxProfilerBegin() HX_PROFILE_ONLY_( g_hxProfiler_.start_() )

// hxProfilerEnd() - Ends sampling. Does not clear samples. Compiles to a NOP
// when not in use.
#define hxProfilerEnd() HX_PROFILE_ONLY_( g_hxProfiler_.stop_() )

// hxProfilerLog() - Stops sampling and writes samples to the system log.
// Compiles to a NOP when not in use.
#define hxProfilerLog() HX_PROFILE_ONLY_( g_hxProfiler_.log_() )

// hxProfilerWriteToChromeTracing(const char* filename) - Stops sampling and
// writes samples to the provided file. Writes profiling data in a format usable
// by Chrome's chrome://tracing view. Usage: In Chrome go to "chrome://tracing/".
// Load the generated json file. Use the W/A/S/D keys. See
// http://www.chromium.org/developers/how-tos/trace-event-profiling-tool
// Compiles to a NOP when not in use.
#define hxProfilerWriteToChromeTracing(filename_) \
    HX_PROFILE_ONLY_( g_hxProfiler_.writeToChromeTracing_(filename_) )
