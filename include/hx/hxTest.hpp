#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// Enable this to use Google Test instead of hxTestSuiteExecutor_.
#if HX_USE_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_USE_GOOGLE_TEST
#include <hx/internal/hxTestInternal.hpp>

// A partial Google Test reimplementation.  Use -DHX_TEST_MAX_CASES to provide
// enough room for all tests.
// Use hxTestSuiteExecutor_::setFilterStringLiteral() to filter tests.

namespace testing {

class Test {
public:
	virtual void run_() = 0;
	virtual ~Test() {}
};

HX_INLINE void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }
HX_INLINE void InitGoogleTest() { }

} // namespace testing

// TEST. Google Test reimplementation.
#define TEST(suiteName_, caseName_) \
	struct HX_CONCATENATE(suiteName_, caseName_) : hxTestCaseBase_ { \
		struct hxTestCaseExecutor_ : testing::Test { virtual void run_() HX_OVERRIDE; }; \
		HX_CONCATENATE(suiteName_, caseName_)() { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
		virtual void run_() HX_OVERRIDE { hxTestCaseExecutor_ executor_; executor_.run_(); } \
		virtual const char* suite_() HX_OVERRIDE { return #suiteName_; } \
		virtual const char* case_() HX_OVERRIDE { return #caseName_; } \
		virtual const char* file_() HX_OVERRIDE { return __FILE__; } \
		virtual size_t line_() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(suiteName_, caseName_) HX_CONCATENATE(s_hxTest, caseName_); \
	void HX_CONCATENATE(suiteName_, caseName_)::hxTestCaseExecutor_::run_()

// TEST_F.  Google Test reimplementation, SuiteName must be a subclass of testing::Test.
#define TEST_F(suiteName_, caseName_) \
	struct HX_CONCATENATE(suiteName_, caseName_) : hxTestCaseBase_ { \
		struct hxTestCaseExecutor_ : suiteName_ { virtual void run_() HX_OVERRIDE; }; \
		HX_CONCATENATE(suiteName_, caseName_)() { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
		virtual void run_() HX_OVERRIDE { hxTestCaseExecutor_ executor_; executor_.run_(); } \
		virtual const char* suite_() HX_OVERRIDE { return #suiteName_; } \
		virtual const char* case_() HX_OVERRIDE { return #caseName_; } \
		virtual const char* file_() HX_OVERRIDE { return __FILE__; } \
		virtual size_t line_() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(suiteName_, caseName_) HX_CONCATENATE(s_hxTest, caseName_); \
	void HX_CONCATENATE(suiteName_, caseName_)::hxTestCaseExecutor_::run_()

// RUN_ALL_TESTS.
#define RUN_ALL_TESTS() hxTestSuiteExecutor_::singleton_().executeAllTests_()

// SUCCEED causes current test to pass instead of failing with NOTHING_ASSERTED.
#define SUCCEED() hxTestSuiteExecutor_::singleton_().assertCheck_(hxnull, 0, true, hxnull)

// FAIL causes current test to fail.
#define FAIL() hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, false, "failed here")

// EXPECT_*. Args are only evaluated once.  Uses operators '<' and '==' only.
#define EXPECT_TRUE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (x_), #x_)
#define EXPECT_FALSE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !(x_), "!" #x_)
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxTestSuiteExecutor_::singleton_().assertCheck_( \
	__FILE__, __LINE__, hxabs((expected_)-(actual_)) <= (absolute_range_), "abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
#define EXPECT_LT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
#define EXPECT_GT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (rhs_) < (lhs_), #lhs_ ">" #rhs_)
#define EXPECT_LE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((rhs_) < (lhs_)), #lhs_ "<=" #rhs_)
#define EXPECT_GE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) < (rhs_)), #lhs_ ">=" #rhs_)
#define EXPECT_EQ(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
#define EXPECT_NE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) == (rhs_)), #lhs_ "!=" #rhs_)

// Without using exceptions ASSERT_* becomes EXPECT_*.
#define ASSERT_TRUE EXPECT_TRUE
#define ASSERT_FALSE EXPECT_FALSE
#define ASSERT_NEAR EXPECT_NEAR
#define ASSERT_LT EXPECT_LT
#define ASSERT_GT EXPECT_GT
#define ASSERT_LE EXPECT_LE
#define ASSERT_GE EXPECT_GE
#define ASSERT_EQ EXPECT_EQ
#define ASSERT_NE EXPECT_NE

#endif // !HX_USE_GOOGLE_TEST

// ----------------------------------------------------------------------------
// hxTestRandom.  The linear congruential random number generator from Numerical Recipes.
struct hxTestRandom {
public:
	HX_INLINE hxTestRandom(uint32_t seed_ = 1u) : m_seed(seed_) { }
	HX_INLINE uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }
	uint32_t m_seed;
};
