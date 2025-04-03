// Copyright 2017-2025 Adrian Johnston

#include <hx/hxProfiler.h>
#include <hx/hxConsole.h>
#include <hx/hxFile.h>

#if HX_PROFILE

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// Console commands

static void hxProfileStartCommand() {
	hxProfilerStart();
}
hxConsoleCommandNamed(hxProfileStartCommand, profilestart);

static void hxProfilerLogCommand() {
	hxProfilerLog();
}
hxConsoleCommandNamed(hxProfilerLogCommand, profilelog);

static void hxProfilerWriteToChromeTracingCommand(const char* filename) {
	hxProfilerWriteToChromeTracing(filename);
}
hxConsoleCommandNamed(hxProfilerWriteToChromeTracingCommand, profiletrace);

// ----------------------------------------------------------------------------
// variables

// Use the address of a thread local variable as a unique thread id.
HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress = 0;

hxProfiler g_hxProfiler;

// ----------------------------------------------------------------------------
// hxProfiler

void hxProfiler::start() {
	HX_PROFILER_LOCK();
	m_records.clear();
	m_isStarted = true;
}

void hxProfiler::stop() {
	HX_PROFILER_LOCK();
	m_isStarted = false;
}

void hxProfiler::log() {
	HX_PROFILER_LOCK();
	m_isStarted = false;

	for (size_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];

		size_t delta = rec.m_end - rec.m_begin;
		hxLogRelease("profile %s: %fms cycles %u thread %x\n", hxBasename(rec.m_label),
			delta * (double)c_hxTimeMillisecondsPerCycle, (unsigned int)delta,
			(unsigned int)rec.m_threadId);
	}
}

void hxProfiler::writeToChromeTracing(const char* filename) {
	HX_PROFILER_LOCK();
	m_isStarted = false;

	hxFile f(hxFile::out, "%s", filename);

	f.print("[\n");
	// Converting absolute values works better with integer precision.
	size_t cyclesPerMicrosecond = (size_t)(1.0e-3f / c_hxTimeMillisecondsPerCycle);
	for (size_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];
		if (i != 0) { f.print(",\n"); }
		const char* bn = hxBasename(rec.m_label);
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%u},\n",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_begin / cyclesPerMicrosecond));
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%u}",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_end / cyclesPerMicrosecond));
	}
	f.print("\n]\n");

	hxLogConsole("wrote %s.\n", filename);
}

#endif // HX_PROFILE
