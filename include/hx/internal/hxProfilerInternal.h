#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

// ----------------------------------------------------------------------------
// hxProfiler internals.  See hxProfiler.h instead

#if !(HX_PROFILE)
#error #include <hx/hxProfiler.h>
#endif

#if HX_USE_CPP11_TIME
#include <chrono>
#else
#include <time.h>
#endif

// ----------------------------------------------------------------------------
// variables

// Use direct access to an object with static linkage for speed.
extern class hxProfiler g_hxProfiler;

// address of s_hxProfilerThreadIdAddress used to uniquely identify thread.
extern HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress;

extern float g_hxProfilerMillisecondsPerCycle; // Scales cycles to ms.

// ----------------------------------------------------------------------------
// hxProfiler

class hxProfiler {
public:
	struct Record {
		HX_INLINE Record(uint32_t begin, uint32_t end, const char* label, uint32_t threadId)
			: m_label(label), m_begin(begin), m_end(end), m_threadId(threadId) {
		}
		const char* m_label;
		uint32_t m_begin;
		uint32_t m_end;
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

#if HX_USE_CPP11_TIME
	static std::chrono::high_resolution_clock::time_point s_start;

	HX_INLINE static uint32_t sampleCycles() {
		return (uint32_t)(std::chrono::high_resolution_clock::now() - s_start).count();
	}
#else
	// TODO: read cycle counter register for target.  This version is a Linux fallback.
	HX_INLINE static uint32_t sampleCycles() {
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (uint32_t)ts.tv_nsec;
	}
#endif // !HX_USE_CPP11_TIME

private:
	template<uint32_t MinCycles> friend class hxProfilerScopeInternal;
	bool m_isStarted;
	hxStockpile<Record, HX_PROFILER_MAX_RECORDS> m_records;
};

// ----------------------------------------------------------------------------
// hxProfilerScopeInternal

template<uint32_t MinCycles=0u>
class hxProfilerScopeInternal {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal(const char* labelStringLiteral)
		: m_label(labelStringLiteral)
	{
		m_t0 = g_hxProfiler.m_isStarted ? hxProfiler::sampleCycles() : ~(uint32_t)0;
	}

	HX_INLINE ~hxProfilerScopeInternal() {
		if (m_t0 != ~(uint32_t)0) {
			uint32_t t1 = hxProfiler::sampleCycles();
			if ((t1 - m_t0) >= MinCycles) {
				void* rec = g_hxProfiler.m_records.emplace_back_atomic();
				if (rec) {
					::new (rec) hxProfiler::Record(m_t0, t1, m_label,
						(uint32_t)(uintptr_t)&s_hxProfilerThreadIdAddress);
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

