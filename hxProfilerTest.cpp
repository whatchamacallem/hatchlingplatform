// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxProfiler.h"
#include "hxTest.h"
#include "hxTestPRNG.h"
#include "hxTaskQueue.h"
#include "hxConsole.h"

HX_REGISTER_FILENAME_HASH;

// ----------------------------------------------------------------------------------
#if HX_PROFILE

static const char* s_hxTestLabels[] = {
	"Alpha",   "Beta",     "Gamma",
	"Delta",   "hxsilon",  "Zeta",
	"Eta",     "Theta",    "Iota",
	"Kappa",   "Lambda",   "Mu",
	"Nu",       "Xi",      "Omicron",
	"Pi",       "Rho",     "Sigma",
	"Tau",      "Upsilon", "Phi",
	"Chi",      "Psi",     "Omega"
};
static const uint32_t s_hxTestNumLabels = sizeof s_hxTestLabels / sizeof *s_hxTestLabels;

class hxProfilerTest :
	public testing::test
{
public:
	struct hxProfilerTaskTest : public hxTaskQueue::Task {
		hxProfilerTaskTest() : m_targetMs(0.0f), m_accumulator(0) { }

		void construct(const char* label, float targetMs) {
			setLabel(label);
			m_targetMs = targetMs;
			m_accumulator = 0;
		}

		virtual void execute(hxTaskQueue* q) HX_OVERRIDE {
			generateScopes(m_targetMs);
		}

		virtual void generateScopes(float targetMs) {
			uint32_t startCycles = hxProfilerScopeInternal<0u>::sampleCycles();
			uint32_t delta = 0u;

			// Open up a sub-scope if time allows.
			if (targetMs >= 2.0f) {
				float subtarget = targetMs / 2.0f;
				const char* subLabel = s_hxTestLabels[(int32_t)subtarget];
				hxProfileScope(subLabel);
				generateScopes(subtarget);
			}

			while ((float)delta * g_hxProfilerMillisecondsPerCycle < targetMs) {
				// Perform work that might not be optimized away by the compiler.
				int32_t ops = (m_accumulator & 0xff);
				for (int32_t i = 0; i < ops; ++i) {
					m_accumulator ^= m_testPrng();
				}

				// Unsigned arithmetic handles clock wrapping correctly.
				delta = hxProfilerScopeInternal<0u>::sampleCycles() - startCycles;
			}
		}

		float m_targetMs;
		int32_t m_accumulator;
		hxTestPRNG m_testPrng;
	};
};

// ----------------------------------------------------------------------------------

TEST_F(hxProfilerTest, Single1ms) {
	uint32_t startRecords = g_hxProfiler.recordsSize();
	{
		hxProfileScope("1 ms");
		hxProfilerTaskTest one;
		one.construct("1 ms", 1.0f);
		one.execute(hx_null);
	}

	ASSERT_TRUE(1u == (g_hxProfiler.recordsSize() - startRecords));
}

TEST_F(hxProfilerTest, writeToChromeTracing) {
	// Shut down profiling and use console commands for next capture.
	hxProfilerStop();
	hxConsoleExecLine("profileStart");

	hxTaskQueue q;
	hxProfilerTaskTest tasks[s_hxTestNumLabels];
	for (int32_t i = s_hxTestNumLabels; i--; ) {
		tasks[i].construct(s_hxTestLabels[i], (float)i);
		q.enqueue(tasks + i);
	}
	q.waitForAll();

	ASSERT_TRUE(90u == g_hxProfiler.recordsSize());

	hxConsoleExecLine("profileToChrome profile.json");
}

#endif // HX_PROFILE
