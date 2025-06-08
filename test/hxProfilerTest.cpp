// Copyright 2017-2025 Adrian Johnston

#include <hx/hxProfiler.hpp>
#include <hx/hxTaskQueue.hpp>
#include <hx/hxConsole.hpp>

#include <hx/hxTest.hpp>

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
static const size_t s_hxTestNumLabels = sizeof s_hxTestLabels / sizeof *s_hxTestLabels;

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
			hxcycles_t startCycles = hxTimeSampleCycles();
			hxcycles_t delta = 0u;

			// Open up a sub-scope if time allows.
			if (targetMs >= 2.0f) {
				float subtarget = targetMs / 2.0f;
				const char* subLabel = s_hxTestLabels[(size_t)subtarget];
				hxProfileScope(subLabel);
				generateScopes(subtarget);
			}

			while ((double)delta * c_hxMillisecondsPerCycle < targetMs) {
				// Perform work that might not be optimized away by the compiler.
				size_t ops = (m_accumulator & 0xf) + 1;
				for (size_t i = 0; i < ops; ++i) {
					m_accumulator ^= m_testPrng();
				}

				// Unsigned arithmetic handles clock wrapping correctly.
				delta = hxTimeSampleCycles() - startCycles;
			}
		}

		float m_targetMs;
		size_t m_accumulator;
		hxTestRandom m_testPrng;
	};
};

// ----------------------------------------------------------------------------

TEST_F(hxProfilerTest, Single1ms) {
	hxProfilerBegin();

	size_t startRecords = g_hxProfiler_.recordsSize_();
	{
		hxProfileScope("1 ms");
		hxProfilerTaskTest one;
		one.construct("1 ms", 1.0f);
		one.execute(hxnull);
	}

	ASSERT_TRUE(1u == (g_hxProfiler_.recordsSize_() - startRecords));

	// stops the profiler and dumps sample to console:
	bool isOk = hxConsoleExecLine("profilelog");
	ASSERT_TRUE(isOk);
}

TEST_F(hxProfilerTest, writeToChromeTracing) {
	// Shut down profiling and use console commands for next capture.
	hxProfilerEnd();
	hxConsoleExecLine("profilebegin");

	hxTaskQueue q;
	hxProfilerTaskTest tasks[s_hxTestNumLabels];
	for (size_t i = s_hxTestNumLabels; i--; ) {
		tasks[i].construct(s_hxTestLabels[i], (float)i);
		q.enqueue(tasks + i);
	}
	q.waitForAll();

	ASSERT_TRUE(90u == g_hxProfiler_.recordsSize_());

	hxConsoleExecLine("profilewrite profile.json");
	hxProfilerLog();
}

#endif // HX_PROFILE
