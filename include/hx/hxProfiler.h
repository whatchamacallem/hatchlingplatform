#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxStockpile.h>
#include <hx/hxTime.h>

#if HX_PROFILE
#include <hx/internal/hxProfilerInternal.h>
#define HX_PROFILE_FN(x) x
#else // !HX_PROFILE
#define HX_PROFILE_FN(x) ((void)0)
#endif

// ----------------------------------------------------------------------------
// hxProfiler API
//
// hxProfileScope declares an RAII-style profiling sample.  WARNING: A pointer
// to labelStringLiteral is kept.  c_hxProfilerDefaultSamplingCutoff is provided
// below as a recommended MinCycles cutoff.

// hxProfileScope(const char* labelStringLiteral)
#define hxProfileScope(labelStringLiteral) \
	HX_PROFILE_FN( hxProfilerScopeInternal<> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral) )

// hxProfileScopeMin(const char* labelStringLiteral, uint32_t minCycles)
#define hxProfileScopeMin(labelStringLiteral, minCycles) \
	HX_PROFILE_FN( hxProfilerScopeInternal<minCycles> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral) )

// Clears samples and begins sampling.
#define hxProfilerStart() HX_PROFILE_FN( g_hxProfiler.start() )

// Ends sampling.  Does not clear samples.
#define hxProfilerStop() HX_PROFILE_FN( g_hxProfiler.stop() )

// Writes samples to the system log.
#define hxProfilerLog() HX_PROFILE_FN( g_hxProfiler.log() )

// filename is a C string representing a writable destination.  Writes profiling
// data in a format usable by Chrome's chrome://tracing view.  Usage: In Chrome
// go to "chrome://tracing/". Load the generated json file.  Use the W/A/S/D keys.
// See http://www.chromium.org/developers/how-tos/trace-event-profiling-tool
#define hxProfilerWriteToChromeTracing(filename) HX_PROFILE_FN( g_hxProfiler.writeToChromeTracing(filename) )
