// Copyright 2017-2025 Adrian Johnston

#include <hx/hxprofiler.hpp>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>

#if HX_PROFILE

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// Console commands

namespace {

bool hxprofile_start_command_() { hxprofiler_start(); return true; }

bool hxprofile_stop_command_() { hxprofiler_stop(); return true; }

bool hxprofiler_log_command_() { hxprofiler_log(); return true; }

bool hxprofiler_write_to_chrome_tracing_command_(const char* filename) {
	hxprofiler_write_to_chrome_tracing(filename);
	return true;
}

hxconsole_command_named(hxprofile_start_command_, profilestart);
hxconsole_command_named(hxprofile_stop_command_, profilestop);
hxconsole_command_named(hxprofiler_log_command_, profilelog);
hxconsole_command_named(hxprofiler_write_to_chrome_tracing_command_, profilewrite);

} // namespace

// ----------------------------------------------------------------------------
// variables

// Use the address of a thread local variable as a unique thread id.
HX_THREAD_LOCAL uint8_t s_hxprofiler_thread_id_address_ = 0;

hxprofiler_internal_ g_hxprofiler_;

// ----------------------------------------------------------------------------
// hxprofiler_internal_

void hxprofiler_internal_::start_() {
	HX_PROFILER_LOCK_();
	m_records.clear();
	m_is_started_ = true;
}

void hxprofiler_internal_::stop_() {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;
}

void hxprofiler_internal_::log_() {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;

	for (size_t i = 0; i < m_records.size(); ++i) {
		const hxprofiler_record_& rec = m_records[i];

		hxcycles_t delta = rec.m_end_ - rec.m_begin_;
		hxlogrelease("profile %s: %.15gms cycles %.15g thread %x\n", hxbasename(rec.m_label_),
			(double)delta * hxmilliseconds_per_cycle, (double)delta,
			(unsigned int)rec.m_thread_id_);
	}
}

void hxprofiler_internal_::write_to_chrome_tracing_(const char* filename) {
	HX_PROFILER_LOCK_();
	m_is_started_ = false;

	hxfile f(hxfile::out, "%s", filename);

	f.print("[\n");
	if(!m_records.empty()) {
		// this works for 32-bit hxcycles_t too.
		hxcycles_t epoch = m_records[0].m_begin_;
		for (size_t i = 0; i < m_records.size(); ++i) {
			const hxprofiler_record_& rec = m_records[i];
			if (i != 0) { f.print(",\n"); }
			const char* bn = hxbasename(rec.m_label_);
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%.15g},\n",
				bn, (unsigned int)rec.m_thread_id_, (double)(rec.m_begin_ - epoch) * hxmicroseconds_per_cycle);
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%.15g}",
				bn, (unsigned int)rec.m_thread_id_, (double)(rec.m_end_ - epoch) * hxmicroseconds_per_cycle);
		}
	}
	f.print("\n]\n");

	hxlogconsole("wrote %s.\n", filename);
}

#endif // HX_PROFILE
