#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxStockpile.h"

#if HX_PROFILE

#if HX_HAS_CPP11_TIME
#include <chrono>
#else
#include <time.h>
#endif

// ----------------------------------------------------------------------------------
// hxProfiler API

// hxProfileScope declares an RAII-style profiling sample.
// WARNING: A pointer to labelStaticString is kept.
// c_hxProfilerDefaultSamplingCutoff is provided below as a recommended MinCycles cutoff.
// Macro signatures are: hxProfileScope(const char* labelStaticString) and
// hxProfileScopeMin(const char* labelStaticString, uint32_t MinCycles = 0u);
#define hxProfileScope(labelStaticString)  \
	hxProfilerScopeInternal<> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStaticString)

#define hxProfileScopeMin(labelStaticString, Min)  \
	hxProfilerScopeInternal<Min> HX_CONCATENATE(hxProfileScope_,__LINE__)(labelStaticString)

#define hxProfilerStart() g_hxProfiler.start()
#define hxProfilerStop() g_hxProfiler.stop()
#define hxProfilerLog() g_hxProfiler.log()

// Writes profiling data in a format usable by Chrome's chrome://tracing view.
// Usage: In Chrome go to "chrome://tracing/". Load the generated json file.  Use the W/A/S/D keys.
// Format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
#define hxProfilerWriteToChromeTracing(filename) g_hxProfiler.writeToChromeTracing(filename)

extern float g_hxProfilerMillisecondsPerCycle;     // Scales cycles to ms.

// c_hxProfilerDefaultSamplingCutoff is 1 microsecond.
#if HX_HAS_CPP11_TIME
static constexpr uint32_t c_hxProfilerDefaultSamplingCutoff =
	(uint32_t)(std::chrono::high_resolution_clock::period::den / (std::chrono::high_resolution_clock::period::num * 1000000));
#else
enum { c_hxProfilerDefaultSamplingCutoff = 1000 };
#endif

// ----------------------------------------------------------------------------------
// hxProfiler internals, do not use directly.

#if HX_HAS_CPP11_TIME
extern std::chrono::high_resolution_clock::time_point g_hxStart;
#endif

// address of s_hxProfilerThreadIdAddress used to uniquely identify thread.
extern HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress;

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

	hxProfiler() : m_isStarted(false) { };

	void start();
	void stop();
	void log();
	void writeToChromeTracing(const char* filename);

	// For testing
	HX_INLINE uint32_t recordsSize() { return m_records.size(); }
	HX_INLINE void recordsClear() { m_records.clear(); }

#if HX_HAS_CPP11_TIME
	HX_INLINE static uint32_t sampleCycles() {
		return (uint32_t)(std::chrono::high_resolution_clock::now() - g_hxStart).count();
	}
#else
	// TODO: read cycle counter register for target.  This version is a Linux fallback.
	HX_INLINE static uint32_t sampleCycles() {
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (uint32_t)ts.tv_nsec;
	}
#endif // !HX_HAS_CPP11_TIME

private:
	template<uint32_t MinCycles> friend class hxProfilerScopeInternal;
	bool m_isStarted;
	hxStockpile<Record, HX_PROFILER_MAX_RECORDS> m_records;
};

// Use direct access to an object with static linkage for speed.
extern hxProfiler g_hxProfiler;

template<uint32_t MinCycles=0u>
class hxProfilerScopeInternal {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal(const char* labelStaticString)
		: m_label(labelStaticString)
	{
		m_t0 = g_hxProfiler.m_isStarted ? hxProfiler::sampleCycles() : ~0u;
	}

	HX_INLINE ~hxProfilerScopeInternal() {
		if (m_t0 != ~0u) {
			uint32_t t1 = hxProfiler::sampleCycles();
			if ((t1 - m_t0) >= MinCycles) {
				void* rec = g_hxProfiler.m_records.emplace_back_atomic();
				if (rec) {
					::new (rec) hxProfiler::Record(m_t0, t1, m_label, (uint32_t)(ptrdiff_t)&s_hxProfilerThreadIdAddress);
				}
			}
		}
	}

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
#define hxProfileScopeMin(...) ((void)0)
#define hxProfilerStart() ((void)0)
#define hxProfilerStop() ((void)0)
#define hxProfilerLog() ((void)0)
#define hxProfilerWriteToChromeTracing(filename) ((void)0)
#endif
