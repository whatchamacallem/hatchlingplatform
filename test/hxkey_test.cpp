// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxkey.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

static_assert(hxis_same<
	decltype(hxkey_equal_function<const volatile char*>()),
	bool (*)(const volatile char* const&, const volatile char* const&)>::value,
	"hxkey_equal_function must preserve const volatile pointer types");

static_assert(hxis_same<
	decltype(hxkey_equal_function<volatile char&&>()),
	bool (*)(const char&, const char&)>::value,
	"hxkey_equal_function must handle non-const volatile references.");

static_assert(hxis_same<
	decltype(hxkey_less_function<const volatile char*>()),
	bool (*)(const volatile char* const&, const volatile char* const&)>::value,
	"hxkey_less_function must preserve const volatile pointer types");

static_assert(hxis_same<
	decltype(hxkey_less_function<volatile char&&>()),
	bool (*)(const char&, const char&)>::value,
	"hxkey_less_function must handle non-const volatile references.");

TEST(hxkey_function, char_pointer_dispatch) {
	const auto equal_fn = hxkey_equal_function<char*>();
	const auto less_fn = hxkey_less_function<char*>();

	char alpha[] = "alpha";
	char beta[] = "beta";
	char alpha_duplicate[] = "alpha";

	EXPECT_TRUE(equal_fn(alpha, alpha_duplicate));
	EXPECT_FALSE(equal_fn(alpha, beta));

	EXPECT_TRUE(less_fn(alpha, beta));
	EXPECT_FALSE(less_fn(beta, alpha));
}
