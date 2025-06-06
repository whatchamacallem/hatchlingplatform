#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// c_hxTimeDefaultTimingCutoff is 0.1 millisecond.
#if HX_USE_CHRONO
#include <chrono>

// this is cycles per millisecond. converts cycles to milliseconds
constexpr float c_hxTimeMillisecondsPerCycle = ((float)std::chrono::high_resolution_clock::period::num * 1.0e+3f)
												/ (float)std::chrono::high_resolution_clock::period::den;

// cutoff in cycles for samples that performed little to no processing.
constexpr size_t c_hxTimeDefaultTimingCutoff = (size_t)(std::chrono::high_resolution_clock::period::den
												/ (std::chrono::high_resolution_clock::period::num * 1.0e+4f));

// internal use only
extern std::chrono::high_resolution_clock::time_point g_hxTimeStart;

// Read cycle counter register.  This version is a Linux fall-back.
static inline size_t hxTimeSampleCycles() {
	return (size_t)(std::chrono::high_resolution_clock::now() - g_hxTimeStart).count();
}

#else

// TODO: This needs to be configured for the target.
#include <time.h>

static const float c_hxTimeMillisecondsPerCycle = 1.0f/1e+9f;
static const size_t c_hxTimeDefaultTimingCutoff = 100000;

static inline size_t hxTimeSampleCycles() {
	timespec ts_;
	clock_gettime(CLOCK_MONOTONIC, &ts_);
	return (size_t)ts_.tv_nsec;
}

#endif
