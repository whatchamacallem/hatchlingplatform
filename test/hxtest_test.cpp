// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxtest.hpp>
#include <hx/hxarray.hpp>

HX_REGISTER_FILENAME_HASH


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
