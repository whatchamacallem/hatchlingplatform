// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxProfiler.h>
#include <hx/hxFile.h>
#include <hx/hxConsole.h>

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

#if HX_USE_CPP11_TIME
std::chrono::high_resolution_clock::time_point hxProfiler::s_start;

float g_hxProfilerMillisecondsPerCycle =
	((float)std::chrono::high_resolution_clock::period::num * 1.0e+3f)
		/ (float)std::chrono::high_resolution_clock::period::den;
#else
// TODO: Use target settings.
float g_hxProfilerMillisecondsPerCycle = 1.0e-6f; // Also 1.e+6 cycles/ms, a 1 GHz chip.
#endif // !HX_USE_CPP11_TIME

// ----------------------------------------------------------------------------
// hxProfiler

void hxProfiler::start() {
	m_records.clear();
	m_isStarted = true;

	// Logging may easily be off at this point.
#if HX_USE_CPP11_TIME
	s_start = std::chrono::high_resolution_clock::now();
#endif
}

void hxProfiler::stop() {
	m_isStarted = false;
}

void hxProfiler::log() {
	m_isStarted = false;

	for (uint32_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];

		uint32_t delta = rec.m_end - rec.m_begin;
		hxLogRelease("profile %s: %fms cycles %u thread %x\n", hxBasename(rec.m_label),
			delta * (double)g_hxProfilerMillisecondsPerCycle, (unsigned int)delta,
			(unsigned int)rec.m_threadId);
	}
}

void hxProfiler::writeToChromeTracing(const char* filename) {
	m_isStarted = false;

	hxFile f(hxFile::out, "%s", filename);

	f.print("[\n");
	// Converting absolute values works better with integer precision.
	uint32_t cyclesPerMicrosecond = (uint32_t)(1.0e-3f / g_hxProfilerMillisecondsPerCycle);
	for (uint32_t i = 0; i < m_records.size(); ++i) {
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
