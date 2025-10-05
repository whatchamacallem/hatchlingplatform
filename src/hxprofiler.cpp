// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxprofiler.hpp"
#include "../include/hx/hxconsole.hpp"
#include "../include/hx/hxfile.hpp"

HX_REGISTER_FILENAME_HASH

#if HX_PROFILE

namespace hxdetail_ {

// ----------------------------------------------------------------------------
// Console commands

static bool hxprofile_start_command_(void) { hxprofiler_start(); return true; }

static bool hxprofile_stop_command_(void) { hxprofiler_stop(); return true; }

static bool hxprofiler_log_command_(void) { hxprofiler_log(); return true; }

static bool hxprofiler_write_to_chrome_tracing_command_(const char* filename) {
	hxprofiler_write_to_chrome_tracing(filename);
	return true;
}

hxconsole_command_named(hxprofile_start_command_, profilestart);
hxconsole_command_named(hxprofile_stop_command_, profilestop);
hxconsole_command_named(hxprofiler_log_command_, profilelog);
hxconsole_command_named(hxprofiler_write_to_chrome_tracing_command_, profilewrite);

// ----------------------------------------------------------------------------
// variables

hxprofiler_internal_ g_hxprofiler_;

// ----------------------------------------------------------------------------
// hxprofiler_internal_

void hxprofiler_internal_::start_(void) {
	HX_PROFILER_LOCK_();
	m_records.clear();
	m_is_started_ = true;
}

void hxprofiler_internal_::stop_(void) {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;
}

void hxprofiler_internal_::log_(void) {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;

	hxlogconsole("[ ");
	for(size_t i = 0; i < m_records.size(); ++i) {
		const hxprofiler_record_& rec = m_records[i];

		if(i != 0) { hxlogconsole(",\n"); }

		const hxcycles_t delta = rec.m_end_ - rec.m_begin_;
		hxlogconsole("{ \"name\":\"%s\", \"ms\":%.15g, \"thread\":\"%x\" }",
			rec.m_label_, (double)delta * hxmilliseconds_per_cycle,
			(unsigned int)rec.m_thread_id_);
	}
	hxlogconsole(" ]\n");
}

void hxprofiler_internal_::write_to_chrome_tracing_(const char* filename) {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;

	hxfile f(hxfile::out, "%s", filename);

	f.print("[\n");
	if(!m_records.empty()) {
		// this works for 32-bit hxcycles_t too.
		const hxcycles_t epoch = m_records[0].m_begin_;
		for(size_t i = 0; i < m_records.size(); ++i) {
			const hxprofiler_record_& rec = m_records[i];
			if(i != 0) { f.print(",\n"); }
			const char* label = rec.m_label_;
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%.15g},\n",
				label, (unsigned int)rec.m_thread_id_, (double)(rec.m_begin_ - epoch) * hxmicroseconds_per_cycle);
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%.15g}",
				label, (unsigned int)rec.m_thread_id_, (double)(rec.m_end_ - epoch) * hxmicroseconds_per_cycle);
		}
	}
	f.print("\n]\n");

	hxlogconsole("wrote %s.\n", filename);
}

} // hxdetail_

#endif // HX_PROFILE
