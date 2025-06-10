#pragma once
// Copyright 2017-2025 Adrian Johnston
//
// hxProfilerInternal_ internals. See hxProfiler.h instead

#if !defined(HX_PROFILE)
#error #include <hx/hxProfiler.h> instead
#endif

#include <hx/hxArray.hpp>

#if defined(__x86_64__) || defined(__i386__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#if HX_USE_CPP_THREADS
#include <mutex>

#define HX_PROFILER_LOCK_() std::unique_lock<std::mutex> hxProfilerMutexLock_(g_hxProfiler_.m_mutex)
#else
#define HX_PROFILER_LOCK_() (void)0
#endif

static inline hxcycles_t hxTimeSampleCycles(void);

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
	inline size_t recordsSize_() { return m_records.size(); }
	inline void recordsClear_() { m_records.clear(); }

private:
	struct hxProfilerRecord_ {
		inline hxProfilerRecord_(size_t begin_, size_t end_, const char* label_, uint32_t threadId_)
			: m_label(label_), m_begin(begin_), m_end(end_), m_threadId(threadId_) {
		}
		const char* m_label;
		hxcycles_t m_begin;
		hxcycles_t m_end;
		uint32_t m_threadId;
	};

	template<hxcycles_t MinCycles_> friend class hxProfilerScopeInternal_;

	bool m_isStarted;
#if HX_USE_CPP_THREADS
	std::mutex m_mutex;
#endif
	hxArray<hxProfilerRecord_, HX_PROFILER_MAX_RECORDS> m_records;
};

// ----------------------------------------------------------------------------
// hxProfilerScopeInternal_

template<hxcycles_t MinCycles_=0u>
class hxProfilerScopeInternal_ {
public:
	// See hxProfileScope() below.
	inline hxProfilerScopeInternal_(const char* labelStringLiteral)
		: m_label(labelStringLiteral)
	{
		// fastest not to check if the profiler is running.
		m_t0 = hxTimeSampleCycles();
	}

#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxProfilerScopeInternal_() {
		hxcycles_t t1_ = hxTimeSampleCycles();

		HX_PROFILER_LOCK_();

		if (g_hxProfiler_.m_isStarted) {
			if ((t1_ - m_t0) >= MinCycles_) {
				void* rec_ = g_hxProfiler_.m_records.emplaceBackUnconstructed();
				if (rec_) {
					::new (rec_) hxProfilerInternal_::hxProfilerRecord_(m_t0, t1_, m_label,
						(uint32_t)(uintptr_t)&s_hxProfilerThreadIdAddress_);
				}
			}
		}
	}

private:
	hxProfilerScopeInternal_(void) HX_DELETE_FN;
	hxProfilerScopeInternal_(const hxProfilerScopeInternal_&) HX_DELETE_FN;
	void operator=(const hxProfilerScopeInternal_&) HX_DELETE_FN;
	const char* m_label;
	hxcycles_t m_t0;
};
