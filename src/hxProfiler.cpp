// Copyright 2017-2025 Adrian Johnston

#include <hx/hxProfiler.hpp>
#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>

#if HX_PROFILE

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// Console commands

namespace {

void hxProfileBeginCommand_() { hxProfilerBegin(); }

void hxProfileEndCommand_() { hxProfilerEnd(); }

void hxProfilerLogCommand_() { hxProfilerLog(); }

void hxProfilerWriteToChromeTracingCommand_(const char* filename) {
	hxProfilerWriteToChromeTracing(filename);
}

hxConsoleCommandNamed(hxProfileBeginCommand_, profilebegin);
hxConsoleCommandNamed(hxProfileEndCommand_, profileend);
hxConsoleCommandNamed(hxProfilerLogCommand_, profilelog);
hxConsoleCommandNamed(hxProfilerWriteToChromeTracingCommand_, profilewrite);

} // namespace

// ----------------------------------------------------------------------------
// variables

// Use the address of a thread local variable as a unique thread id.
HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress_ = 0;

hxProfilerInternal_ g_hxProfiler_;

// ----------------------------------------------------------------------------
// hxProfilerInternal_

void hxProfilerInternal_::start_() {
	HX_PROFILER_LOCK_();
	m_records.clear();
	m_isStarted_ = true;
}

void hxProfilerInternal_::stop_() {
	HX_PROFILER_LOCK_();
	m_isStarted_ = false;
}

void hxProfilerInternal_::log_() {
	HX_PROFILER_LOCK_();
	m_isStarted_ = false;

	for (size_t i = 0; i < m_records.size(); ++i) {
		const hxProfilerRecord_& rec = m_records[i];

		hxcycles_t delta = rec.m_end_ - rec.m_begin_;
		hxLogRelease("profile %s: %.15gms cycles %.15g thread %x\n", hxBasename(rec.m_label_),
			(double)delta * c_hxMillisecondsPerCycle, (double)delta,
			(unsigned int)rec.m_threadId_);
	}
}

void hxProfilerInternal_::writeToChromeTracing_(const char* filename) {
	HX_PROFILER_LOCK_();
	m_isStarted_ = false;

	hxFile f(hxFile::out, "%s", filename);

	f.print("[\n");
	if(!m_records.empty()) {
		// this works for 32-bit hxcycles_t too.
		hxcycles_t epoch = m_records[0].m_begin_;
		for (size_t i = 0; i < m_records.size(); ++i) {
			const hxProfilerRecord_& rec = m_records[i];
			if (i != 0) { f.print(",\n"); }
			const char* bn = hxBasename(rec.m_label_);
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%.15g},\n",
				bn, (unsigned int)rec.m_threadId_, (double)(rec.m_begin_ - epoch) * c_hxMicrosecondsPerCycle);
			f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%.15g}",
				bn, (unsigned int)rec.m_threadId_, (double)(rec.m_end_ - epoch) * c_hxMicrosecondsPerCycle);
		}
	}
	f.print("\n]\n");

	hxLogConsole("wrote %s.\n", filename);
}

#endif // HX_PROFILE
