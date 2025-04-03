#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxFile.h>
#include <hx/hxProfiler.h>

// ----------------------------------------------------------------------------
// Enable this to use Google Test instead of hxTestRunner::executeAllTests().
#if HX_USE_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_USE_GOOGLE_TEST
#include <hx/internal/hxTestInternal.h>

// A partial Google Test reimplementation.
namespace testing {

class Test {
public:
	virtual void Run() = 0;
	virtual ~Test() {}
};

// Use hxTestRunner::setFilterStringLiteral() to filter tests.
HX_INLINE void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }
HX_INLINE void InitGoogleTest() { }

} // namespace testing

// TEST. Google Test reimplementation.
#define TEST(SuiteName_, CaseName_) \
	struct HX_CONCATENATE(SuiteName_, CaseName_) : hxTestRunner::TestFactoryBase { \
		struct Executor_ : testing::Test { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName_, CaseName_)() { hxTestRunner::singleton().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor_ executor_; executor_.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName_; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName_; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual size_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName_, CaseName_) HX_CONCATENATE(s_hxTest, CaseName_); \
	void HX_CONCATENATE(SuiteName_, CaseName_)::Executor_::Run()

// TEST_F.  Google Test reimplementation, SuiteName must be a subclass of testing::Test.
#define TEST_F(SuiteName_, CaseName_) \
	struct HX_CONCATENATE(SuiteName_, CaseName_) : hxTestRunner::TestFactoryBase { \
		struct Executor_ : SuiteName_ { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName_, CaseName_)() { hxTestRunner::singleton().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor_ executor_; executor_.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName_; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName_; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual size_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName_, CaseName_) HX_CONCATENATE(s_hxTest, CaseName_); \
	void HX_CONCATENATE(SuiteName_, CaseName_)::Executor_::Run()

// RUN_ALL_TESTS.
#define RUN_ALL_TESTS() hxTestRunner::singleton().executeAllTests()

// SUCCEED causes current test to pass instead of failing with NOTHING_ASSERTED.
#define SUCCEED() hxTestRunner::singleton().assertCheck(hxnull, 0, true, hxnull)

// FAIL causes current test to fail.
#define FAIL() hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, false, "failed here")

// EXPECT_*. Args are only evaluated once.  Uses operators '<' and '==' only.
#define EXPECT_TRUE(x_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, (x_), #x_)
#define EXPECT_FALSE(x_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, !(x_), "!" #x_)
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxTestRunner::singleton().assertCheck( \
	__FILE__, __LINE__, hxabs((expected_)-(actual_)) <= (absolute_range_), "abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
#define EXPECT_LT(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
#define EXPECT_GT(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, (rhs_) < (lhs_), #lhs_ ">" #rhs_)
#define EXPECT_LE(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, !((rhs_) < (lhs_)), #lhs_ "<=" #rhs_)
#define EXPECT_GE(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, !((lhs_) < (rhs_)), #lhs_ ">=" #rhs_)
#define EXPECT_EQ(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
#define EXPECT_NE(lhs_, rhs_) hxTestRunner::singleton().assertCheck(__FILE__, __LINE__, !((lhs_) == (rhs_)), #lhs_ "!=" #rhs_)

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
