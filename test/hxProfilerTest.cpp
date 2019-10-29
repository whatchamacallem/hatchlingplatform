// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxProfiler.h>
#include <hx/hxTaskQueue.h>
#include <hx/hxConsole.h>

#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
#if HX_PROFILE

static const char* s_hxTestLabels[] = {
	"Alpha",   "Beta",     "Gamma",
	"Delta",   "Epsilon",  "Zeta",
	"Eta",     "Theta",    "Iota",
	"Kappa",   "Lambda",   "Mu",
	"Nu",       "Xi",      "Omicron",
	"Pi",       "Rho",     "Sigma",
	"Tau",      "Upsilon", "Phi",
	"Chi",      "Psi",     "Omega"
};
static const uint32_t s_hxTestNumLabels = sizeof s_hxTestLabels / sizeof *s_hxTestLabels;

class hxProfilerTest :
	public testing::Test
{
public:
	struct hxProfilerTaskTest : public hxTask {
		hxProfilerTaskTest() : m_targetMs(0.0f), m_accumulator(0) { }

		void construct(const char* label, float targetMs) {
			setLabel(label);
			m_targetMs = targetMs;
			m_accumulator = 0;
		}

		virtual void execute(hxTaskQueue* q) HX_OVERRIDE {
			(void)q;
			generateScopes(m_targetMs);
		}

		virtual void generateScopes(float targetMs) {
			uint32_t startCycles = hxTimeSampleCycles();
			uint32_t delta = 0u;

			// Open up a sub-scope if time allows.
			if (targetMs >= 2.0f) {
				float subtarget = targetMs / 2.0f;
				const char* subLabel = s_hxTestLabels[(int32_t)subtarget];
				hxProfileScope(subLabel);
				generateScopes(subtarget);
			}

			while ((float)delta * c_hxTimeMillisecondsPerCycle < targetMs) {
				// Perform work that might not be optimized away by the compiler.
				int32_t ops = (m_accumulator & 0xf) + 1;
				for (int32_t i = 0; i < ops; ++i) {
					m_accumulator ^= m_testPrng();
				}

				// Unsigned arithmetic handles clock wrapping correctly.
				delta = hxTimeSampleCycles() - startCycles;
			}
		}

		float m_targetMs;
		int32_t m_accumulator;
		hxTestRandom m_testPrng;
	};
};

// ----------------------------------------------------------------------------

TEST_F(hxProfilerTest, Single1ms) {
	hxProfilerStart();

	uint32_t startRecords = g_hxProfiler.recordsSize();
	{
		hxProfileScope("1 ms");
		hxProfilerTaskTest one;
		one.construct("1 ms", 1.0f);
		one.execute(hxnull);
	}

	ASSERT_TRUE(1u == (g_hxProfiler.recordsSize() - startRecords));

	bool isok = hxConsoleExecLine("profilelog");
	ASSERT_TRUE(isok);
}

TEST_F(hxProfilerTest, writeToChromeTracing) {
	// Shut down profiling and use console commands for next capture.
	hxProfilerStop();
	hxConsoleExecLine("profilestart");

	hxTaskQueue q;
	hxProfilerTaskTest tasks[s_hxTestNumLabels];
	for (int32_t i = s_hxTestNumLabels; i--; ) {
		tasks[i].construct(s_hxTestLabels[i], (float)i);
		q.enqueue(tasks + i);
	}
	q.waitForAll();

	ASSERT_TRUE(90u == g_hxProfiler.recordsSize());

	hxConsoleExecLine("profiletrace profile.json");
	hxProfilerLog();
}

#endif // HX_PROFILE
