// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxProfiler.h"
#include "hxArray.h"
#include "hxFile.h"
#include "hxConsole.h"

#if HX_PROFILE

HX_REGISTER_FILENAME_HASH;

hxProfiler g_hxProfiler;

// ----------------------------------------------------------------------------------
// Console commands

static void hxProfile() { g_hxProfiler.start(); }
hxConsoleCommandNamed(hxProfile, profileStart);

static void hxProfileLog() { g_hxProfiler.log(); }
hxConsoleCommandNamed(hxProfileLog, profileLog);

static void hxProfileToChrome(const char* filename) { g_hxProfiler.writeToChromeTracing(filename); }
hxConsoleCommandNamed(hxProfileToChrome, profileToChrome);

// ----------------------------------------------------------------------------------
// Use the address of a thread local variable as a unique thread id.
HX_THREAD_LOCAL uint8_t s_hxProfilerThreadIdAddress = 0;

// ----------------------------------------------------------------------------------
#if HX_HAS_CPP11_TIME
float g_hxProfilerMillisecondsPerCycle = ((float)c_hxProfilerPeriod::num * 1.0e+3f) / (float)c_hxProfilerPeriod::den;
std::chrono::high_resolution_clock::time_point g_hxStart;
#else
// TODO: Use target settings.
float g_hxProfilerMillisecondsPerCycle = 1.0e-6f; // Also 1.e+6 cycles/ms, a 1 GHz chip.
#endif // !HX_HAS_CPP11_TIME

// ----------------------------------------------------------------------------------

void hxProfiler::init() {
	if (m_isEnabled) {
		hxWarn("Profiler already enabled");
		return;
	}

	m_isEnabled = true;

#if HX_HAS_CPP11_TIME
	g_hxStart = std::chrono::high_resolution_clock::now();
#endif
	// Logging may easily be off at this point.
	hxLogRelease("hxProfilerInit... %u cycles\n", (unsigned int)hxProfilerScopeInternal<0u>::sampleCycles());
}

void hxProfiler::shutdown() {
	if (m_isEnabled) {
		hxLogRelease("hxProfilerShutdown... %u cycles\n", (unsigned int)hxProfilerScopeInternal<0u>::sampleCycles());
	}
	m_isEnabled = false;
	m_records.clear();
}

void hxProfiler::start() {
	if (!m_isEnabled) {
		init();
	}
	else {
		m_records.clear();
	}
}

void hxProfiler::log() {
	if (m_records.empty()) {
		hxLogRelease("hxProfiler no samples\n");
	}

	for (uint32_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];

		uint32_t delta = rec.m_end - rec.m_begin;
		hxLogRelease("hxProfiler %s: thread %x cycles %u %fms\n", hxBasename(rec.m_label),
			(unsigned int)rec.m_threadId, (unsigned int)delta, (float)delta * g_hxProfilerMillisecondsPerCycle);
	}

	m_records.clear();
}

void hxProfiler::writeToChromeTracing(const char* filename) {
	hxFile f(hxFile::out, "%s", filename);

	if (m_records.empty()) {
		f.print("[]\n");
		hxLogRelease("Trace has no samples: %s...\n", filename);
		return;
	}

	f.print("[\n");
	// Converting absolute values works better with integer precision.
	uint32_t cyclesPerMicrosecond = (uint32_t)(1.0e-3f / g_hxProfilerMillisecondsPerCycle);
	for (uint32_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];
		if (i != 0) {
			f.print(",\n");
		}
		const char* bn = hxBasename(rec.m_label);
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%u},\n",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_begin / cyclesPerMicrosecond));
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%u}",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_end / cyclesPerMicrosecond));
	}
	f.print("\n]\n");

	m_records.clear();

	hxLogRelease("Wrote trace to: %s...\n", filename);
}

#endif // HX_PROFILE
