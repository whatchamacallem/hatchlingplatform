#pragma once
// Copyright 2019 Adrian Johnston

#include <hx/hatchling.h>

// Stores at least a seconds worth of CPU cycles.  Used when profiling.
typedef size_t hx_cycles_t;

// c_hxTimeDefaultTimingCutoff is 1 microsecond.  
#if HX_USE_CPP11_TIME
#include <chrono>

// converts cycles to milliseconds
constexpr float c_hxTimeMillisecondsPerCycle = ((float)std::chrono::high_resolution_clock::period::num * 1.0e+3f)
												/ (float)std::chrono::high_resolution_clock::period::den;

// cutoff for samples that performed little to no processing.
constexpr hx_cycles_t c_hxTimeDefaultTimingCutoff = (hx_cycles_t)(std::chrono::high_resolution_clock::period::den
												/ (std::chrono::high_resolution_clock::period::num * 1000000));

// internal use only
extern std::chrono::high_resolution_clock::time_point g_hxTimeStart;

// Read cycle counter register.  This version is a Linux fall-back.
HX_INLINE static hx_cycles_t hxTimeSampleCycles() {
	return (hx_cycles_t)(std::chrono::high_resolution_clock::now() - g_hxTimeStart).count();
}

#else

// TODO: This needs to be configured for the target.
#include <time.h>

static const float c_hxTimeMillisecondsPerCycle = 1.0e-6f; // Also 1.e+6 cycles/ms, a 1 GHz chip.

static const hx_cycles_t c_hxTimeDefaultTimingCutoff = 1000;

HX_INLINE static hx_cycles_t hxTimeSampleCycles() {
	timespec ts_;
	clock_gettime(CLOCK_MONOTONIC, &ts_);
	return (hx_cycles_t)ts_.tv_nsec;
}
#endif
