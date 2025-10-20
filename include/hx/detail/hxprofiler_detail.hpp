#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//
// hxprofiler_internal_ internals. See hxprofiler.hpp instead.

#if !defined HX_PROFILE
#error #include <hx/hxprofiler.h> instead.
#endif

#include "../hxarray.hpp"
#include "../hxthread.hpp"

#if HX_USE_THREADS
#define HX_PROFILER_LOCK_() const hxunique_lock hxprofiler_mutex_lock_(g_hxprofiler_.m_mutex_)
#else
#define HX_PROFILER_LOCK_() (void)0
#endif

inline hxcycles_t hxtime_sample_cycles(void) {
	uint64_t cycles_ = 0; (void)cycles_;
#if defined __wasm__
	const double t_ = emscripten_get_now() * 1.0e+6;
	cycles_ = (uint64_t)t_;
#elif defined __x86_64__ || defined __i386__
	cycles_ = __rdtsc();
#elif defined __aarch64__  // ARMv8-A 64-bit.
	__asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles_));
#elif defined __arm__  // ARMv7-A 32-bit.
	uint32_t t_;
	__asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(t_));
	cycles_ = (uint64_t)t_;
#elif defined __riscv && (__riscv_xlen == 64)
	__asm__ volatile("rdcycle %0" : "=r"(cycles_));
#elif defined __powerpc__ || defined __ppc__
	__asm__ volatile("mftb %0" : "=r"(cycles_));
#else
static_assert(0, "Implement hxtime_sample_cycles.");
#endif
	return static_cast<hxcycles_t>(cycles_);
}

namespace hxdetail_ {

// Use direct access to an object with static linkage for speed.
extern class hxprofiler_internal_ g_hxprofiler_;

// hxprofiler_internal_ - Manager object for internal use.
class hxprofiler_internal_ {
public:
	hxprofiler_internal_() : m_is_started_(false) { };

	void start_();
	void stop_();
	void log_();
	void write_to_chrome_tracing_(const char* filename) hxattr_nonnull(2);

	// For testing.
	size_t records_size_(void) { return m_records.size(); }
	void records_clear_(void) { m_records.clear(); }

private:
	class hxprofiler_record_ {
	public:
		explicit hxprofiler_record_(size_t begin_, size_t end_, const char* label_, uint32_t thread_id_)
			: m_label_(label_), m_begin_(begin_), m_end_(end_), m_thread_id_(thread_id_) {
		}
		const char* m_label_;
		hxcycles_t m_begin_;
		hxcycles_t m_end_;
		uint32_t m_thread_id_;
	};

	template<hxcycles_t min_cycles_> friend class hxprofiler_scope_internal_;

	bool m_is_started_;
#if HX_USE_THREADS
	hxmutex m_mutex_;
#endif
	hxarray<hxprofiler_record_, HX_PROFILER_MAX_RECORDS> m_records;
};

// hxprofiler_scope_internal_ - RAII object for internal use.
template<hxcycles_t min_cycles_=0u>
class hxprofiler_scope_internal_ {
public:
	// See hxprofile_scope() below.
	hxprofiler_scope_internal_(const char* label_string_literal_) hxattr_nonnull(2)
		: m_label_(label_string_literal_)
	{
		// Fastest to avoid checking whether the profiler is running.
		m_t0_ = hxtime_sample_cycles();
	}

	~hxprofiler_scope_internal_(void) {
		// Avoid overhead in leaf samples.
		const hxcycles_t t1_ = hxtime_sample_cycles();

		HX_PROFILER_LOCK_();

		if(g_hxprofiler_.m_is_started_) {
			if((t1_ - m_t0_) >= min_cycles_) {
				if(!g_hxprofiler_.m_records.full()) {
					g_hxprofiler_.m_records.emplace_back(
						m_t0_, t1_, m_label_, static_cast<uint32_t>(hxthread_id()));
				}
			}
		}
	}

private:
	hxprofiler_scope_internal_(void) = delete;
	hxprofiler_scope_internal_(const hxprofiler_scope_internal_&) = delete;
	void operator=(const hxprofiler_scope_internal_&) = delete;
	const char* m_label_;
	hxcycles_t m_t0_;
};

} // hxdetail_
using namespace hxdetail_;
