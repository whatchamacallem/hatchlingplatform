#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>
#include <hx/hxProfiler.h>
#include <hx/hxFile.h>

// ----------------------------------------------------------------------------
// Enable this to use Google Test instead of hxTestRunner::executeAllTests().
#if HX_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_GOOGLE_TEST
#include <hx/internal/hxTestInternal.h>

// A partial Google Test reimplementation.
namespace testing {

class Test {
public:
	virtual void Run(void) = 0;
	virtual ~Test() {}
};

// Use hxTestRunner::setFilterStringLiteral() instead.
HX_INLINE void InitGoogleTest(int *argc, char **argv) { (void)argc; (void)argv; }
HX_INLINE void InitGoogleTest() { }

} // namespace testing

// TEST. GoogleTest reimplementation.
#define TEST(SuiteName, CaseName) \
	struct HX_CONCATENATE(SuiteName, CaseName) : hxTestRunner::FactoryBase { \
		struct Executor : testing::Test { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName, CaseName)() { hxTestRunner::get().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor executor; executor.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual int32_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName, CaseName) HX_CONCATENATE(s_hxTest, CaseName); \
	void HX_CONCATENATE(SuiteName, CaseName)::Executor::Run(void)

// TEST_F.  GoogleTest reimplementation, SuiteName must be a subclass of testing::Test.
#define TEST_F(SuiteName, CaseName) \
	struct HX_CONCATENATE(SuiteName, CaseName) : hxTestRunner::FactoryBase { \
		struct Executor : SuiteName { virtual void Run() HX_OVERRIDE; }; \
		HX_CONCATENATE(SuiteName, CaseName)() { hxTestRunner::get().addTest(this); } \
		virtual void Run() HX_OVERRIDE { Executor executor; executor.Run(); } \
		virtual const char* Suite() HX_OVERRIDE { return #SuiteName; } \
		virtual const char* Case() HX_OVERRIDE { return #CaseName; } \
		virtual const char* File() HX_OVERRIDE { return __FILE__; } \
		virtual int32_t Line() HX_OVERRIDE { return __LINE__; } \
	}; \
	static HX_CONCATENATE(SuiteName, CaseName) HX_CONCATENATE(s_hxTest, CaseName); \
	void HX_CONCATENATE(SuiteName, CaseName)::Executor::Run(void)

#define RUN_ALL_TESTS() hxTestRunner::get().executeAllTests()
#define SUCCEED() hxTestRunner::get().assertCheck(hxnull, 0, true, hxnull) // Avoids NOTHING ASSERTED.
#define FAIL() hxTestRunner::get().assertCheck(__FILE__, __LINE__, false, "test fail\n")

// Args are only evaluated once.
#define ASSERT_TRUE(x) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (x), #x "\n")
#define ASSERT_FALSE(x) hxTestRunner::get().assertCheck(__FILE__, __LINE__, !(x), "!" #x "\n")
#define ASSERT_NEAR(expected, actual, absolute_range) hxTestRunner::get().assertCheck(__FILE__, __LINE__, hxAbs((expected)-(actual)) <= (absolute_range), "abs(" #expected "-" #actual ")<=" #absolute_range "\n")
#define ASSERT_LT(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) < (rhs), #lhs "<" #rhs "\n")
#define ASSERT_GT(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) > (rhs), #lhs ">" #rhs "\n")
#define ASSERT_LE(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) <= (rhs), #lhs "<=" #rhs "\n")
#define ASSERT_GE(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) >= (rhs), #lhs ">=" #rhs "\n")
#define ASSERT_EQ(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) == (rhs), #lhs "==" #rhs "\n")
#define ASSERT_NE(lhs, rhs) hxTestRunner::get().assertCheck(__FILE__, __LINE__, (lhs) != (rhs), #lhs "!=" #rhs "\n")

#define EXPECT_TRUE ASSERT_TRUE
#define EXPECT_FALSE ASSERT_FALSE
#define EXPECT_NEAR ASSERT_NEAR
#define EXPECT_LT ASSERT_LT
#define EXPECT_GT ASSERT_GT
#define EXPECT_LE ASSERT_LE
#define EXPECT_GE ASSERT_GE
#define EXPECT_EQ ASSERT_EQ
#define EXPECT_NE ASSERT_NE

#endif // !HX_GOOGLE_TEST

// ----------------------------------------------------------------------------
// hxTestRandom.  The linear congruential random number generator from Numerical Recipes.
struct hxTestRandom {
public:
	HX_INLINE hxTestRandom(uint32_t seed = 1u) : m_seed(seed) { }
	HX_INLINE uint32_t operator()() { return (m_seed = 1664525u * m_seed + 1013904223u); }
	uint32_t m_seed;
};
