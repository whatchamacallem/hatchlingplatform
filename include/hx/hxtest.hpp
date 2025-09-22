#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxtest.hpp This is a Google Test-compatable framework for writing unit
/// tests. It doesn't spam your system memory allocator with string operations
/// right after an assert fails. Actually, it makes no allocations ever. To
/// disable this header and switch to testing with `<gtest/gtest.h>` directly use
/// `-DHX_USE_GOOGLE_TEST=1`. Only core features are provided. This framework only
/// uses the `operator<` and `operator==` in it's asserts. Compatibility with
/// Google Test may require additional relational operators.
///
/// - `TEST(suite, name)` - Defines a test case without a fixture.
/// - `TEST_F(fixture, name)` - Defines a test case using a fixture class.
/// - Use `ASSERT_`* for fatal assertions and `EXPECT_`* for non-fatal.
/// - See `RUN_ALL_TESTS()` in `test/hxtest_main.cpp` for example.
///
/// - Simple Test Case (no fixture):
/// ```
///   TEST(Math, Addition) {
///	   int a = 2, b = 3;
///	   EXPECT_EQ(a + b, 5);
///	   EXPECT_TRUE(a < b + 2);
///	   EXPECT_NEAR(3.14, 3.141, 0.01);
///	   SUCCEED();
///   }
/// ```
/// - Fixture-Based Test Case (using `TEST_F`):
/// ```
///   class MyFixture : public testing::Test {
///   public:
///	   void SetUp() override { value = 42; }
///	   void TearDown() override { EXPECT_EQ(value, 0) }
///	   void set_value(int x) { value = x; }
///	   int value;
///   };
///
///   TEST_F(MyFixture, ValueIsSet) {
///	   EXPECT_EQ(value, 42);
///	   set_value(100);
///	   EXPECT_NE(value, 42);
///   }
/// ```
/// - Condition Check Macros: (`a` and `b` only require `operator<` and `operator==`.)
/// ```
///   EXPECT_TRUE(expr);	  // Checks expr is true
///   EXPECT_FALSE(expr);	  // Checks expr is false
///   EXPECT_EQ(a, b);		  // Checks a == b
///   EXPECT_NE(a, b);		  // Checks a != b
///   EXPECT_LT(a, b);		  // Checks a < b
///   EXPECT_GT(a, b);		  // Checks a > b
///   EXPECT_LE(a, b);		  // Checks a <= b
///   EXPECT_GE(a, b);		  // Checks a >= b
///   EXPECT_NEAR(a, b, tol); // Checks |a-b| <= tol
///   SUCCEED();			  // Marks test as successful
///   FAIL();				  // Marks test as failed
/// ```
/// - `ASSERT_`* macros are equivalent to `EXPECT_`*

// Used to resolve `HX_USE_GOOGLE_TEST`.
#include "hatchling.h"

// HX_USE_GOOGLE_TEST - Enable this to use Google Test instead of hxtest.
#if HX_USE_GOOGLE_TEST
#include <gtest/gtest.h>
#else // !HX_USE_GOOGLE_TEST
#include "detail/hxtest_detail.hpp"

/// `testing` - A partial Google Test reimplementation. Use `-DHX_TEST_MAX_CASES`
/// to provide enough room for all tests.
namespace testing {

/// `Test` - Base class for tests required by Google Test's `TEST_F`.
class Test {
public:
	/// User override for tests using `TEST_F`.
	virtual void SetUp(void) { }
	/// User override for tests using `TEST_F`.
	virtual void TearDown(void) { }
	/// User override for tests using `TEST_F`.
	virtual ~Test(void) { };

	// Not for direct use. This is the Google Test invocation protocol.
	/// \cond HIDDEN
	void run_(void) {
		SetUp();
		run_code_();
		TearDown();
	}
	/// \endcond

private:
	// Provided and used by the `TEST_F` macro.
	virtual void run_code_() = 0;
};

/// `InitGoogleTest` - Initializes Google Test with command-line arguments. No-op in
/// this implementation.
inline void InitGoogleTest(int *, char **) { }

/// `InitGoogleTest` - Overloaded version of `InitGoogleTest` with no arguments. No-op
/// in this implementation.
inline void InitGoogleTest(void) { }

} // namespace testing

/// `HX_TEST_NAME_` - Macro for concatenating 3 arguments into one name.
/// Macro parameters will be evaluated before concatenating.
#define HX_TEST_NAME_(x_, y_, z_) x_ ## y_ ## _ ## z_ ## _

/// `TEST(suite_name, case_name)` - Google Test reimplementation. Defines a test
/// case with a suite name and case name.
/// - `suite_name` : A valid C identifier for the test suite.
/// - `case_name` : A valid C identifier for the test case.
#define TEST(suite_name_, case_name_) \
	class HX_TEST_NAME_(hxtest_, suite_name_, case_name_) : public hxtest_case_interface_ { \
	public: \
		HX_TEST_NAME_(hxtest_, suite_name_, case_name_)(void) { hxtest_::dispatcher_().add_test_(this); } \
		virtual void run_(void) override; \
		virtual const char* suite_(void) const override { return #suite_name_; } \
		virtual const char* case_(void) const override { return #case_name_; } \
		virtual const char* file_(void) const override { return __FILE__; } \
		virtual size_t line_(void) const override { return __LINE__; } \
	} static HX_TEST_NAME_(s_hxtest_, suite_name_, case_name_); \
	void HX_TEST_NAME_(hxtest_, suite_name_, case_name_)::run_(void)

/// `TEST_F(suite_name, case_name)` - Google Test reimplementation for
/// fixture-based tests. Defines a test case where the `suite_name` is a
/// subclass of `testing::Test`.
/// - `suite_name` : A valid C identifier for the test suite.
/// - `case_name` : A valid C identifier for the test case.
#define TEST_F(suite_name_, case_name_) \
	class HX_TEST_NAME_(hxtest_, suite_name_, case_name_) : public hxtest_case_interface_ { \
	public: \
		class hxtest_case_dispatcher_ : public suite_name_ { virtual void run_code_(void) override; }; \
		HX_TEST_NAME_(hxtest_, suite_name_, case_name_)(void) { hxtest_::dispatcher_().add_test_(this); } \
		virtual void run_(void) override { hxtest_case_dispatcher_ dispatcher_; dispatcher_.run_(); } \
		virtual const char* suite_(void) const override { return #suite_name_; } \
		virtual const char* case_(void) const override { return #case_name_; } \
		virtual const char* file_(void) const override { return __FILE__; } \
		virtual size_t line_(void) const override { return __LINE__; } \
	} static HX_TEST_NAME_(s_hxtest, suite_name_, case_name_); \
	void HX_TEST_NAME_(hxtest_, suite_name_, case_name_)::hxtest_case_dispatcher_::run_code_(void)

/// `int RUN_ALL_TESTS(...)` - Executes all registered test cases.
/// `...` : An optional const char* matching a specific test suite to run. (Non-standard.)
#define RUN_ALL_TESTS(...) hxtest_::dispatcher_().run_all_tests_(__VA_ARGS__)

/// `void SUCCEED()` - Marks the current test as successful without any checks.
#define SUCCEED() hxtest_::dispatcher_().condition_check_(true, __FILE__, __LINE__, hxnull, false)

/// `void FAIL()` - Marks the current test as failed.
#define FAIL() hxtest_::dispatcher_().condition_check_(false, __FILE__, __LINE__, "FAIL()", false)

/// `void EXPECT_TRUE(bool)` - Checks that the condition is true.
#define EXPECT_TRUE(x_) hxtest_::dispatcher_().condition_check_((x_), __FILE__, __LINE__, #x_, false)
/// `void EXPECT_FALSE(bool)` - Checks that the condition is false.
#define EXPECT_FALSE(x_) hxtest_::dispatcher_().condition_check_(!(x_), __FILE__, __LINE__, "!" #x_, false)

/// `void EXPECT_NEAR(T expected, T actual, T absolute_range)` - Checks that two values are within a given range.
#define EXPECT_NEAR(expected_, actual_, absolute_range_) hxtest_::dispatcher_().condition_check_( \
	(((expected_) < (actual_)) ? ((actual_)-(expected_)) : ((expected_)-(actual_))) <= (absolute_range_), \
	__FILE__, __LINE__, "abs(" #expected_ "-" #actual_ ") <= " #absolute_range_, false)
/// `void EXPECT_LT(T a, T b)` - Checks `a < b`.
#define EXPECT_LT(a_, b_) hxtest_::dispatcher_().condition_check_((a_) < (b_), __FILE__, __LINE__, #a_ " < " #b_, false)
/// `void EXPECT_GT(T a, T b)` - Checks `a > b` using `b < a`.
#define EXPECT_GT(a_, b_) hxtest_::dispatcher_().condition_check_((b_) < (a_), __FILE__, __LINE__, #a_ " > " #b_, false)
/// `void EXPECT_LE(T a, T b)` - Checks `a <= b` using `!(b < a)`.
#define EXPECT_LE(a_, b_) hxtest_::dispatcher_().condition_check_(!((b_) < (a_)), __FILE__, __LINE__, #a_ " <= " #b_, false)
/// `void EXPECT_GE(T a, T b)` - Checks `a >= b` using `!(a < b)`.
#define EXPECT_GE(a_, b_) hxtest_::dispatcher_().condition_check_(!((a_) < (b_)), __FILE__, __LINE__, #a_ " >= " #b_, false)
/// `void EXPECT_EQ(T a, T b)` - Checks `a == b`.
#define EXPECT_EQ(a_, b_) hxtest_::dispatcher_().condition_check_((a_) == (b_), __FILE__, __LINE__, #a_ " == " #b_, false)
/// `void EXPECT_NE(T a, T b)` - Checks `a != b` using `!(a == b)`.
#define EXPECT_NE(a_, b_) hxtest_::dispatcher_().condition_check_(!((a_) == (b_)), __FILE__, __LINE__, #a_ " != " #b_, false)

/// `void ASSERT_TRUE(bool)` - Asserts that the condition is true.
#define ASSERT_TRUE(x_) hxtest_::dispatcher_().condition_check_((x_), __FILE__, __LINE__, #x_, true)
/// `void ASSERT_FALSE(bool)` - Asserts that the condition is false.
#define ASSERT_FALSE(x_) hxtest_::dispatcher_().condition_check_(!(x_), __FILE__, __LINE__, "!" #x_, true)

/// `void ASSERT_NEAR(T expected, T actual, T absolute_range)` - Asserts that two values are within a given range.
#define ASSERT_NEAR(expected_, actual_, absolute_range_) hxtest_::dispatcher_().condition_check_( \
	(((expected_) < (actual_)) ? ((actual_)-(expected_)) : ((expected_)-(actual_))) <= (absolute_range_), \
	__FILE__, __LINE__, "abs(" #expected_ "-" #actual_ ") <= " #absolute_range_, true)
/// `void ASSERT_LT(T a, T b)` - Asserts `a < b`.
#define ASSERT_LT(a_, b_) hxtest_::dispatcher_().condition_check_((a_) < (b_), __FILE__, __LINE__, #a_ " < " #b_, true)
/// `void ASSERT_GT(T a, T b)` - Asserts `a > b` using `b < a`.
#define ASSERT_GT(a_, b_) hxtest_::dispatcher_().condition_check_((b_) < (a_), __FILE__, __LINE__, #a_ " > " #b_, true)
/// `void ASSERT_LE(T a, T b)` - Asserts `a <= b` using `!(b < a)`.
#define ASSERT_LE(a_, b_) hxtest_::dispatcher_().condition_check_(!((b_) < (a_)), __FILE__, __LINE__, #a_ " <= " #b_, true)
/// `void ASSERT_GE(T a, T b)` - Asserts `a >= b` using `!(a < b)`.
#define ASSERT_GE(a_, b_) hxtest_::dispatcher_().condition_check_(!((a_) < (b_)), __FILE__, __LINE__, #a_ " >= " #b_, true)
/// `void ASSERT_EQ(T a, T b)` - Asserts `a == b`.
#define ASSERT_EQ(a_, b_) hxtest_::dispatcher_().condition_check_((a_) == (b_), __FILE__, __LINE__, #a_ " == " #b_, true)
/// `void ASSERT_NE(T a, T b)` - Asserts `a != b` using `!(a == b)`.
#define ASSERT_NE(a_, b_) hxtest_::dispatcher_().condition_check_(!((a_) == (b_)), __FILE__, __LINE__, #a_ " != " #b_, true)

#endif // !HX_USE_GOOGLE_TEST
