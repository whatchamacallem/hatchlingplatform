#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

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

// Use hxTestRunner::setFilterStringLiteral() instead.
HX_INLINE void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }
HX_INLINE void InitGoogleTest() { }

} // namespace testing

// TEST. Google Test reimplementation.
#define TEST(SuiteName_, CaseName_) \
	struct HX_CONCATENATE(SuiteName_, CaseName_) : hxTestRunner::FactoryBase { \
		struct Executor_ : testing::Test { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName_, CaseName_)() { hxTestRunner::get().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor_ executor; executor.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName_; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName_; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual int32_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName_, CaseName_) HX_CONCATENATE(s_hxTest, CaseName_); \
	void HX_CONCATENATE(SuiteName_, CaseName_)::Executor_::Run()

// TEST_F.  Google Test reimplementation, SuiteName_ must be a subclass of testing::Test.
#define TEST_F(SuiteName_, CaseName_) \
	struct HX_CONCATENATE(SuiteName_, CaseName_) : hxTestRunner::FactoryBase { \
		struct Executor_ : SuiteName_ { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName_, CaseName_)() { hxTestRunner::get().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor_ executor; executor.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName_; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName_; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual int32_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName_, CaseName_) HX_CONCATENATE(s_hxTest, CaseName_); \
	void HX_CONCATENATE(SuiteName_, CaseName_)::Executor_::Run()

#define RUN_ALL_TESTS() hxTestRunner::get().executeAllTests()
#define SUCCEED() hxTestRunner::get().assertCheck(hxnull, 0, true, hxnull) // Avoids NOTHING ASSERTED.
#define FAIL() hxTestRunner::get().assertCheck(__FILE__, __LINE__, false, "failed here")

// Args are only evaluated once.
#define ASSERT_TRUE(x_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (x_), #x_)
#define ASSERT_FALSE(x_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, !(x_), "!" #x_)
#define ASSERT_NEAR(expected_, actual_, absolute_range_) hxTestRunner::get().assertCheck( \
	__FILE__, __LINE__, hxAbs((expected_)-(actual_)) <= (absolute_range_), "abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
#define ASSERT_LT(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
#define ASSERT_GT(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) > (rhs_), #lhs_ ">" #rhs_)
#define ASSERT_LE(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) <= (rhs_), #lhs_ "<=" #rhs_)
#define ASSERT_GE(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) >= (rhs_), #lhs_ ">=" #rhs_)
#define ASSERT_EQ(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
#define ASSERT_NE(lhs_, rhs_) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs_) != (rhs_), #lhs_ "!=" #rhs_)

#define EXPECT_TRUE ASSERT_TRUE
#define EXPECT_FALSE ASSERT_FALSE
#define EXPECT_NEAR ASSERT_NEAR
#define EXPECT_LT ASSERT_LT
#define EXPECT_GT ASSERT_GT
#define EXPECT_LE ASSERT_LE
#define EXPECT_GE ASSERT_GE
#define EXPECT_EQ ASSERT_EQ
#define EXPECT_NE ASSERT_NE

#endif // !HX_USE_GOOGLE_TEST

// ----------------------------------------------------------------------------
// hxTestRandom.  The linear congruential random number generator from Numerical Recipes.
struct hxTestRandom {
public:
	HX_INLINE hxTestRandom(uint32_t seed_ = 1u) : m_seed(seed_) { }
	HX_INLINE uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }
	uint32_t m_seed;
};
