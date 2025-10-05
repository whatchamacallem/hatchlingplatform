// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxtest.hpp>
#include <hx/hxarray.hpp>

#include "hxctest.h"

#include <ctype.h>

HX_REGISTER_FILENAME_HASH

namespace {

template<typename T>
void hxtest_relational_(T a, T b) {
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

TEST(hxtest_macros, relational) {
	hxtest_relational_<int>(-1, 0);
	hxtest_relational_<long>(-5, -4);
	hxtest_relational_<long long>(-9, -8);
	hxtest_relational_<unsigned int>(1u, 2u);
	hxtest_relational_<unsigned long>(5ul, 6ul);
	hxtest_relational_<unsigned long long>(11ull, 12ull);
	hxtest_relational_<int8_t>(static_cast<int8_t>(-12), static_cast<int8_t>(-11));
	hxtest_relational_<uint8_t>(static_cast<uint8_t>(3), static_cast<uint8_t>(4));
	hxtest_relational_<int16_t>(static_cast<int16_t>(-301), static_cast<int16_t>(-300));
	hxtest_relational_<uint16_t>(static_cast<uint16_t>(7), static_cast<uint16_t>(8));
	hxtest_relational_<int32_t>(static_cast<int32_t>(-70001), static_cast<int32_t>(-70000));
	hxtest_relational_<uint32_t>(static_cast<uint32_t>(100), static_cast<uint32_t>(101));
	hxtest_relational_<float>(-0.00002f, -0.00001f);
	hxtest_relational_<double>(0.0, 1.0);

	ASSERT_STREQ("a", "a");
	ASSERT_STRNE("a", "b");
}

// Run all the C tests.
TEST(hxctest, all_tests) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	EXPECT_TRUE(hxctest_all());
}

// Make sure new and delete plausibly exist. Make sure hxnullptr compiles.
TEST(hxnew, smoketest) {
	unsigned int* t = new unsigned int(3);
	ASSERT_TRUE(t);
	hxassertrelease(t, "new"); // Should be impossible.
	*t = 0xdeadbeefu;
	delete t;
	t = hxnullptr;
	ASSERT_FALSE(t);
}

// These two tests test the test framework by failing.
#if HX_TEST_ERROR_HANDLING
TEST(death_test, fail) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	for(int i = 10; i--;) {
		FAIL() << "this message is intentionally blank.\n";
	}
	SUCCEED();
}
TEST(death_test, nothing_asserted) {
	hxlog("EXPECTING_TEST_FAILURE\n");
}
#endif

TEST(hxisspace, compare_with_standard) {
	// Don't use non-ASCII or setlocale because it might not exist.

	for (int c = 0; c < 128; ++c) {
		const bool hx = hxisspace((char)c);
		const bool st = ::isspace((unsigned char)c) != 0;
		EXPECT_EQ(hx, st);
	}
	for (int c = 128; c < 256; ++c) {
		const bool hx = hxisspace((char)c);
		EXPECT_EQ(hx, false);
	}
}

TEST(hxisgraph, compare_with_standard) {
	// Don't use non-ASCII or setlocale because it might not exist.

	for (int c = 0; c <= 255; ++c) {
		const bool hx = hxisgraph((char)c);

		const bool expected = (c >= 0x21 && c <= 0x7E) || (c >= 0x80);
		EXPECT_EQ(hx, expected);

		if (c < 0x80) {
			const bool st = ::isgraph(c) != 0;
			EXPECT_EQ(hx, st);
		}
	}
}
