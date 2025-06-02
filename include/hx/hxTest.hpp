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

// Base class for tests required by TEST_F.
class Test {
public:
	// User overrides for fixtures.
	virtual ~Test() { };
	virtual void SetUp() {}
	virtual void TearDown() {};

	// Provided and used by the TEST_F macro.
	virtual void runCode_() = 0;

	// Used by the TEST_F macro.
	void run_() {
		SetUp();
		runCode_();
		TearDown();
	}
};

// Initializes Google Test with command-line arguments. No-op in this implementation.
HX_CONSTEXPR_FN void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }

// Overloaded version of InitGoogleTest with no arguments. No-op in this implementation.
HX_CONSTEXPR_FN void InitGoogleTest() { }

} // namespace testing

// TEST. Google Test reimplementation.
// Defines a test case with a suite name and case name.
#define TEST(suiteName_, caseName_) \
	struct HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) : public hxTestCaseBase_ { \
		HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)() { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
		virtual void run_() HX_OVERRIDE; \
		virtual const char* suite_() HX_OVERRIDE { return #suiteName_; } \
		virtual const char* case_() HX_OVERRIDE { return #caseName_; } \
		virtual const char* file_() HX_OVERRIDE { return __FILE__; } \
		virtual size_t line_() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) HX_CONCATENATE_3(s_hxTest_, suiteName_, caseName_); \
	void HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)::run_()

// TEST_F. Google Test reimplementation for fixture-based tests.
// Defines a test case where the suite is a subclass of testing::Test.
#define TEST_F(suiteName_, caseName_) \
	struct HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) : public hxTestCaseBase_ { \
		struct hxTestCaseExecutor_ : suiteName_ { virtual void runCode_() HX_OVERRIDE; }; \
		HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)() { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
		virtual void run_() HX_OVERRIDE { hxTestCaseExecutor_ executor_; executor_.run_(); } \
		virtual const char* suite_() HX_OVERRIDE { return #suiteName_; } \
		virtual const char* case_() HX_OVERRIDE { return #caseName_; } \
		virtual const char* file_() HX_OVERRIDE { return __FILE__; } \
		virtual size_t line_() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) HX_CONCATENATE_3(s_hxTest, suiteName_, caseName_); \
	void HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)::hxTestCaseExecutor_::runCode_()

// RUN_ALL_TESTS. Executes all registered test cases.
#define RUN_ALL_TESTS() hxTestSuiteExecutor_::singleton_().executeAllTests_()

// SUCCEED. Marks the current test as successful without any assertions.
#define SUCCEED() hxTestSuiteExecutor_::singleton_().assertCheck_(hxnull, 0, true, hxnull)

// FAIL. Marks the current test as failed.
#define FAIL() hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, false, "failed here")

// EXPECT_*. Assertions for various conditions.
#define EXPECT_TRUE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (x_), #x_)
#define EXPECT_FALSE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !(x_), "!" #x_)

// Works with unsigned types.
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxTestSuiteExecutor_::singleton_().assertCheck_( \
	__FILE__, __LINE__,(((expected_) < (actual_)) ? ((actual_)-(expected_)) : ((expected_)-(actual_))) <= (absolute_range_), \
	"abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
#define EXPECT_LT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
#define EXPECT_GT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (rhs_) < (lhs_), #lhs_ ">" #rhs_)
#define EXPECT_LE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((rhs_) < (lhs_)), #lhs_ "<=" #rhs_)
#define EXPECT_GE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) < (rhs_)), #lhs_ ">=" #rhs_)
#define EXPECT_EQ(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
#define EXPECT_NE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) == (rhs_)), #lhs_ "!=" #rhs_)

// ASSERT_*. Assertions that are equivalent to EXPECT_* in this implementation.
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
// hxTestRandom. The linear congruential random number generator from Numerical Recipes.
struct hxTestRandom {
public:
	// Constructor to initialize the random number generator with a seed.
	// Parameters:
	// - seed_: Initial seed value for the random number generator.
	HX_CONSTEXPR_FN hxTestRandom(uint32_t seed_ = 1u) : m_seed(seed_) { }

	// Generates the next random number in the sequence.
	// Returns:
	// - The next random number as a 32-bit unsigned integer.
	HX_CONSTEXPR_FN uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }

	// Current seed value used for generating random numbers.
	uint32_t m_seed;
};
