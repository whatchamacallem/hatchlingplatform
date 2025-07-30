// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the terms of the LICENSE.md file.

// confirm hxtest.h includes hatchling.h correctly.
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

TEST(hxstring_hash_test, equality) {
	hxhash_t hash1 = hxstring_literal_hash("");
	hxhash_t hash2 = hxstring_literal_hash("abc");
	hxhash_t hash3 = hxstring_literal_hash(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456");

	EXPECT_EQ(hash1, hxstring_literal_hash_debug(""));
	EXPECT_EQ(hash2, hxstring_literal_hash_debug("abc"));
	EXPECT_EQ(hash3, hxstring_literal_hash_debug(
		"The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456"));
}
