#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../hxfile.hpp"
#include "../hxsort.hpp"

static_assert(!HX_USE_GOOGLE_TEST, "Do not include this file directly.");

namespace hxdetail_ {

// hxtest_case_interface_ - Internal. Used to interrogate and dispatch tests.
class hxtest_case_interface_ {
public:
	virtual void run_test_() = 0;
	virtual const char* suite_() const = 0;
	virtual const char* case_() const = 0;
	virtual const char* file_() const = 0;
	virtual size_t line_() const = 0;
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

	enum test_state_t_ : uint8_t {
		test_state_nothing_asserted_,
		test_state_pass_,
		test_state_fail_
	};

	hxtest_(void);

	// Ensures constructor runs before tests are registered by global constructors.
	static hxtest_& dispatcher_(void);

	// Called by global constructors.
	void add_test_(hxtest_case_interface_* fn_) hxattr_nonnull(2);

	// Assert callback used by macros. Returns equivalent of /dev/null on
	// success and the system log otherwise.
	hxfile& condition_check_(bool condition_, const char* file_, size_t line_,
							 const char* message_, bool is_assert_) hxattr_nonnull(3,5);

	// Run tests. test_suite_filter_ must be identical.
	size_t run_all_tests_(const char* test_suite_filter_=hxnull);

private:
	hxtest_(const hxtest_&) = delete;
	void operator=(const hxtest_&) = delete;

	const char* m_test_suite_filter_;
	hxtest_case_interface_* m_test_cases_[HX_TEST_MAX_CASES];
	size_t m_num_test_cases_;
	hxtest_case_interface_* m_current_test_;
	test_state_t_ m_test_state_;
	size_t m_pass_count_;
	size_t m_fail_count_;
	size_t m_total_assert_count_;
	size_t m_assert_count_;
};

} // hxdetail_
using namespace hxdetail_;
