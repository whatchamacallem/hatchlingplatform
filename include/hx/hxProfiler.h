#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxTime.h>
#include <hx/hxArray.h>

#if HX_PROFILE
#include <hx/internal/hxProfilerInternal.h>
#define HX_PROFILE_ONLY_(x_) x_
#else // !HX_PROFILE
#define HX_PROFILE_ONLY_(x_) ((void)0)
#endif

// ----------------------------------------------------------------------------
// hxProfilerInternal_ API
//
// hxProfileScope declares an RAII-style profiling sample.  WARNING: A pointer
// to labelStringLiteral is kept.  c_hxProfilerDefaultSamplingCutoff is provided
// in hxTime.h as a recommended MinCycles cutoff.

// hxProfileScope(const char* labelStringLiteral)
#define hxProfileScope(labelStringLiteral_) \
	HX_PROFILE_ONLY_( hxProfilerScopeInternal_<> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral_) )

// hxProfileScopeMin(const char* labelStringLiteral, size_t minCycles)
#define hxProfileScopeMin(labelStringLiteral_, minCycles_) \
	HX_PROFILE_ONLY_( hxProfilerScopeInternal_<minCycles_> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStringLiteral_) )

// Clears samples and begins sampling.hxTestRunner_::singleton_().
#define hxProfilerStart() HX_PROFILE_ONLY_( g_hxProfiler_.start_() )

// Ends sampling.  Does not clear samples.
#define hxProfilerStop() HX_PROFILE_ONLY_( g_hxProfiler_.stop_() )

// Writes samples to the system log.
#define hxProfilerLog() HX_PROFILE_ONLY_( g_hxProfiler_.log_() )

// filename is a C string representing a writable destination.  Writes profiling
// data in a format usable by Chrome's chrome://tracing view.  Usage: In Chrome
// go to "chrome://tracing/". Load the generated json file.  Use the W/A/S/D keys.
// See http://www.chromium.org/developers/how-tos/trace-event-profiling-tool
#define hxProfilerWriteToChromeTracing(filename_) HX_PROFILE_ONLY_( g_hxProfiler_.writeToChromeTracing_(filename_) )
