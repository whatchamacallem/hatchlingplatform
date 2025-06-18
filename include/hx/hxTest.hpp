#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxTestSuiteExecutor_ - Enable this to use Google Test instead of hxTestSuiteExecutor_.
#if HX_USE_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_USE_GOOGLE_TEST
#include <hx/internal/hxTestInternal.hpp>

// testing::Test - A partial Google Test reimplementation. Use -DHX_TEST_MAX_CASES
// to provide enough room for all tests.

namespace testing {

// Test - Base class for tests required by TEST_F.
class Test {
public:
    // User overrides for fixtures.
    virtual ~Test() { };
    virtual void SetUp() { }
    virtual void TearDown() { };

    // Standard invocation protocol.
    void run_() {
        SetUp();
        runCode_();
        TearDown();
    }

    // Provided and used by the TEST_F macro.
    virtual void runCode_() = 0;
};

// InitGoogleTest - Initializes Google Test with command-line arguments. No-op in
// this implementation.
HX_CONSTEXPR_FN void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }

// InitGoogleTest - Overloaded version of InitGoogleTest with no arguments. No-op
// in this implementation.
HX_CONSTEXPR_FN void InitGoogleTest() { }

} // namespace testing

// TEST(suiteName, caseName) - Google Test reimplementation. Defines a test
// case with a suite name and case name.
// - suiteName: A C valid identifier for the test suite.
// - caseName: A C valid identifier for the test case.
#define TEST(suiteName_, caseName_) \
    struct HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) : public hxTestCaseBase_ { \
        HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)(void) { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
        virtual void run_(void) HX_OVERRIDE; \
        virtual const char* suite_(void) HX_OVERRIDE { return #suiteName_; } \
        virtual const char* case_(void) HX_OVERRIDE { return #caseName_; } \
        virtual const char* file_(void) HX_OVERRIDE { return __FILE__; } \
        virtual size_t line_(void) HX_OVERRIDE { return __LINE__; } \
    } static HX_CONCATENATE_3(s_hxTest_, suiteName_, caseName_); \
    void HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)::run_(void)

// TEST_F(suiteName, caseName) - Google Test reimplementation for fixture-based tests.
// Defines a test case where the suiteName is a subclass of testing::Test.
// - suiteName: A C valid identifier for the test suite.
// - caseName: A C valid identifier for the test case.
#define TEST_F(suiteName_, caseName_) \
    struct HX_CONCATENATE_3(hxTest_, suiteName_, caseName_) : public hxTestCaseBase_ { \
        struct hxTestCaseExecutor_ : public suiteName_ { virtual void runCode_(void) HX_OVERRIDE; }; \
        HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)(void) { hxTestSuiteExecutor_::singleton_().addTest_(this); } \
        virtual void run_(void) HX_OVERRIDE { hxTestCaseExecutor_ executor_; executor_.run_(); } \
        virtual const char* suite_(void) HX_OVERRIDE { return #suiteName_; } \
        virtual const char* case_(void) HX_OVERRIDE { return #caseName_; } \
        virtual const char* file_(void) HX_OVERRIDE { return __FILE__; } \
        virtual size_t line_(void) HX_OVERRIDE { return __LINE__; } \
    } static HX_CONCATENATE_3(s_hxTest, suiteName_, caseName_); \
    void HX_CONCATENATE_3(hxTest_, suiteName_, caseName_)::hxTestCaseExecutor_::runCode_(void)

// int RUN_ALL_TESTS() - Executes all registered test cases.
#define RUN_ALL_TESTS() hxTestSuiteExecutor_::singleton_().executeAllTests_()

// void SUCCEED() - Marks the current test as successful without any assertions.
#define SUCCEED() hxTestSuiteExecutor_::singleton_().assertCheck_(hxnull, 0, true, hxnull)

// void FAIL() - Marks the current test as failed.
#define FAIL() hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, false, "failed here")

// void EXPECT_TRUE(bool) - Asserts that the condition is true.
#define EXPECT_TRUE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (x_), #x_)
// void EXPECT_FALSE(bool) - Asserts that the condition is false.
#define EXPECT_FALSE(x_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !(x_), "!" #x_)

// void EXPECT_NEAR(T expected, T actual, T absolute_range) - Asserts that two values are within a given range.
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxTestSuiteExecutor_::singleton_().assertCheck_( \
    __FILE__, __LINE__,(((expected_) < (actual_)) ? ((actual_)-(expected_)) : ((expected_)-(actual_))) <= (absolute_range_), \
    "abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
// void EXPECT_LT(T lhs, T rhs) - Asserts lhs < rhs.
#define EXPECT_LT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
// void EXPECT_GT(T lhs, T rhs) - Asserts lhs > rhs.
#define EXPECT_GT(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (rhs_) < (lhs_), #lhs_ ">" #rhs_)
// void EXPECT_LE(T lhs, T rhs) - Asserts lhs <= rhs.
#define EXPECT_LE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((rhs_) < (lhs_)), #lhs_ "<=" #rhs_)
// void EXPECT_GE(T lhs, T rhs) - Asserts lhs >= rhs.
#define EXPECT_GE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) < (rhs_)), #lhs_ ">=" #rhs_)
// void EXPECT_EQ(T lhs, T rhs) - Asserts lhs == rhs.
#define EXPECT_EQ(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
// void EXPECT_NE(T lhs, T rhs) - Asserts lhs != rhs.
#define EXPECT_NE(lhs_, rhs_) hxTestSuiteExecutor_::singleton_().assertCheck_(__FILE__, __LINE__, !((lhs_) == (rhs_)), #lhs_ "!=" #rhs_)

// void ASSERT_TRUE(bool) - Asserts that the condition is true (equivalent to EXPECT_TRUE).
#define ASSERT_TRUE EXPECT_TRUE
// void ASSERT_FALSE(bool) - Asserts that the condition is false (equivalent to EXPECT_FALSE).
#define ASSERT_FALSE EXPECT_FALSE
// void ASSERT_NEAR(T expected, T actual, T absolute_range) - Asserts that two values are within a given range (equivalent to EXPECT_NEAR).
#define ASSERT_NEAR EXPECT_NEAR
// void ASSERT_LT(T lhs, T rhs) - Asserts lhs < rhs (equivalent to EXPECT_LT).
#define ASSERT_LT EXPECT_LT
// void ASSERT_GT(T lhs, T rhs) - Asserts lhs > rhs (equivalent to EXPECT_GT).
#define ASSERT_GT EXPECT_GT
// void ASSERT_LE(T lhs, T rhs) - Asserts lhs <= rhs (equivalent to EXPECT_LE).
#define ASSERT_LE EXPECT_LE
// void ASSERT_GE(T lhs, T rhs) - Asserts lhs >= rhs (equivalent to EXPECT_GE).
#define ASSERT_GE EXPECT_GE
// void ASSERT_EQ(T lhs, T rhs) - Asserts lhs == rhs (equivalent to EXPECT_EQ).
#define ASSERT_EQ EXPECT_EQ
// void ASSERT_NE(T lhs, T rhs) - Asserts lhs != rhs (equivalent to EXPECT_NE).
#define ASSERT_NE EXPECT_NE

#endif // !HX_USE_GOOGLE_TEST
