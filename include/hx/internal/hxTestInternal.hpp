#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxfile.hpp>

HX_STATIC_ASSERT(!HX_USE_GOOGLE_TEST, "Do not include directly");

struct hxtest_case_base_ {
	virtual void run_() = 0;
	virtual const char* suite_() = 0;
	virtual const char* case_() = 0;
	virtual const char* file_() = 0;
	virtual size_t line_() = 0;
};

struct hxtest_suite_executor_ {
public:
	enum Test_state_ {
		TEST_STATE_NOTHING_ASSERTED_,
		TEST_STATE_PASS_,
		TEST_STATE_FAIL_
	};

	enum {
		TEST_MAX_FAIL_MESSAGES_ = 5,
#if !defined HX_TEST_MAX_CASES
		HX_TEST_MAX_CASES = 1024
#endif
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxtest_suite_executor_& singleton_() { static hxtest_suite_executor_ s_hxtest_runner; return s_hxtest_runner; }

	hxtest_suite_executor_() {
		m_search_term_string_literal_ = hxnull;
		m_num_factories_ = 0;
		m_current_test_ = hxnull;
		::memset(m_factories_ + 0, 0x00, sizeof m_factories_);
	}

	void set_search_term_(const char* search_term_string_literal_) {
		m_search_term_string_literal_ = search_term_string_literal_;
	}

	void add_test_(hxtest_case_base_* fn_) {
		// Use -DHX_TEST_MAX_CASES to provide enough room for all tests.
		hxassert_release(m_num_factories_ < HX_TEST_MAX_CASES, "HX_TEST_MAX_CASES overflow\n");
		if(m_num_factories_ < HX_TEST_MAX_CASES) {
			m_factories_[m_num_factories_++] = fn_;
		}
	}

	// message is required to end with an \n. Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxfile& assert_check_(const char* file_, size_t line_, bool condition_, const char* message_) {
		hxassert_release(m_current_test_, "not testing");
		++m_assert_count_;
		m_test_state_ = (condition_ && m_test_state_ != TEST_STATE_FAIL_) ? TEST_STATE_PASS_ : TEST_STATE_FAIL_;
		if (!condition_) {
			if(++m_assert_fail_count_ >= TEST_MAX_FAIL_MESSAGES_) {
				if (m_assert_fail_count_ == TEST_MAX_FAIL_MESSAGES_) {
					hxlog_console("remaining asserts will fail silently...\n");
				}
				return file_null_();
			}

			// prints full path error messages that can be clicked on in an ide.
			hxlog_handler(hxlog_level_Assert, "%s.%s", m_current_test_->suite_(), m_current_test_->case_());
			hxlog_handler(hxlog_level_Assert, "%s(%zu): %s", file_, line_, message_);

			// Implements GTEST_FLAG_SET(break_on_failure, true);
#if (HX_TEST_ERROR_HANDLING) == 0 && (HX_RELEASE) == 0
			HX_BREAKPOINT();
#endif
			return file_log_();
		}
		return file_null_();
	}

	size_t execute_all_tests_() {
		hxinit(); // RUN_ALL_TESTS could be called first.
		m_pass_count_ = m_fail_count_ = m_assert_count_ = 0;
		hxlog_console("RUNNING_TESTS (%s)\n", (m_search_term_string_literal_ ? m_search_term_string_literal_ : "ALL"));
		for (hxtest_case_base_** it_ = m_factories_; it_ != (m_factories_ + m_num_factories_); ++it_) {
			if (!m_search_term_string_literal_ || ::strstr(m_search_term_string_literal_, (*it_)->suite_()) != hxnull) {
				hxlog_console("%s.%s...\n", (*it_)->suite_(), (*it_)->case_());

				m_current_test_ = *it_;
				m_test_state_ = TEST_STATE_NOTHING_ASSERTED_;
				m_assert_fail_count_ = 0;

#ifdef __cpp_exceptions
				try
#endif
				{
					// Tests should have no side effects. Therefore all allocations must be
					// safe to reset.
					hxmemory_allocator_scope temporary_stack(hxmemory_allocator_Temporary_stack);
					(*it_)->run_();
				}
#ifdef __cpp_exceptions
				catch (...) {
					this->assert_check_((*it_)->file_(), (*it_)->line_(), false, "unexpected exception");
				}
#endif

				if (m_test_state_ == TEST_STATE_NOTHING_ASSERTED_) {
					this->assert_check_((*it_)->file_(), (*it_)->line_(), false, "NOTHING_ASSERTED");
					++m_fail_count_;
				}
				else if (m_test_state_ == TEST_STATE_PASS_) {
					++m_pass_count_;
				}
				else {
					++m_fail_count_;
				}
			}
		}

		hxlog_console("skipped %zu tests. checked %zu assertions.\n",
			m_num_factories_ - m_pass_count_ - m_fail_count_, m_assert_count_);

		hxwarn_msg(m_pass_count_ + m_fail_count_, "NOTHING TESTED");

		if (m_pass_count_ != 0 && m_fail_count_ == 0) {
			hxlog_handler(hxlog_level_Console, "[  PASSED  ] %zu test%s.\n", m_pass_count_,
				((m_pass_count_ != 1) ? "s" : ""));
		}
		else {
			hxlog_handler(hxlog_level_Console, " %zu FAILED TEST%s\n", m_fail_count_,
				((m_fail_count_ != 1) ? "S" : ""));
			m_fail_count_ = hxmax(m_fail_count_, (size_t)1u); // Nothing tested is failure.
		}
		return m_fail_count_;
	}

private:
	hxfile& file_null_() { static hxfile f_(hxfile::out | hxfile::failable); return f_; }
	hxfile& file_log_()  { static hxfile f_(hxfile::out | hxfile::stdio); return f_; }

	hxtest_suite_executor_(const hxtest_suite_executor_&) HX_DELETE_FN;
	void operator=(const hxtest_suite_executor_&) HX_DELETE_FN;

	const char* m_search_term_string_literal_;
	hxtest_case_base_* m_factories_[HX_TEST_MAX_CASES];
	size_t m_num_factories_;
	hxtest_case_base_* m_current_test_;
	Test_state_ m_test_state_;
	size_t m_pass_count_;
	size_t m_fail_count_;
	size_t m_assert_count_;
	size_t m_assert_fail_count_;
};
