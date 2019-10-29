#pragma once
// Copyright 2019 Adrian Johnston

#include <hx/hatchling.h>

#define HX_24_HOURS_IN_MILLISECONDS (1000u*60u*60u*24u)

// hx_milliseconds_t stores 49 days worth of milliseconds.  Used to schedule
// events within the next day or so.
typedef uint32_t hx_milliseconds_t;

// Stores at least a seconds worth of CPU cycles.  Used when profiling.
typedef uint32_t hx_cycles_t;

// c_hxTimeDefaultTimingCutoff is 1 microsecond.  
#if HX_USE_CPP11_TIME
#include <chrono>

constexpr hx_cycles_t c_hxTimeDefaultTimingCutoff = (uint32_t)(std::chrono::high_resolution_clock::period::den
												/ (std::chrono::high_resolution_clock::period::num * 1000000));

constexpr float c_hxTimeMillisecondsPerCycle = ((float)std::chrono::high_resolution_clock::period::num * 1.0e+3f)
												/ (float)std::chrono::high_resolution_clock::period::den;

extern std::chrono::high_resolution_clock::time_point g_hxTimeStart;

HX_INLINE static hx_cycles_t hxTimeSampleCycles() {
	return (hx_cycles_t)(std::chrono::high_resolution_clock::now() - g_hxTimeStart).count();
}

#else
// TODO: Configure for target.  This needs to be configured.
HX_STATIC_ASSERT(__linux__, "TODO: Configure for target.");
#include <time.h>

static const hx_cycles_t c_hxTimeDefaultTimingCutoff = 1000;

// Also 1.e+6 cycles/ms, a 1 GHz chip.
static const float c_hxTimeMillisecondsPerCycle = 1.0e-6f;

// Read cycle counter register.  This version is a Linux fall-back.
HX_INLINE static hx_cycles_t hxTimeSampleCycles() {
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)ts.tv_nsec;
}
#endif
