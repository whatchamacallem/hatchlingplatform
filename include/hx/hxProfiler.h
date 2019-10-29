#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxStockpile.h>

#if HX_PROFILE
#include <hx/internal/hxProfilerInternal.h>

// ----------------------------------------------------------------------------
// hxProfiler API
//
// hxProfileScope declares an RAII-style profiling sample.  WARNING: A pointer
// to labelStringLiteral is kept.  c_hxProfilerDefaultSamplingCutoff is provided
// below as a recommended MinCycles cutoff.

// hxProfileScope(const char* labelStringLiteral)
#define hxProfileScope(labelStringLiteral) \
	hxProfilerScopeInternal<> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral)

// hxProfileScopeMin(const char* labelStringLiteral, uint32_t minCycles)
#define hxProfileScopeMin(labelStringLiteral, min) \
	hxProfilerScopeInternal<min> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral)

// Clears samples and begins sampling.
#define hxProfilerStart() g_hxProfiler.start()

// Ends sampling.  Does not clear samples.
#define hxProfilerStop() g_hxProfiler.stop()

// Writes samples to the log.
#define hxProfilerLog() g_hxProfiler.log()

// Writes profiling data in a format usable by Chrome's chrome://tracing view.
// Usage: In Chrome go to "chrome://tracing/". Load the generated json file.  Use
// the W/A/S/D keys.  Format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
#define hxProfilerWriteToChromeTracing(filename) g_hxProfiler.writeToChromeTracing(filename)

// c_hxProfilerDefaultSamplingCutoff is 1 microsecond.
#if HX_USE_CPP11_TIME
static constexpr uint32_t c_hxProfilerDefaultSamplingCutoff =
	(uint32_t)(std::chrono::high_resolution_clock::period::den / (std::chrono::high_resolution_clock::period::num * 1000000));
#else
enum { c_hxProfilerDefaultSamplingCutoff = 1000 };
#endif

// ----------------------------------------------------------------------------
#else // !HX_PROFILE
#define hxProfileScope(...) ((void)0)
#define hxProfileScopeMin(...) ((void)0)
#define hxProfilerStart() ((void)0)
#define hxProfilerStop() ((void)0)
#define hxProfilerLog() ((void)0)
#define hxProfilerWriteToChromeTracing(filename) ((void)0)
#endif
