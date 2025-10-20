// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxtest.hpp>
#include <hx/hxarray.hpp>

#include "hxctest.h"

HX_REGISTER_FILENAME_HASH

namespace {

template<typename T>
void hxtest_relational(T a, T b) {
	ASSERT_EQ(a, a);
	ASSERT_GE(a, a);
	ASSERT_GE(b, a);
	ASSERT_GT(b, a);
	ASSERT_LE(a, a);
	ASSERT_LE(a, b);
	ASSERT_LT(a, b);
	ASSERT_NE(a, b);
	EXPECT_EQ(a, a);
	EXPECT_GE(a, a);
	EXPECT_GE(b, a);
	EXPECT_GT(b, a);
	EXPECT_LE(a, a);
	EXPECT_LE(a, b);
	EXPECT_LT(a, b);
	EXPECT_NE(a, b);
}

} // namespace {

TEST(hxtest_test, relational) {
	hxtest_relational<int>(-1, 0);
	hxtest_relational<long>(-5, -4);
	hxtest_relational<long long>(-9, -8);
	hxtest_relational<unsigned int>(1u, 2u);
	hxtest_relational<unsigned long>(5ul, 6ul);
	hxtest_relational<unsigned long long>(11ull, 12ull);
	hxtest_relational<int8_t>(static_cast<int8_t>(-12), static_cast<int8_t>(-11));
	hxtest_relational<uint8_t>(static_cast<uint8_t>(3), static_cast<uint8_t>(4));
	hxtest_relational<int16_t>(static_cast<int16_t>(-301), static_cast<int16_t>(-300));
	hxtest_relational<uint16_t>(static_cast<uint16_t>(7), static_cast<uint16_t>(8));
	hxtest_relational<int32_t>(static_cast<int32_t>(-70001), static_cast<int32_t>(-70000));
	hxtest_relational<uint32_t>(static_cast<uint32_t>(100), static_cast<uint32_t>(101));
	hxtest_relational<float>(-0.00002f, -0.00001f);
	hxtest_relational<double>(0.0, 1.0);

	// "Requires that two C strings are equal, without checking null pointers."
	ASSERT_STREQ("a", "a");
	ASSERT_STRNE("a", "b");
}

// These avoid knowing anything about the implementation.
TEST(hxtest_test, float_eq) {
	// "Checks floats for equality within a scaled tolerance." Cover a handful
	// of representative cases.
	const float third = 1.0f / 3.0f;
	ASSERT_FLOAT_EQ(third + third + third, 1.0f);

	const float a = 0.1f;
	const float b = 0.2f;
	const float c = 0.3f;
	ASSERT_FLOAT_EQ(a + b, c);
	ASSERT_FLOAT_EQ(c - b, a);
	ASSERT_FLOAT_EQ((a + b) - a, b);

	const float tenth = 1.0f / 10.0f;
	ASSERT_FLOAT_EQ(tenth * 10.0f, 1.0f);
	ASSERT_FLOAT_EQ(a * a, 0.01f);
}

TEST(hxtest_test, double_eq) {
	// "Checks doubles for equality within a scaled tolerance." Mirror float
	// coverage using double path.
	const double third = 1.0 / 3.0;
	ASSERT_DOUBLE_EQ(third + third + third, 1.0);

	const double a = 0.1;
	const double b = 0.2;
	const double c = 0.3;
	ASSERT_DOUBLE_EQ(a + b, c);
	ASSERT_DOUBLE_EQ(c - b, a);
	ASSERT_DOUBLE_EQ((a + b) - a, b);

	const double tenth = 1.0 / 10.0;
	ASSERT_DOUBLE_EQ(tenth * 10.0, 1.0);
	ASSERT_DOUBLE_EQ(a * a, 0.01);
}

// Run all the C tests.
TEST(hxtest_test, all_tests) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	EXPECT_TRUE(hxctest_all());
}

// Another "nothing asserted" test case.
TEST(hxtest_test, succeed) {
	SUCCEED();
}

// These two tests exercise the test framework by failing.
#if HX_TEST_ERROR_HANDLING
TEST(hxtest_error_handling, fail) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	FAIL();
	hxassertrelease(0, "internal_error FAIL() did not return");
}

TEST(hxtest_error_handling, add_failure) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	for(int i = 10; i--;) {
		ADD_FAILURE() << "This message is intentionally blank.\n";
	}
}

TEST(hxtest_error_handling, add_failure_at) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	ADD_FAILURE_AT("fake_file.cpp", 10000) << "This message is also intentionally blank.\n";
}

TEST(hxtest_error_handling, nothing_asserted) {
	hxlog("EXPECTING_TEST_FAILURE\n");
}
#endif
