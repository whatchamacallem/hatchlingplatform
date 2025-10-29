// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxtest.hpp"

#include <math.h>

#if !HX_USE_GOOGLE_TEST

namespace hxdetail_ {

// hxtest_*_eq_ - ULP-based floating point equality (GoogleTest-compatible)
// Compares IEEE-754 numbers by units in the last place (ULPs) with a fixed
// threshold of 4 ULPs for gtest's `EXPECT_FLOAT_EQ` / `EXPECT_DOUBLE_EQ`.
// Unlike Google Test, this fails on any non-finite values because comparing
// test data with infinity indicates a possible issue.

#if defined __GNUC__ || defined __clang__
__attribute__((optnone))
#endif
bool hxtest_float_eq_(float a_, float b_) {
	if(!isfinite(a_) || !isfinite(b_)) { return false; }
	if(a_ == b_) { return true; }

	uint32_t ua_; ::memcpy(&ua_, &a_, sizeof ua_);
	uint32_t ub_; ::memcpy(&ub_, &b_, sizeof ub_);

	const uint32_t sign_mask_ = 1u << 31;
	const uint32_t ba_ = ((ua_ & sign_mask_) != 0u) ? (~ua_ + 1u) : (sign_mask_ | ua_);
	const uint32_t bb_ = ((ub_ & sign_mask_) != 0u) ? (~ub_ + 1u) : (sign_mask_ | ub_);
	const uint32_t delta_ = (ba_ >= bb_) ? (ba_ - bb_) : (bb_ - ba_);

	return delta_ <= 4u; // 4 ULPs.
}

#if defined __GNUC__ || defined __clang__
__attribute__((optnone))
#endif
bool hxtest_double_eq_(double a_, double b_) {
	if(!isfinite(a_) || !isfinite(b_)) { return false; }
	if(a_ == b_) { return true; }

	uint64_t ua_; ::memcpy(&ua_, &a_, sizeof ua_);
	uint64_t ub_; ::memcpy(&ub_, &b_, sizeof ub_);

	const uint64_t sign_mask_ = 1ull << 63;
	const uint64_t ba_ = ((ua_ & sign_mask_) != 0u) ? (~ua_ + 1u) : (sign_mask_ | ua_);
	const uint64_t bb_ = ((ub_ & sign_mask_) != 0u) ? (~ub_ + 1u) : (sign_mask_ | ub_);
	const uint64_t delta_ = (ba_ >= bb_) ? (ba_ - bb_) : (bb_ - ba_);

	return delta_ <= 4u; // 4 ULPs.
}

static bool hxtest_case_sort_(const hxtest_case_interface_* a_, const hxtest_case_interface_* b_) {
	// Run tests by suite name and then by line number. Runs smoke tests before
	// complex tests in the order written.
	const int compare_ = ::strcmp(a_->suite_(), b_->suite_());
	if(compare_ == 0) { return a_->line_() < b_->line_(); }
	return compare_ < 0;
}

hxtest_::hxtest_(void) {
	::memset((void*)this, 0x00, sizeof *this);
}

hxtest_& hxtest_::dispatcher_(void) {
	static hxtest_ s_hxtest_runner;
	return s_hxtest_runner;
}

void hxtest_::add_test_(hxtest_case_interface_* fn_) {
	// Use -DHX_TEST_MAX_CASES to provide enough room for all tests.
	hxassertrelease(m_num_test_cases_ < HX_TEST_MAX_CASES, "HX_TEST_MAX_CASES overflow\n");
	if(m_num_test_cases_ < HX_TEST_MAX_CASES) {
		m_test_cases_[m_num_test_cases_++] = fn_;
	}
}

// Message is required to end with an \n. Returns hxdev_null on success and
// hxerr otherwise.
hxfile& hxtest_::condition_check_(bool condition_, const char* file_, size_t line_, const char* message_, bool is_assert_) {
	hxassertrelease(m_current_test_, "test_not_started");
	m_test_state_ = (condition_ && m_test_state_ != test_state_fail_) ? test_state_pass_ : test_state_fail_;
		if(!condition_) {
			++m_total_assert_count_;
			if(++m_assert_count_ >= max_fail_messages_) {
				if(m_assert_count_ == max_fail_messages_) {
					hxlogconsole("remaining asserts will fail silently...\n");
				}
				return hxdev_null;
			}

		// Prints full-path error messages that can be clicked on in an IDE.
		m_current_test_->case_();
		m_current_test_->suite_();
		hxloghandler(hxloglevel_assert, "test_fail %s.%s", m_current_test_->suite_(), m_current_test_->case_());
		hxloghandler(hxloglevel_assert, "test_fail_at %s(%zu): %s", file_, line_, message_);

		if(is_assert_) {
			// ASSERT_* macros halt the test suite on failure.
			hxloghandler(hxloglevel_assert, "test_assert_fail ❌");
			hxbreakpoint();
			::_Exit(EXIT_FAILURE);
		}
		else {
			// Debug builds always set breakpoints on unexpected failures.
			// Implements GTEST_FLAG_SET(break_on_failure, true);
#if (HX_TEST_ERROR_HANDLING) == 0 && (HX_RELEASE) == 0
			hxbreakpoint();
#endif
		}
		return hxerr;
	}
	return hxdev_null;
}

size_t hxtest_::run_all_tests_(const char* test_suite_filter_) {
	hxinit(); // RUN_ALL_TESTS could be called first.
	hxlogconsole("[==========] Running tests: %s\n", (test_suite_filter_ ? test_suite_filter_ : "All"));

	m_test_suite_filter_ = test_suite_filter_;
	m_pass_count_ = m_fail_count_ = 0u;
	m_total_assert_count_ = 0u;

	// Breaking hxinsertion_sort breaks everything.
	hxinsertion_sort(m_test_cases_, m_test_cases_ + m_num_test_cases_, hxtest_case_sort_);

	// Starting point. Expected to reset to zero after each test.
	hxsystem_allocator_scope temporary_stack_base(hxsystem_allocator_temporary_stack);

	hxassertrelease(temporary_stack_base.get_current_allocation_count() == 0u
				&& temporary_stack_base.get_current_bytes_allocated() == 0u,
		"test_leaks Temp stack is expected to be empty when running tests.");

	for(hxtest_case_interface_** it_ = m_test_cases_; it_ != (m_test_cases_ + m_num_test_cases_); ++it_) {
		if((m_test_suite_filter_ == hxnull)
				|| (::strcmp(m_test_suite_filter_, (*it_)->suite_()) == 0)) {
			hxlogconsole("[ RUN      ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());
			m_current_test_ = *it_;
			m_test_state_ = test_state_nothing_asserted_;
			m_assert_count_ = 0u;

#ifdef __cpp_exceptions
			try
#endif
			{
				(*it_)->run_test_();

				// Expect the test to use another scope to reset the stack if needed.
				const size_t t_count = temporary_stack_base.get_current_allocation_count();
				const size_t t_bytes = temporary_stack_base.get_current_bytes_allocated();
				if((t_count != 0u) || (t_bytes != 0u)) {
					this->condition_check_(false, (*it_)->file_(), (*it_)->line_(),
						"test_leaks All tests must reset the temp stack.", true);
				}
			}
#ifdef __cpp_exceptions
			catch (...) {
				this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "unexpected_exception", true);
			}
#endif
			if(m_test_state_ == test_state_nothing_asserted_) {
				this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "nothing_tested", false);
			}
			if(m_test_state_ == test_state_pass_) {
				++m_pass_count_;
				hxlogconsole("[       OK ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());
			}
			else {
				++m_fail_count_;
				hxlogconsole("[  FAILED  ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());
			}
		}
	}
	m_current_test_ = hxnull;

	hxlogconsole("[==========] skipped %zu tests. failed %zu assertions.\n",
		m_num_test_cases_ - m_pass_count_ - m_fail_count_, m_total_assert_count_);

	hxwarnmsg(m_pass_count_ + m_fail_count_, "nothing_tested");

	if(m_pass_count_ != 0u && m_fail_count_ == 0u) {
		// This is Google Test style. If only it were green.
		hxloghandler(hxloglevel_console, "[  PASSED  ] %zu test%s.\n", m_pass_count_,
			((m_pass_count_ != 1u) ? "s" : ""));
	}
	else {
		hxloghandler(hxloglevel_console, "%zu FAILED TEST%s ❌\n", m_fail_count_,
			m_fail_count_ == 1u ? "" : "S");
		// Count nothing tested as one failure.
		m_fail_count_ = hxmax(m_fail_count_, (size_t)1u);
	}
	return m_fail_count_;
}

} // hxdetail_

#endif
