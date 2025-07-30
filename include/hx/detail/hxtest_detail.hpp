#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxfile.hpp>
#include <hx/hxsort.hpp>

static_assert(!HX_USE_GOOGLE_TEST, "Do not this file include directly");

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
bool hxtest_case_sort_(const hxtest_case_interface_* a_, const hxtest_case_interface_* b_);

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

	hxtest_(void);

	// Ensures constructor runs before tests are registered by global constructors.
	static hxtest_& dispatcher_(void);

	// Called by global constructors.
	void add_test_(hxtest_case_interface_* fn_);

	// Assert callback used by macros. message is required to end with an \n.
	// Returns equivalent of /dev/null on success and the system log otherwise.
	hxfile& condition_check_(bool condition_, const char* file_, size_t line_,
							 const char* message_, bool is_assert_);

	// Run tests. test_suite_filter_ must be identical.
	size_t run_all_tests_(const char* test_suite_filter_=hxnull);

private:
	hxfile& file_null_(void);
	hxfile& file_log_(void);

	hxtest_(const hxtest_&) = delete;
	void operator=(const hxtest_&) = delete;

	const char* m_test_suite_filter_;
	hxtest_case_interface_* m_test_cases_[HX_TEST_MAX_CASES];
	size_t m_num_test_cases_;
	hxtest_case_interface_* m_current_test_;
	test_state_t_ m_test_state_;
	size_t m_pass_count_;
	size_t m_fail_count_;
	size_t m_assert_fail_count_;
};

} // hxdetail_
using namespace hxdetail_;
