// Copyright 2017-2025 Adrian Johnston

#include <hx/hxprofiler.hpp>
#include <hx/hxtask_queue.hpp>
#include <hx/hxconsole.hpp>
#include <hx/hxrandom.hpp>

#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

#if HX_PROFILE

static const char* s_hxtest_labels[] = {
	"Alpha",   "Beta",	 "Gamma",
	"Delta",   "Epsilon",  "Zeta",
	"Eta",	 "Theta",	"Iota",
	"Kappa",   "Lambda",   "Mu",
	"Nu",	   "Xi",	  "Omicron",
	"Pi",	   "Rho",	 "Sigma",
	"Tau",	  "Upsilon", "Phi",
	"Chi",	  "Psi",	 "Omega"
};
static const size_t s_hxtest_num_labels = sizeof s_hxtest_labels / sizeof *s_hxtest_labels;

class hxprofiler_test :
	public testing::Test
{
public:
	class hxprofiler_task_test : public hxtask {
	public:
		hxprofiler_task_test() : m_target_ms_(0.0f), m_accumulator_(0) { }

		void construct(const char* label, float target_ms) {
			set_label(label);
			m_target_ms_ = target_ms;
			m_accumulator_ = 0;
		}

		virtual void execute(hxtask_queue* q) override {
			(void)q;
			generate_scopes(m_target_ms_);
		}

		virtual void generate_scopes(float target_ms) {
			hxcycles_t start_cycles = hxtime_sample_cycles();
			hxcycles_t delta = 0u;

			// Open up a sub-scope if time allows.
			if (target_ms >= 2.0f) {
				float subtarget = target_ms / 2.0f;
				const char* sub_label = s_hxtest_labels[(size_t)subtarget];
				hxprofile_scope(sub_label);
				generate_scopes(subtarget);
			}

			while ((double)delta * hxmilliseconds_per_cycle < target_ms) {
				// Perform work that might not be optimized away by the compiler.
				uint32_t ops = (m_accumulator_ & 0xf) + 1;
				for (uint32_t i = 0; i < ops; ++i) {
					m_accumulator_ ^= (uint32_t)m_test_prng_;
				}

				// Unsigned arithmetic handles clock wrapping correctly.
				delta = hxtime_sample_cycles() - start_cycles;
			}
		}

	private:
		float m_target_ms_;
		uint32_t m_accumulator_;
		hxrandom m_test_prng_;
	};
};

TEST_F(hxprofiler_test, single1ms) {
	hxprofiler_start();

	size_t start_records = g_hxprofiler_.records_size_();
	{
		hxprofile_scope("1 ms");
		hxprofiler_task_test one;
		one.construct("1 ms", 1.0f);
		one.execute(hxnull);
	}

	EXPECT_TRUE(1u == (g_hxprofiler_.records_size_() - start_records));

	// stops the profiler and dumps sample to console:
	bool is_ok = hxconsole_exec_line("profilelog");
	EXPECT_TRUE(is_ok);
}

TEST_F(hxprofiler_test, write_to_chrome_tracing) {
	// Shut down profiling and use console commands for next capture.
	hxprofiler_stop();
	hxconsole_exec_line("profilestart");

	hxtask_queue q;
	hxprofiler_task_test tasks[s_hxtest_num_labels];
	for (size_t i = s_hxtest_num_labels; i--; ) {
		tasks[i].construct(s_hxtest_labels[i], (float)i);
		q.enqueue(tasks + i);
	}
	q.wait_for_all();

	EXPECT_TRUE(90u == g_hxprofiler_.records_size_());

	hxconsole_exec_line("profilewrite profile.json");
	hxprofiler_log();
}

#endif // HX_PROFILE
