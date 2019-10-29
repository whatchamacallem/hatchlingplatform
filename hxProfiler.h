#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxArray.h"

#include <time.h>

// ----------------------------------------------------------------------------------
#if HX_PROFILE

extern const float g_hxProfilerMillisecondsPerCycle;     // Scales cycles to ms.
extern const uint32_t g_hxProfilerDefaultSamplingCutoff; // ~1 microsecond of cycles.

// hxProfileScope declares an RAII-style profiling sample.
// WARNING: A pointer to labelStaticString is kept.
// hxProfilerCyclesPerMicrosecond is provided as a recommended minCycles cutoff.
//
void hxProfileScope(const char* labelStaticString, uint32_t minCycles = 0u); 
#define hxProfileScope(...)  hxProfilerScopeInternal HX_CONCATENATE(hxProfileScope_,__LINE__)(__VA_ARGS__)

#define hxProfilerInit() g_hxProfiler.init()
#define hxProfilerShutdown() g_hxProfiler.shutdown()
#define hxProfilerLog() g_hxProfiler.log()

// Writes profiling data in a format usable by Chrome's chrome://tracing view.
// Usage: In Chrome go to "chrome://tracing/". Load the generated json file.  Use the W/A/S/D keys.
// Format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
#define hxProfilerWriteToChromeTracing(filename) g_hxProfiler.writeToChromeTracing(filename)

// ----------------------------------------------------------------------------------

class hxProfilerScopeInternal;

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
	friend class hxProfilerScopeInternal;
	bool m_isEnabled;
	hxArray<Record> m_records;
};

// Use direct access to an object with static linkage for speed.
extern hxProfiler g_hxProfiler;

class hxProfilerScopeInternal {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal(const char* labelStaticString, uint32_t minCycles = 0u)
		: m_label(labelStaticString), m_minCycles(minCycles)
	{
		m_t0 = sampleCycles();
	}

#if HX_HAS_CPP11_THREADS
	~hxProfilerScopeInternal();
#else
	// TODO, WARNING: Single threaded implementation.
	HX_INLINE ~hxProfilerScopeInternal() {
		uint32_t t1 = sampleCycles();
		uint32_t delta = t1 - m_t0;
		if (g_hxProfiler.m_isEnabled && !g_hxProfiler.m_records.full() && delta >= m_minCycles) {
			::new (g_hxProfiler.m_records.emplace_back_unconstructed()) hxProfiler::Record(m_t0, t1, m_label, 0);
		}
	}
#endif // !HX_HAS_CPP11_THREADS
#if HX_HAS_CPP11_TIME
	static uint32_t sampleCycles();
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
	uint32_t m_minCycles;
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
