// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxutility.h>
#include <hx/hxtest.hpp>

// This is not a normal dependency of Hatchling Platform.
#include <ctype.h>

HX_REGISTER_FILENAME_HASH

namespace {

template<typename A, typename B> struct hxutility_test_is_same { enum { value = 0 }; };
template<typename A> struct hxutility_test_is_same<A, A> { enum { value = 1 }; };

} // namespace

static_assert(hxfalse_t::value == 0, "hxfalse_t must report false");
static_assert(hxtrue_t::value == 1, "hxtrue_t must report true");

static_assert(hxutility_test_is_same<hxenable_if_t<true, int>, int>::value,
	"hxenable_if_t<true> must expose the requested type");

static_assert(hxutility_test_is_same<hxremove_reference_t<int>, int>::value,
	"hxremove_reference leaves non-references untouched");
static_assert(hxutility_test_is_same<hxremove_reference_t<int&>, int>::value,
	"hxremove_reference_t strips lvalue references");
static_assert(hxutility_test_is_same<hxremove_reference_t<int&&>, int>::value,
	"hxremove_reference_t strips rvalue references");

static_assert(hxutility_test_is_same<hxremove_pointer_t<int*>, int>::value,
	"hxremove_pointer should strip pointers");
static_assert(hxutility_test_is_same<hxremove_pointer_t<int* const>, int>::value,
	"hxremove_pointer should ignore const pointers");
static_assert(hxutility_test_is_same<hxremove_pointer_t<const int*>, const int>::value,
	"hxremove_pointer should leave pointed-to qualifiers");
static_assert(hxutility_test_is_same<hxremove_pointer_t<int* volatile>, int>::value,
	"hxremove_pointer_t should ignore volatile pointers");
static_assert(hxutility_test_is_same<hxremove_pointer_t<int>, int>::value,
	"hxremove_pointer_t should leave non-pointers untouched");

static_assert(hxis_lvalue_reference<int&>::value == 1,
	"hxis_lvalue_reference should detect lvalues");
static_assert(hxis_lvalue_reference<int>::value == 0,
	"hxis_lvalue_reference should reject non-references");
static_assert(hxis_rvalue_reference<int&&>::value == 1,
	"hxis_rvalue_reference should detect rvalues");
static_assert(hxis_rvalue_reference<int&>::value == 0,
	"hxis_rvalue_reference should reject lvalues");

static_assert(hxutility_test_is_same<hxremove_cv_t<int>, int>::value,
	"hxremove_cv_t leaves plain types untouched");
static_assert(hxutility_test_is_same<hxremove_cv_t<const volatile int>, int>::value,
	"hxremove_cv strips const volatile");
static_assert(hxutility_test_is_same<hxremove_cv_t<const int>, int>::value,
	"hxremove_cv_t strips const");
static_assert(hxutility_test_is_same<hxremove_cv_t<volatile int>, int>::value,
	"hxremove_cv_t strips volatile");

static_assert(hxis_const<int>::value == 0, "hxis_const should reject mutable");
static_assert(hxis_const<const int>::value == 1,
	"hxis_const should detect const");

static_assert(hxis_void<void>::value == 1, "hxis_void should detect void");
static_assert(hxis_void<const void>::value == 1,
	"hxis_void should ignore qualifiers");
static_assert(hxis_void<int>::value == 0, "hxis_void should reject others");

static_assert(hxis_null_pointer<decltype(nullptr)>::value == 1,
	"hxis_null_pointer should detect nullptr_t");
static_assert(hxis_null_pointer<const decltype(nullptr)>::value == 1,
	"hxis_null_pointer should ignore qualifiers");
static_assert(hxis_null_pointer<int*>::value == 0,
	"hxis_null_pointer should reject pointers");

static_assert(hxis_integral<int>::value == 1,
	"hxis_integral should detect int");
static_assert(hxis_integral<const unsigned long>::value == 1,
	"hxis_integral should ignore qualifiers");
static_assert(hxis_integral<float>::value == 0,
	"hxis_integral should reject floats");

static_assert(hxis_floating_point<float>::value == 1,
	"hxis_floating_point should detect floats");
static_assert(hxis_floating_point<const long double>::value == 1,
	"hxis_floating_point should ignore qualifiers");
static_assert(hxis_floating_point<int>::value == 0,
	"hxis_floating_point should reject ints");

static_assert(hxis_array<int>::value == 0,
	"hxis_array should reject non-arrays");
static_assert(hxis_array<int[4]>::value == 1,
	"hxis_array should detect sized arrays");
static_assert(hxis_array<const int[]>::value == 1,
	"hxis_array should detect unsized arrays");

static_assert(hxis_pointer<int*>::value == 1,
	"hxis_pointer should detect pointers");
static_assert(hxis_pointer<const int*>::value == 1,
	"hxis_pointer should ignore qualifiers");
static_assert(hxis_pointer<int>::value == 0,
	"hxis_pointer should reject non-pointers");

static_assert(hxutility_test_is_same<hxrestrict_t<int>, int>::value,
	"hxrestrict_t should leave non-pointers untouched");
static_assert(sizeof(hxrestrict_t<int*>) == sizeof(int*),
	"hxrestrict_t should preserve pointer representation");

namespace {

enum hxutility_test_forward {
	hxutility_test_forward_none,
	hxutility_test_forward_lvalue,
	hxutility_test_forward_const_lvalue,
	hxutility_test_forward_rvalue,
	hxutility_test_forward_const_rvalue
};

class hxutility_test_forward_t {
public:
	int value;
};

hxutility_test_forward_t hxutility_test_forward_make_forwarded() { return { 11 }; }
const hxutility_test_forward_t hxutility_test_forward_make_const_forwarded() { return { 13 }; }
hxutility_test_forward hxutility_test_forward_detect(hxutility_test_forward_t&) { return hxutility_test_forward_lvalue; }
hxutility_test_forward hxutility_test_forward_detect(const hxutility_test_forward_t&) { return hxutility_test_forward_const_lvalue; }
hxutility_test_forward hxutility_test_forward_detect(hxutility_test_forward_t&&) { return hxutility_test_forward_rvalue; }
hxutility_test_forward hxutility_test_forward_detect(const hxutility_test_forward_t&&) { return hxutility_test_forward_const_rvalue; }

template<typename T>
hxutility_test_forward hxutility_test_forward_through_template(T&& value) {
	return hxutility_test_forward_detect(hxforward<T>(value));
}

} // namespace

TEST(hxutility_test, hxabs_double) {
	const double negative = -42.75;
	const double positive = 42.75;
	EXPECT_DOUBLE_EQ(hxabs(negative), positive);
	EXPECT_DOUBLE_EQ(hxabs(positive), positive);
	EXPECT_DOUBLE_EQ(hxabs(-0.0), -0.0);
}

TEST(hxutility_test, hxforward) {
	EXPECT_EQ(hxutility_test_forward_rvalue,
		hxutility_test_forward_detect(hxforward<hxutility_test_forward_t>(hxutility_test_forward_make_forwarded())));

	EXPECT_EQ(hxutility_test_forward_const_rvalue,
		hxutility_test_forward_detect(hxforward<const hxutility_test_forward_t>(hxutility_test_forward_make_const_forwarded())));

	hxutility_test_forward_t lvalue = { 7 };
	EXPECT_EQ(hxutility_test_forward_lvalue, hxutility_test_forward_through_template(lvalue));

	const hxutility_test_forward_t const_lvalue = { 9 };
	EXPECT_EQ(hxutility_test_forward_const_lvalue,
		hxutility_test_forward_through_template(const_lvalue));

	EXPECT_EQ(hxutility_test_forward_rvalue,
		hxutility_test_forward_through_template(hxutility_test_forward_make_forwarded()));

	hxutility_test_forward_t movable_value = { 17 };
	EXPECT_EQ(hxutility_test_forward_rvalue,
		hxutility_test_forward_through_template(hxmove(movable_value)));

	const hxutility_test_forward_t const_movable_value = { 19 };
	EXPECT_EQ(hxutility_test_forward_const_rvalue,
		hxutility_test_forward_through_template(hxmove(const_movable_value)));
}

TEST(hxutility_test, hxnullptr_converts_only_to_null) {
	hxnullptr_t null_object;
	const int* int_ptr = null_object;
	EXPECT_EQ(int_ptr, hxnullptr);

	struct hxutility_member_holder { int value; };
	int hxutility_member_holder::* member_ptr = null_object;
	EXPECT_EQ(member_ptr, hxnullptr);
}

TEST(hxutility_test, hxbasename_handles_separators) {
	EXPECT_STREQ(hxbasename("plain"), "plain");
	EXPECT_STREQ(hxbasename("dir/file.bin"), "file.bin");
	EXPECT_STREQ(hxbasename("dir\\file.bin"), "file.bin");
	EXPECT_STREQ(hxbasename("dir/sub\\mixed"), "mixed");
	EXPECT_STREQ(hxbasename("dir/"), "");
}

TEST(hxutility_test, hxlog2i_returns_highest_set_bit) {
	EXPECT_EQ(hxlog2i(1u), 0);
	EXPECT_EQ(hxlog2i(2u), 1);
	EXPECT_EQ(hxlog2i(3u), 1);
	EXPECT_EQ(hxlog2i(16u), 4);
	EXPECT_EQ(hxlog2i((size_t)1u << 20), 20);
}

TEST(hxutility_test, hxisfinite_detects_special_values) {
	const uint32_t float_pos_inf_bits = 0x7f800000u;
	const uint32_t float_neg_inf_bits = 0xff800000u;
	const uint32_t float_nan_bits = 0x7fc00000u;
	const uint64_t double_pos_inf_bits = 0x7ff0000000000000ull;
	const uint64_t double_neg_inf_bits = 0xfff0000000000000ull;
	const uint64_t double_nan_bits = 0x7ff8000000000000ull;

	float float_pos_inf, float_neg_inf, float_nan;
	double double_pos_inf, double_neg_inf, double_nan;

	memcpy(&float_pos_inf, &float_pos_inf_bits, sizeof float_pos_inf);
	memcpy(&float_neg_inf, &float_neg_inf_bits, sizeof float_neg_inf);
	memcpy(&float_nan, &float_nan_bits, sizeof float_nan);
	memcpy(&double_pos_inf, &double_pos_inf_bits, sizeof double_pos_inf);
	memcpy(&double_neg_inf, &double_neg_inf_bits, sizeof double_neg_inf);
	memcpy(&double_nan, &double_nan_bits, sizeof double_nan);

	EXPECT_TRUE(hxisfinitef(-0.0f));
	EXPECT_TRUE(hxisfinitef(1.0f));
	EXPECT_FALSE(hxisfinitef(float_pos_inf));
	EXPECT_FALSE(hxisfinitef(float_neg_inf));
	EXPECT_FALSE(hxisfinitef(float_nan));

	EXPECT_TRUE(hxisfinitel(-0.0));
	EXPECT_TRUE(hxisfinitel(1.0));
	EXPECT_FALSE(hxisfinitel(double_pos_inf));
	EXPECT_FALSE(hxisfinitel(double_neg_inf));
	EXPECT_FALSE(hxisfinitel(double_nan));
}

TEST(hxutility_test, arithmetic_helpers_cover_min_max_abs_clamp) {
	EXPECT_EQ(hxmin(3, 7), 3);
	EXPECT_EQ(hxmax(3, 7), 7);
	EXPECT_EQ(hxabs(-9), 9);
	EXPECT_EQ(hxabs(9), 9);
	EXPECT_EQ(hxclamp(5, 0, 10), 5);
	EXPECT_EQ(hxclamp(-1, 0, 10), 0);
	EXPECT_EQ(hxclamp(11, 0, 10), 10);
}

TEST(hxutility_test, hxswap_memcpy) {
	struct hxutility_memcpy_record {
		int32_t first;
		int32_t second;
	} first = { 1, 2 }, second = { 3, 4 };

	hxswap_memcpy(first, second);

	EXPECT_EQ(first.first, 3);
	EXPECT_EQ(first.second, 4);
	EXPECT_EQ(second.first, 1);
	EXPECT_EQ(second.second, 2);
}

TEST(hxutility_test, hxisspace) {
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

TEST(hxutility_test, hxisgraph) {
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
