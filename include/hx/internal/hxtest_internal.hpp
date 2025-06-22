#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxfile.hpp>

hxstatic_assert(!HX_USE_GOOGLE_TEST, "Do not include directly");

namespace hxx_ {

class hxtest_case_interface_ {
public:
	virtual void run_() = 0;
	virtual const char* suite_() = 0;
	virtual const char* case_() = 0;
	virtual const char* file_() = 0;
	virtual size_t line_() = 0;
};

class hxtest_suite_executor_ {
public:
	enum test_state_t_ {
		test_state_nothing_asserted_,
		test_state_pass_,
		test_state_fail_
	};

	enum {
#if !defined hxtest_max_cases
		// use -Dhxtest_max_cases=N to increase.
		hxtest_max_cases = 1024,
#endif
		max_fail_messages_ = 5
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

	void add_test_(hxtest_case_interface_* fn_) {
		// Use -DHX_TEST_MAX_CASES to provide enough room for all tests.
		hxassertrelease(m_num_factories_ < hxtest_max_cases, "hxtest_max_cases overflow\n");
		if(m_num_factories_ < hxtest_max_cases) {
			m_factories_[m_num_factories_++] = fn_;
		}
	}

	// message is required to end with an \n. Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxfile& condition_check_(bool condition_, const char* file_, size_t line_, const char* message_, bool critical_) {
		hxassertrelease(m_current_test_, "not testing");
		++m_assert_count_;
		m_test_state_ = (condition_ && m_test_state_ != test_state_fail_) ? test_state_pass_ : test_state_fail_;
		if (!condition_) {
			if(++m_assert_fail_count_ >= max_fail_messages_) {
				if (m_assert_fail_count_ == max_fail_messages_) {
					hxlogconsole("remaining asserts will fail silently...\n");
				}
				return file_null_();
			}

			// prints full path error messages that can be clicked on in an ide.
			hxloghandler(hxloglevel_assert, "%s.%s", m_current_test_->suite_(), m_current_test_->case_());
			hxloghandler(hxloglevel_assert, "%s(%zu): %s", file_, line_, message_);

			if(critical_) {
				// ASSERT_* macros halt the test suite on failure.
				hxloghandler(hxloglevel_assert, "stopping due to assert.");
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
			return file_log_();
		}
		return file_null_();
	}

	size_t execute_all_tests_() {
		hxinit(); // RUN_ALL_TESTS could be called first.
		m_pass_count_ = m_fail_count_ = m_assert_count_ = 0;
		hxlogconsole("RUNNING_TESTS (%s)\n", (m_search_term_string_literal_ ? m_search_term_string_literal_ : "ALL"));
		for (hxtest_case_interface_** it_ = m_factories_; it_ != (m_factories_ + m_num_factories_); ++it_) {
			if (!m_search_term_string_literal_ || ::strstr(m_search_term_string_literal_, (*it_)->suite_()) != hxnull) {
				hxlogconsole("%s.%s...\n", (*it_)->suite_(), (*it_)->case_());

				m_current_test_ = *it_;
				m_test_state_ = test_state_nothing_asserted_;
				m_assert_fail_count_ = 0;

#ifdef __cpp_exceptions
				try
#endif
				{
					// Tests should have no side effects. Therefore all allocations must be
					// safe to reset.
					hxmemory_allocator_scope temporary_stack(hxmemory_allocator_temporary_stack);
					(*it_)->run_();
				}
#ifdef __cpp_exceptions
				catch (...) {
					this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "unexpected exception", true);
				}
#endif

				if (m_test_state_ == test_state_nothing_asserted_) {
					this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "NOTHING_ASSERTED", false);
					++m_fail_count_;
				}
				else if (m_test_state_ == test_state_pass_) {
					++m_pass_count_;
				}
				else {
					++m_fail_count_;
				}
			}
		}

		hxlogconsole("skipped %zu tests. checked %zu assertions.\n",
			m_num_factories_ - m_pass_count_ - m_fail_count_, m_assert_count_);

		hxwarnmsg(m_pass_count_ + m_fail_count_, "NOTHING TESTED");

		if (m_pass_count_ != 0 && m_fail_count_ == 0) {
			hxloghandler(hxloglevel_console, "[  PASSED  ] %zu test%s.\n", m_pass_count_,
				((m_pass_count_ != 1) ? "s" : ""));
		}
		else {
			hxloghandler(hxloglevel_console, " %zu FAILED TEST%s\n", m_fail_count_,
				((m_fail_count_ != 1) ? "S" : ""));
			m_fail_count_ = hxmax(m_fail_count_, (size_t)1u); // Nothing tested is failure.
		}
		return m_fail_count_;
	}

private:
	hxfile& file_null_() { static hxfile f_(hxfile::out | hxfile::failable); return f_; }
	hxfile& file_log_()  { static hxfile f_(hxfile::out | hxfile::stdio); return f_; }

	hxtest_suite_executor_(const hxtest_suite_executor_&) hxdelete_fn;
	void operator=(const hxtest_suite_executor_&) hxdelete_fn;

	const char* m_search_term_string_literal_;
	hxtest_case_interface_* m_factories_[hxtest_max_cases];
	size_t m_num_factories_;
	hxtest_case_interface_* m_current_test_;
	test_state_t_ m_test_state_;
	size_t m_pass_count_;
	size_t m_fail_count_;
	size_t m_assert_count_;
	size_t m_assert_fail_count_;
};

} // namespace hxx_
using namespace hxx_;
