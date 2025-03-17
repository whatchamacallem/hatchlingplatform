#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion
//
// hxProfiler internals.  See hxProfiler.h instead

#if !(HX_PROFILE)
#error #include <hx/hxProfiler.h>
#endif

#if HX_USE_CPP11_THREADS
#include <mutex>

#define HX_PROFILER_LOCK() std::unique_lock<std::mutex> lk(g_hxProfiler.m_mutex)
#else
#define HX_PROFILER_LOCK() (void)0
#endif

// Use direct access to an object with static linkage for speed.
extern class hxProfiler g_hxProfiler;

// Address of s_hxProfilerThreadIdAddress used to uniquely identify thread.
extern HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress;

// ----------------------------------------------------------------------------
// hxProfiler

class hxProfiler {
public:
	struct Record {
		HX_INLINE Record(hx_cycles_t begin_, hx_cycles_t end_, const char* label_, uint32_t threadId_)
			: m_label(label_), m_begin(begin_), m_end(end_), m_threadId(threadId_) {
		}
		const char* m_label;
		hx_cycles_t m_begin;
		hx_cycles_t m_end;
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

private:
	template<hx_cycles_t MinCycles_> friend class hxProfilerScopeInternal;
	bool m_isStarted;
#if HX_USE_CPP11_THREADS
	std::mutex m_mutex;
#endif
	hxArray<Record, HX_PROFILER_MAX_RECORDS> m_records;
};

// ----------------------------------------------------------------------------
// hxProfilerScopeInternal

template<hx_cycles_t MinCycles_=0u>
class hxProfilerScopeInternal {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal(const char* labelStringLiteral)
		: m_label(labelStringLiteral)
	{
		HX_PROFILER_LOCK();

		m_t0 = g_hxProfiler.m_isStarted ? hxTimeSampleCycles() : ~(hx_cycles_t)0;
	}

	HX_INLINE ~hxProfilerScopeInternal() {
		HX_PROFILER_LOCK();

		if (m_t0 != ~(hx_cycles_t)0) {
			hx_cycles_t t1_ = hxTimeSampleCycles();
			if ((t1_ - m_t0) >= MinCycles_) {
				void* rec_ = g_hxProfiler.m_records.emplace_back_unconstructed();
				if (rec_) {
					::new (rec_) hxProfiler::Record(m_t0, t1_, m_label,
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
	hx_cycles_t m_t0;
};

