#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxArray.h"

#if HX_PROFILE

#if HX_HAS_CPP11_TIME
#include <chrono>
#else
#include <time.h>
#endif

#if HX_HAS_CPP11_THREADS
#include <mutex>
#endif

// ----------------------------------------------------------------------------------

// hxProfileScope declares an RAII-style profiling sample.
// WARNING: A pointer to labelStaticString is kept.
// g_hxProfilerDefaultSamplingCutoff is provided below as a recommended minCycles cutoff.
// Macro signature is: hxProfileScope(const char* labelStaticString, uint32_t minCycles = 0u);
#define hxProfileScope(labelStaticString, ...)  hxProfilerScopeInternal<__VA_ARGS__> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStaticString)

#define hxProfilerInit() g_hxProfiler.init()
#define hxProfilerShutdown() g_hxProfiler.shutdown()
#define hxProfilerLog() g_hxProfiler.log()

// Writes profiling data in a format usable by Chrome's chrome://tracing view.
// Usage: In Chrome go to "chrome://tracing/". Load the generated json file.  Use the W/A/S/D keys.
// Format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
#define hxProfilerWriteToChromeTracing(filename) g_hxProfiler.writeToChromeTracing(filename)

extern const float g_hxProfilerMillisecondsPerCycle;     // Scales cycles to ms.

// ----------------------------------------------------------------------------------

#if HX_HAS_CPP11_TIME
using c_hxProfilerPeriod = std::chrono::high_resolution_clock::period;
extern std::chrono::high_resolution_clock::time_point g_hxStart;

enum { g_hxProfilerDefaultSamplingCutoff = (c_hxProfilerPeriod::den / (c_hxProfilerPeriod::num * 1000000)) };
#else
enum { g_hxProfilerDefaultSamplingCutoff = 1000 }; // ~1 microsecond.
#endif

#if HX_HAS_CPP11_THREADS
// address of s_hxProfilerThreadIdAddress used to uniquely identify thread.
extern thread_local uint8_t s_hxProfilerThreadIdAddress;
extern std::mutex s_hxProfilerMutex;
#endif

template<uint32_t minCycles> class hxProfilerScopeInternal;

class hxProfiler {
public:
	struct Record {
		HX_INLINE Record(uint32_t begin, uint32_t end, const char* label, uint32_t threadId)
			: m_begin(begin), m_end(end), m_label(label), m_threadId(threadId) {
		}
		uint32_t m_begin;
		uint32_t m_end;
		const char* m_label;
		uint32_t m_threadId;
	};

	hxProfiler() : m_isEnabled(false) { };

	void init();
	void shutdown();
	void start();
	void log();
	void writeToChromeTracing(const char* filename);

	// For testing
	HX_INLINE uint32_t recordsSize() { return m_records.size(); }
	HX_INLINE void recordsClear() { m_records.clear(); }
private:
	template<uint32_t minCycles> friend class hxProfilerScopeInternal;
	bool m_isEnabled;
	hxArray<Record> m_records;
};

// Use direct access to an object with static linkage for speed.
extern hxProfiler g_hxProfiler;

template<uint32_t minCycles=0u>
class hxProfilerScopeInternal {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal(const char* labelStaticString)
		: m_label(labelStaticString)
	{
		m_t0 = sampleCycles();
	}

#if HX_HAS_CPP11_THREADS
	HX_INLINE ~hxProfilerScopeInternal() {
		uint32_t t1 = sampleCycles();
		uint32_t delta = (t1 - m_t0);
		hxProfiler& data = g_hxProfiler;
		if (data.m_isEnabled && delta >= minCycles) {
			std::unique_lock<std::mutex> lk(s_hxProfilerMutex);
			if (!data.m_records.full()) {
				::new (data.m_records.emplace_back_unconstructed()) hxProfiler::Record(m_t0, t1, m_label, (uint32_t)(ptrdiff_t)&s_hxProfilerThreadIdAddress);
			}
		}
	}
#else
	// TODO, WARNING: Single threaded implementation.
	HX_INLINE ~hxProfilerScopeInternal() {
		uint32_t t1 = sampleCycles();
		uint32_t delta = t1 - m_t0;
		if (g_hxProfiler.m_isEnabled && !g_hxProfiler.m_records.full() && delta >= minCycles) {
			::new (g_hxProfiler.m_records.emplace_back_unconstructed()) hxProfiler::Record(m_t0, t1, m_label, 0);
		}
	}
#endif // !HX_HAS_CPP11_THREADS

#if HX_HAS_CPP11_TIME
	static uint32_t sampleCycles() {
		return (uint32_t)(std::chrono::high_resolution_clock::now() - g_hxStart).count();
	}
#else
	// TODO: read cycle counter register for target.  This version is a Linux fallback.
	static HX_INLINE uint32_t sampleCycles() {
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (uint32_t)ts.tv_nsec;
	}
#endif // !HX_HAS_CPP11_TIME

private:
	hxProfilerScopeInternal(); // = delete
	hxProfilerScopeInternal(const hxProfilerScopeInternal&); // = delete
	void operator=(const hxProfilerScopeInternal&); // = delete
	const char* m_label;
	uint32_t m_t0;
};

// ----------------------------------------------------------------------------------
#else // !HX_PROFILE
#define hxProfileScope(...) ((void)0)
#define hxProfilerInit() ((void)0)
#define hxProfilerShutdown() ((void)0)
#define hxProfilerLog() ((void)0)
#define hxProfilerWriteToChromeTracing(filename) ((void)0)
#endif
