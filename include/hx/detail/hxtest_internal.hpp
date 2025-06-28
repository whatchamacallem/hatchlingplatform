#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxfile.hpp>
#include <hx/hxsort.hpp>

hxstatic_assert(!HX_USE_GOOGLE_TEST, "Do not this file include directly");

namespace hxdetail_ {

// hxtest_case_interface_ - Internal. Used to interrogate and dispatch tests.
class hxtest_case_interface_ {
public:
	virtual void run_() = 0;
	virtual const char* suite_() const = 0;
	virtual const char* case_() const = 0;
	virtual const char* file_() const = 0;
	virtual size_t line_() const = 0;
};

// hxtest_case_sort_ - Run tests in well defined alphanumeric order.
class hxtest_case_sort_ {
public:
	bool operator()(const hxtest_case_interface_* a_, const hxtest_case_interface_* b_) const {
		int compare_ = ::strcmp(a_->suite_(), b_->suite_());
		if(compare_ < 0) { return true; }
		if(compare_ == 0) { return ::strcmp(a_->case_(), b_->case_()) < 0; }
		return false;
	}
};

// hxtest_ - Internal. The test tracking and dispatching singleton.
class hxtest_ {
public:
	enum {
#if !defined HX_TEST_MAX_CASES
		// use -DHX_TEST_MAX_CASES=N to increase.
		HX_TEST_MAX_CASES = 1024,
#endif
		max_fail_messages_ = 5
	};

	enum test_state_t_ {
		test_state_nothing_asserted_,
		test_state_pass_,
		test_state_fail_
	};

	// Ensures constructor runs before tests are registered by global constructors.
	static hxtest_& dispatcher_(void) { static hxtest_ s_hxtest_runner; return s_hxtest_runner; }

	hxtest_(void) {
		m_search_term_string_literal_ = hxnull;
		m_num_test_cases_ = 0;
		m_current_test_ = hxnull;
		::memset(m_test_cases_ + 0, 0x00, sizeof m_test_cases_);
	}

	void set_search_term_(const char* search_term_string_literal_) {
		m_search_term_string_literal_ = search_term_string_literal_;
	}

	void add_test_(hxtest_case_interface_* fn_) {
		// Use -DHX_TEST_MAX_CASES to provide enough room for all tests.
		hxassertrelease(m_num_test_cases_ < HX_TEST_MAX_CASES, "HX_TEST_MAX_CASES overflow\n");
		if(m_num_test_cases_ < HX_TEST_MAX_CASES) {
			m_test_cases_[m_num_test_cases_++] = fn_;
		}
	}

	// message is required to end with an \n. Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxfile& condition_check_(bool condition_, const char* file_, size_t line_, const char* message_, bool is_assert_) {
		hxassertrelease(m_current_test_, "test_not_started");
		++m_assert_count_;
		m_test_state_ = (condition_ && m_test_state_ != test_state_fail_) ? test_state_pass_ : test_state_fail_;
		if (!condition_) {
			if(++m_assert_fail_count_ >= max_fail_messages_) {
				if (m_assert_fail_count_ == max_fail_messages_) {
					hxlogconsole("remaining asserts will fail silently...\n");
				}
				return this->file_null_();
			}

			// prints full path error messages that can be clicked on in an ide.
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
			return this->file_log_();
		}
		return this->file_null_();
	}

	size_t run_all_tests_(void) {
		hxinit(); // RUN_ALL_TESTS could be called first.

		hxinsertion_sort(m_test_cases_, m_test_cases_ + m_num_test_cases_, hxtest_case_sort_());

		m_pass_count_ = m_fail_count_ = m_assert_count_ = 0;
		hxlogconsole("[==========] Running tests %s\n", (m_search_term_string_literal_ ? m_search_term_string_literal_ : "all"));
		for (hxtest_case_interface_** it_ = m_test_cases_; it_ != (m_test_cases_ + m_num_test_cases_); ++it_) {
			if (!m_search_term_string_literal_ || ::strstr(m_search_term_string_literal_, (*it_)->suite_()) != hxnull) {
				hxlogconsole("[ RUN      ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());

				m_current_test_ = *it_;
				m_test_state_ = test_state_nothing_asserted_;
				m_assert_fail_count_ = 0;

#ifdef __cpp_exceptions
				try
#endif
				{
					// Tests should have no side effects. Therefore all allocations must be
					// safe to reset.
					hxsystem_allocator_scope temporary_stack(hxsystem_allocator_temporary_stack);
					(*it_)->run_();
				}
#ifdef __cpp_exceptions
				catch (...) {
					this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "unexpected_exception", true);
				}
#endif

				if (m_test_state_ == test_state_nothing_asserted_) {
					this->condition_check_(false, (*it_)->file_(), (*it_)->line_(), "nothing_tested", false);
				}
				if (m_test_state_ == test_state_pass_) {
					++m_pass_count_;
					hxlogconsole("[       OK ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());
				}
				else {
					++m_fail_count_;
					hxlogconsole("[  FAILED  ] %s.%s\n", (*it_)->suite_(), (*it_)->case_());
				}
			}
		}

		hxlogconsole("[==========] skipped %zu tests. checked %zu assertions.\n",
			m_num_test_cases_ - m_pass_count_ - m_fail_count_, m_assert_count_);

		hxwarnmsg(m_pass_count_ + m_fail_count_, "nothing_tested");

		if (m_pass_count_ != 0 && m_fail_count_ == 0) {
			// This is Google Test style.  If only it were green.
			hxloghandler(hxloglevel_console, "[  PASSED  ] %zu test%s.\n", m_pass_count_,
				((m_pass_count_ != 1) ? "s" : ""));
		}
		else {
			hxloghandler(hxloglevel_console, "%zu FAILED TEST%s ❌\n", m_fail_count_,
				m_fail_count_ == 1 ? "" : "S");
			// Count nothing tested as 1 failure.
			m_fail_count_ = hxmax(m_fail_count_, (size_t)1u);
		}
		return m_fail_count_;
	}

private:
	hxfile& file_null_(void) {
		static hxfile f_(hxfile::out | hxfile::failable); return f_;
	}
	hxfile& file_log_(void) {
		static hxfile f_(hxfile::out | hxfile::stdio); return f_;
	}

	hxtest_(const hxtest_&) hxdelete_fn;
	void operator=(const hxtest_&) hxdelete_fn;

	const char* m_search_term_string_literal_;
	hxtest_case_interface_* m_test_cases_[HX_TEST_MAX_CASES];
	size_t m_num_test_cases_;
	hxtest_case_interface_* m_current_test_;
	test_state_t_ m_test_state_;
	size_t m_pass_count_;
	size_t m_fail_count_;
	size_t m_assert_count_;
	size_t m_assert_fail_count_;
};

} // hxdetail_
using namespace hxdetail_;
