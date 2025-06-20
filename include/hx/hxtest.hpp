#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// HX_USE_GOOGLE_TEST - Enable this to use Google Test instead of hxtest_suite_executor_.
#if HX_USE_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_USE_GOOGLE_TEST
#include <hx/internal/hxtest_internal.hpp>

// testing::Test - A partial Google Test reimplementation. Use -DHX_TEST_MAX_CASES
// to provide enough room for all tests.

namespace testing {

// Test - Base class for tests required by Google Test's TEST_F.
class Test {
public:
    // User overrides for fixtures.
    virtual ~Test() { };
    virtual void Set_up() { }
    virtual void Tear_down() { };

    // Standard invocation protocol.
    void run_() {
        Set_up();
        run_code_();
        Tear_down();
    }

    // Provided and used by the TEST_F macro.
    virtual void run_code_() = 0;
};

// InitGoogleTest - Initializes Google Test with command-line arguments. No-op in
// this implementation.
hxconstexpr_fn void InitGoogleTest(int *argc_, char **argv_) { (void)argc_; (void)argv_; }

// InitGoogleTest - Overloaded version of InitGoogleTest with no arguments. No-op
// in this implementation.
hxconstexpr_fn void InitGoogleTest() { }

} // namespace testing

// HX_TEST_NAME_ - Macro for concatenating 3 arguments into one name.
// Macro parameters will be evaluated before concatenating.
#define HX_TEST_NAME_(x_, y_, z_) x_ ## y_ ## z_

// TEST(suite_name, case_name) - Google Test reimplementation. Defines a test
// case with a suite name and case name.
// - suite_name: A C valid identifier for the test suite.
// - case_name: A C valid identifier for the test case.
#define TEST(suite_name_, case_name_) \
    class HX_TEST_NAME_(hxtest_, suite_name_, case_name_) : public hxtest_case_interface_ { \
    public: \
        HX_TEST_NAME_(hxtest_, suite_name_, case_name_)(void) { hxtest_suite_executor_::singleton_().add_test_(this); } \
        virtual void run_(void) hxoverride; \
        virtual const char* suite_(void) hxoverride { return #suite_name_; } \
        virtual const char* case_(void) hxoverride { return #case_name_; } \
        virtual const char* file_(void) hxoverride { return __FILE__; } \
        virtual size_t line_(void) hxoverride { return __LINE__; } \
    } static HX_TEST_NAME_(s_hxtest_, suite_name_, case_name_); \
    void HX_TEST_NAME_(hxtest_, suite_name_, case_name_)::run_(void)

// TEST_F(suite_name, case_name) - Google Test reimplementation for fixture-based tests.
// Defines a test case where the suite_name is a subclass of testing::Test.
// - suite_name: A C valid identifier for the test suite.
// - case_name: A C valid identifier for the test case.
#define TEST_F(suite_name_, case_name_) \
    class HX_TEST_NAME_(hxtest_, suite_name_, case_name_) : public hxtest_case_interface_ { \
    public: \
        class hxtest_case_executor_ : public suite_name_ { virtual void run_code_(void) hxoverride; }; \
        HX_TEST_NAME_(hxtest_, suite_name_, case_name_)(void) { hxtest_suite_executor_::singleton_().add_test_(this); } \
        virtual void run_(void) hxoverride { hxtest_case_executor_ executor_; executor_.run_(); } \
        virtual const char* suite_(void) hxoverride { return #suite_name_; } \
        virtual const char* case_(void) hxoverride { return #case_name_; } \
        virtual const char* file_(void) hxoverride { return __FILE__; } \
        virtual size_t line_(void) hxoverride { return __LINE__; } \
    } static HX_TEST_NAME_(s_hxtest, suite_name_, case_name_); \
    void HX_TEST_NAME_(hxtest_, suite_name_, case_name_)::hxtest_case_executor_::run_code_(void)

// int RUN_ALL_TESTS() - Executes all registered test cases.
#define RUN_ALL_TESTS() hxtest_suite_executor_::singleton_().execute_all_tests_()

// void SUCCEED() - Marks the current test as successful without any assertions.
#define SUCCEED() hxtest_suite_executor_::singleton_().assert_check_(hxnull, 0, true, hxnull)

// void FAIL() - Marks the current test as failed.
#define FAIL() hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, false, "failed here")

// void EXPECT_TRUE(bool) - Asserts that the condition is true.
#define EXPECT_TRUE(x_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, (x_), #x_)
// void EXPECT_FALSE(bool) - Asserts that the condition is false.
#define EXPECT_FALSE(x_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, !(x_), "!" #x_)

// void EXPECT_NEAR(T expected, T actual, T absolute_range) - Asserts that two values are within a given range.
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxtest_suite_executor_::singleton_().assert_check_( \
    __FILE__, __LINE__,(((expected_) < (actual_)) ? ((actual_)-(expected_)) : ((expected_)-(actual_))) <= (absolute_range_), \
    "abs(" #expected_ "-" #actual_ ")<=" #absolute_range_)
// void EXPECT_LT(T lhs, T rhs) - Asserts lhs < rhs.
#define EXPECT_LT(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, (lhs_) < (rhs_), #lhs_ "<" #rhs_)
// void EXPECT_GT(T lhs, T rhs) - Asserts lhs > rhs.
#define EXPECT_GT(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, (rhs_) < (lhs_), #lhs_ ">" #rhs_)
// void EXPECT_LE(T lhs, T rhs) - Asserts lhs <= rhs.
#define EXPECT_LE(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, !((rhs_) < (lhs_)), #lhs_ "<=" #rhs_)
// void EXPECT_GE(T lhs, T rhs) - Asserts lhs >= rhs.
#define EXPECT_GE(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, !((lhs_) < (rhs_)), #lhs_ ">=" #rhs_)
// void EXPECT_EQ(T lhs, T rhs) - Asserts lhs == rhs.
#define EXPECT_EQ(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, (lhs_) == (rhs_), #lhs_ "==" #rhs_)
// void EXPECT_NE(T lhs, T rhs) - Asserts lhs != rhs.
#define EXPECT_NE(lhs_, rhs_) hxtest_suite_executor_::singleton_().assert_check_(__FILE__, __LINE__, !((lhs_) == (rhs_)), #lhs_ "!=" #rhs_)

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
