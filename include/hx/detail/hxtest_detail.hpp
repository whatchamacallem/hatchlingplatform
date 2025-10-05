#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../hxfile.hpp"
#include "../hxsort.hpp"

static_assert(!HX_USE_GOOGLE_TEST, "Do not include this file directly.");

namespace hxdetail_ {

// hxtest_*_eq_ — ULP-based floating point equality (GoogleTest-compatible)
// Compares IEEE-754 numbers by **Units in the Last Place (ULPs)** with a fixed
// threshold of **4 ULPs** for gtest’s `EXPECT_FLOAT_EQ` / `EXPECT_DOUBLE_EQ`.
// Unlike Google Test this rejects any infinite values on policy.
static inline bool hxtest_float_eq_(float a_, float b_) {
    if (!hxisfinitef(a_) || !hxisfinitef(b_)) { return false; }
    if (a_ == b_) { return true; }

    uint32_t ua_; memcpy(&ua_, &a_, sizeof ua_);
    uint32_t ub_; memcpy(&ub_, &b_, sizeof ub_);

    const uint32_t sign_mask_ = 1u << 31;
    const uint32_t ba_ = (ua_ & sign_mask_) ? (~ua_ + (uint32_t)1) : (sign_mask_ | ua_);
    const uint32_t bb_ = (ub_ & sign_mask_) ? (~ub_ + (uint32_t)1) : (sign_mask_ | ub_);
    const uint32_t delta_ = (ba_ >= bb_) ? (ba_ - bb_) : (bb_ - ba_);

    return delta_ <= 4u; // 4 ULPs.
}

static inline bool hxtest_double_eq_(double a_, double b_) {
    if (!hxisfinitel(a_) || !hxisfinitel(b_)) { return false; }
    if (a_ == b_) { return true; }

    uint64_t ua_; memcpy(&ua_, &a_, sizeof ua_);
    uint64_t ub_; memcpy(&ub_, &b_, sizeof ub_);

    const uint64_t sign_mask_ = (1ull << 63);
    const uint64_t ba_ = (ua_ & sign_mask_) ? (~ua_ + 1ull) : (sign_mask_ | ua_);
    const uint64_t bb_ = (ub_ & sign_mask_) ? (~ub_ + 1ull) : (sign_mask_ | ub_);
    const uint64_t delta_ = (ba_ >= bb_) ? (ba_ - bb_) : (bb_ - ba_);

    return delta_ <= 4u; // 4 ULPs.
}

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
