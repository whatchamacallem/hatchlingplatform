#pragma once
// Copyright 2017-2025 Adrian Johnston
//
// hxProfilerInternal_ internals.  See hxProfiler.h instead

#if !defined(HX_PROFILE)
#error #include <hx/hxProfiler.h> instead
#endif

#if HX_USE_CPP11_THREADS
#include <mutex>

#define HX_PROFILER_LOCK_() std::unique_lock<std::mutex> hxProfilerMutexLock_(g_hxProfiler_.m_mutex)
#else
#define HX_PROFILER_LOCK_() (void)0
#endif

// Use direct access to an object with static linkage for speed.
extern class hxProfilerInternal_ g_hxProfiler_;

// Address of s_hxProfilerThreadIdAddress_ used to uniquely identify thread.
extern HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress_;

// ----------------------------------------------------------------------------
// hxProfilerInternal_

class hxProfilerInternal_ {
public:
	hxProfilerInternal_() : m_isStarted(false) { };

	void start_();
	void stop_();
	void log_();
	void writeToChromeTracing_(const char* filename);

	// For testing
	HX_INLINE size_t recordsSize_() { return m_records.size(); }
	HX_INLINE void recordsClear_() { m_records.clear(); }

private:
	struct hxProfilerRecord_ {
		HX_INLINE hxProfilerRecord_(hx_cycles_t begin_, hx_cycles_t end_, const char* label_, uint32_t threadId_)
			: m_label(label_), m_begin(begin_), m_end(end_), m_threadId(threadId_) {
		}
		const char* m_label;
		hx_cycles_t m_begin;
		hx_cycles_t m_end;
		uint32_t m_threadId;
	};

	template<hx_cycles_t MinCycles_> friend class hxProfilerScopeInternal_;

	bool m_isStarted;
#if HX_USE_CPP11_THREADS
	std::mutex m_mutex;
#endif
	hxArray<hxProfilerRecord_, HX_PROFILER_MAX_RECORDS> m_records;
};

// ----------------------------------------------------------------------------
// hxProfilerScopeInternal_

template<hx_cycles_t MinCycles_=0u>
class hxProfilerScopeInternal_ {
public:
	// See hxProfileScope() below.
	HX_INLINE hxProfilerScopeInternal_(const char* labelStringLiteral)
		: m_label(labelStringLiteral)
	{
		HX_PROFILER_LOCK_();

		m_t0 = g_hxProfiler_.m_isStarted ? hxTimeSampleCycles() : ~(hx_cycles_t)0;
	}

	HX_INLINE ~hxProfilerScopeInternal_() {
		HX_PROFILER_LOCK_();

		if (m_t0 != ~(hx_cycles_t)0) {
			hx_cycles_t t1_ = hxTimeSampleCycles();
			if ((t1_ - m_t0) >= MinCycles_) {
				void* rec_ = g_hxProfiler_.m_records.emplace_back_unconstructed();
				if (rec_) {
					::new (rec_) hxProfilerInternal_::hxProfilerRecord_(m_t0, t1_, m_label,
						(uint32_t)(uintptr_t)&s_hxProfilerThreadIdAddress_);
				}
			}
		}
	}

private:
	hxProfilerScopeInternal_(); // = delete
	hxProfilerScopeInternal_(const hxProfilerScopeInternal_&); // = delete
	void operator=(const hxProfilerScopeInternal_&); // = delete
	const char* m_label;
	hx_cycles_t m_t0;
};

