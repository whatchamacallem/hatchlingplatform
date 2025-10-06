// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxutility.h>
#include <hx/hxtest.hpp>

#include <stdint.h>

HX_REGISTER_FILENAME_HASH

namespace {

template<typename A_, typename B_>
struct hxutility_is_same { enum { value = 0 }; };

template<typename A_>
struct hxutility_is_same<A_, A_> { enum { value = 1 }; };

template<typename T_>
static hxenable_if_t<hxis_integral<T_>::value, int>
hxutility_enable_if_guard(T_) {
	return 1;
}

class hxutility_swap_move_tracker {
public:
	explicit hxutility_swap_move_tracker(int value_)
		: value(value_), moved_from(false) { }

	hxutility_swap_move_tracker(const hxutility_swap_move_tracker&) = delete;

	hxutility_swap_move_tracker(hxutility_swap_move_tracker&& other_)
		: value(other_.value), moved_from(false) {
		other_.moved_from = true;
	}

	hxutility_swap_move_tracker&
	operator=(hxutility_swap_move_tracker&& other_) {
		value = other_.value;
		moved_from = false;
		other_.moved_from = true;
		return *this;
	}

	hxutility_swap_move_tracker& operator=(
		const hxutility_swap_move_tracker&) = delete;

	int value;
	bool moved_from;
};

struct hxutility_memcpy_record {
	int32_t first;
	int32_t second;
};
} // namespace

static_assert(hxfalse_t::value == 0, "hxfalse_t must report false");
static_assert(hxtrue_t::value == 1, "hxtrue_t must report true");

static_assert(hxutility_is_same<hxenable_if<true, int>::type, int>::value,
	"hxenable_if<true> must expose the requested type");
static_assert(hxutility_is_same<hxenable_if_t<true, int>, int>::value,
	"hxenable_if_t<true> must expose the requested type");

static_assert(hxutility_is_same<hxremove_reference_<int>::type, int>::value,
	"hxremove_reference_ leaves non-references untouched");
static_assert(hxutility_is_same<hxremove_reference_<int&>::type, int>::value,
	"hxremove_reference_ strips lvalue references");
static_assert(hxutility_is_same<hxremove_reference_<int&&>::type, int>::value,
	"hxremove_reference_ strips rvalue references");
static_assert(hxutility_is_same<hxremove_reference_t<int&>, int>::value,
	"hxremove_reference_t strips lvalue references");
static_assert(hxutility_is_same<hxremove_reference_t<int&&>, int>::value,
	"hxremove_reference_t strips rvalue references");

static_assert(hxis_lvalue_reference<int&>::value == 1,
	"hxis_lvalue_reference should detect lvalues");
static_assert(hxis_lvalue_reference<int>::value == 0,
	"hxis_lvalue_reference should reject non-references");
static_assert(hxis_rvalue_reference<int&&>::value == 1,
	"hxis_rvalue_reference should detect rvalues");
static_assert(hxis_rvalue_reference<int&>::value == 0,
	"hxis_rvalue_reference should reject lvalues");

static_assert(hxutility_is_same<hxremove_cv_<int>::type, int>::value,
	"hxremove_cv_ leaves plain types untouched");
static_assert(hxutility_is_same<hxremove_cv_<const int>::type, int>::value,
	"hxremove_cv_ strips const");
static_assert(hxutility_is_same<hxremove_cv_<volatile int>::type, int>::value,
	"hxremove_cv_ strips volatile");
static_assert(
	hxutility_is_same<hxremove_cv_<const volatile int>::type, int>::value,
	"hxremove_cv_ strips const volatile");
static_assert(hxutility_is_same<hxremove_cv_t<const int>, int>::value,
	"hxremove_cv_t strips const");
static_assert(hxutility_is_same<hxremove_cv_t<volatile int>, int>::value,
	"hxremove_cv_t strips volatile");

static_assert(hxis_const<int>::value == 0, "hxis_const should reject mutable");
static_assert(hxis_const<const int>::value == 1,
	"hxis_const should detect const");

static_assert(hxis_void_<void>::value == 1,
	"hxis_void_ should detect the void type");
static_assert(hxis_void_<int>::value == 0,
	"hxis_void_ should reject other types");
static_assert(hxis_void<void>::value == 1, "hxis_void should detect void");
static_assert(hxis_void<const void>::value == 1,
	"hxis_void should ignore qualifiers");
static_assert(hxis_void<int>::value == 0, "hxis_void should reject others");

static_assert(hxis_null_pointer_<decltype(nullptr)>::value == 1,
	"hxis_null_pointer_ should detect nullptr_t");
static_assert(hxis_null_pointer_<int*>::value == 0,
	"hxis_null_pointer_ should reject pointers");
static_assert(hxis_null_pointer<decltype(nullptr)>::value == 1,
	"hxis_null_pointer should detect nullptr_t");
static_assert(hxis_null_pointer<const decltype(nullptr)>::value == 1,
	"hxis_null_pointer should ignore qualifiers");
static_assert(hxis_null_pointer<int*>::value == 0,
	"hxis_null_pointer should reject pointers");

static_assert(hxis_integral_<int>::value == 1,
	"hxis_integral_ should detect int");
static_assert(hxis_integral_<bool>::value == 1,
	"hxis_integral_ should detect bool");
static_assert(hxis_integral_<float>::value == 0,
	"hxis_integral_ should reject floats");
static_assert(hxis_integral<int>::value == 1,
	"hxis_integral should detect int");
static_assert(hxis_integral<const unsigned long>::value == 1,
	"hxis_integral should ignore qualifiers");
static_assert(hxis_integral<float>::value == 0,
	"hxis_integral should reject floats");

static_assert(hxis_floating_point_<float>::value == 1,
	"hxis_floating_point_ should detect float");
static_assert(hxis_floating_point_<double>::value == 1,
	"hxis_floating_point_ should detect double");
static_assert(hxis_floating_point_<int>::value == 0,
	"hxis_floating_point_ should reject ints");
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

static_assert(hxis_pointer_<int*>::value == 1,
	"hxis_pointer_ should detect pointers");
static_assert(hxis_pointer_<int>::value == 0,
	"hxis_pointer_ should reject non-pointers");
static_assert(hxis_pointer<int*>::value == 1,
	"hxis_pointer should detect pointers");
static_assert(hxis_pointer<const int*>::value == 1,
	"hxis_pointer should ignore qualifiers");
static_assert(hxis_pointer<int>::value == 0,
	"hxis_pointer should reject non-pointers");

static_assert(hxutility_is_same<hxrestrict_t_<int>::type, int>::value,
	"hxrestrict_t_ should leave non-pointers untouched");
static_assert(
	sizeof(typename hxrestrict_t_<int*>::type) == sizeof(int*),
	"hxrestrict_t_ should preserve pointer representation");
static_assert(hxutility_is_same<hxrestrict_t<int>, int>::value,
	"hxrestrict_t should leave non-pointers untouched");
static_assert(sizeof(hxrestrict_t<int*>) == sizeof(int*),
	"hxrestrict_t should preserve pointer representation");

TEST(hxutility_test, hxnullptr_converts_only_to_null) {
	hxnullptr_t null_object;
	const int* int_ptr = null_object;
	EXPECT_EQ(int_ptr, hxnull);

	struct hxutility_member_holder { int value; };
	int hxutility_member_holder::* member_ptr = null_object;
	EXPECT_EQ(member_ptr, hxnull);
}

TEST(hxutility_test, hxenable_if_filters_on_integral) {
	EXPECT_EQ(hxutility_enable_if_guard(123), 1);
}

TEST(hxutility_test, hxbasename_handles_separators) {
	EXPECT_STREQ(hxbasename("plain"), "plain");
	EXPECT_STREQ(hxbasename("dir/file.bin"), "file.bin");
	EXPECT_STREQ(hxbasename("dir\\file.bin"), "file.bin");
	EXPECT_STREQ(hxbasename("dir/sub\\mixed"), "mixed");
	EXPECT_STREQ(hxbasename("dir/"), "");
}

TEST(hxutility_test, hxisgraph_matches_expected_range) {
	EXPECT_TRUE(hxisgraph('A'));
	EXPECT_TRUE(hxisgraph('~'));
	EXPECT_FALSE(hxisgraph(' '));
	EXPECT_TRUE(hxisgraph(static_cast<char>(0x80)));
	EXPECT_FALSE(hxisgraph('\0'));
}

TEST(hxutility_test, hxisspace_matches_expected_range) {
	EXPECT_TRUE(hxisspace(' '));
	EXPECT_TRUE(hxisspace('\n'));
	EXPECT_TRUE(hxisspace('\t'));
	EXPECT_FALSE(hxisspace('A'));
	EXPECT_FALSE(hxisspace('~'));
}

TEST(hxutility_test, hxlog2i_returns_highest_set_bit) {
	EXPECT_EQ(hxlog2i(1u), 0);
	EXPECT_EQ(hxlog2i(2u), 1);
	EXPECT_EQ(hxlog2i(3u), 1);
	EXPECT_EQ(hxlog2i(16u), 4);
	EXPECT_EQ(hxlog2i((size_t)1u << 20), 20);
}

TEST(hxutility_test, hxisfinite_detects_special_values) {
	union { uint32_t bits; float value; } float_bits;
	union { uint32_t bits; float value; } float_nan_bits;
	union { uint64_t bits; double value; } double_bits;

	float_bits.bits = 0x7f800000u;
	float_nan_bits.bits = 0x7fc00000u;
	double_bits.bits = 0x7ff0000000000000ull;

	EXPECT_TRUE(hxisfinitef(0.0f));
	EXPECT_FALSE(hxisfinitef(float_bits.value));
	EXPECT_FALSE(hxisfinitef(float_nan_bits.value));

	EXPECT_TRUE(hxisfinitel(0.0));
	EXPECT_FALSE(hxisfinitel(double_bits.value));
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

TEST(hxutility_test, hxswap_respects_move_semantics) {
	hxutility_swap_move_tracker left(1);
	hxutility_swap_move_tracker right(2);

	hxswap(left, right);

	EXPECT_EQ(left.value, 2);
	EXPECT_EQ(right.value, 1);
	EXPECT_FALSE(left.moved_from);
	EXPECT_FALSE(right.moved_from);
}

TEST(hxutility_test, hxswap_memcpy_exchanges_trivial_objects) {
	hxutility_memcpy_record first = { 1, 2 };
	hxutility_memcpy_record second = { 3, 4 };

	hxswap_memcpy(first, second);

	EXPECT_EQ(first.first, 3);
	EXPECT_EQ(first.second, 4);
	EXPECT_EQ(second.first, 1);
	EXPECT_EQ(second.second, 2);
}

TEST(hxutility_test, dump_helpers_execute_without_crashing) {
	uint8_t bytes[5] = { 0u, 1u, 2u, 3u, 4u };
	hxhex_dump(bytes, sizeof bytes, 0);
	hxhex_dump(bytes, sizeof bytes, 1);

	float floats[3] = { 0.0f, -1.25f, 2.5f };
	hxfloat_dump(floats, sizeof floats / sizeof floats[0]);
}
