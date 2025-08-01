// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

#include <limits.h>

#if HX_HOSTED
#include <utility>
#endif

HX_REGISTER_FILENAME_HASH

static class hxarray_test* s_hxtest_current = hxnull;

class hxarray_test :
	public testing::Test
{
public:
	class test_object {
	public:
		test_object(void) {
			++s_hxtest_current->m_constructed;
			id = (INT_MIN < s_hxtest_current->m_next_id) ? s_hxtest_current->m_next_id-- : 0;
			constructor = 0;
		}

		test_object(const test_object& rhs) {
			++s_hxtest_current->m_constructed;
			id = rhs.id;
			constructor = rhs.constructor;
		}
		explicit test_object(int32_t x) {
			hxassert(x >= 0); // User supplied IDs are positive
			++s_hxtest_current->m_constructed;
			id = x;
			constructor = 0;
		}
		~test_object(void) {
			++s_hxtest_current->m_destructed;
			id = ~0u;
		}

		void operator=(const test_object& rhs) { id = rhs.id; }
		bool operator==(int32_t x) const { return id == x; }

		int32_t id;
		int32_t constructor;
	};

	hxarray_test(void) {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = -1;
		s_hxtest_current = this;
	}
	~hxarray_test(void) {
		s_hxtest_current = 0;
	}

	bool Check_totals(size_t total) const {
		return m_constructed == total && m_destructed == total;
	}

	size_t m_constructed;
	size_t m_destructed;
	int32_t m_next_id;
};

TEST_F(hxarray_test, null) {
	{
		test_object to0;
		test_object to1;
		EXPECT_EQ(to0.id, -1);
		EXPECT_EQ(to1.id, -2);
	}
	EXPECT_TRUE(Check_totals(2));
}

TEST_F(hxarray_test, empty_full) {
	hxarray<test_object, hxallocator_dynamic_capacity> a;
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(a.full());
	a.reserve(1);
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(!a.full());
	a.push_back(test_object());
	EXPECT_TRUE(!a.empty());
	EXPECT_TRUE(a.full());
	a.pop_back();
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(!a.full());
}

TEST_F(hxarray_test, allocators) {
	hxarray<test_object> objs_dynamic;
	objs_dynamic.reserve(10u);
	hxarray<test_object, 10u> objs_static;

	EXPECT_EQ(objs_dynamic.size(), 0u);
	EXPECT_EQ(objs_static.size(), 0u);

	objs_dynamic.push_back(test_object(20));
	objs_dynamic.push_back(test_object(21));
	objs_static.push_back(test_object(20));
	objs_static.push_back(test_object(21));

	EXPECT_EQ(objs_dynamic.size(), 2u);
	EXPECT_EQ(objs_dynamic[0], 20);
	EXPECT_EQ(objs_dynamic[1], 21);
	EXPECT_EQ(objs_static.size(), 2u);
	EXPECT_EQ(objs_static[0], 20);
	EXPECT_EQ(objs_static[1], 21);

	objs_dynamic.clear();
	objs_static.clear();

	EXPECT_TRUE(Check_totals(8));
}

TEST_F(hxarray_test, iteration) {
	{
		static const int32_t nums[3] = { 21, 22, 23 };

		hxarray<test_object, 10u> objs;
		objs.push_back(test_object(nums[0]));
		objs.push_back(test_object(nums[1]));
		objs.push_back(test_object(nums[2]));

		const hxarray<test_object, 10u>& cobjs = objs;

		int32_t counter = 0;
		for (hxarray<test_object, 10u>::iterator it = objs.begin(); it != objs.end(); ++it) {
			EXPECT_EQ(it->id, objs[counter].id);
			EXPECT_EQ(it->id, nums[counter]);
			++counter;
		}

		counter = 0;
		for (hxarray<test_object, 10u>::const_iterator it = cobjs.begin();
				it != cobjs.end(); ++it) {
			EXPECT_EQ(it->id, objs[counter].id);
			EXPECT_EQ(it->id, nums[counter]);
			++counter;
		}

		EXPECT_EQ(objs.front(), nums[0]);
		EXPECT_EQ(objs.back(), nums[2]);
		EXPECT_EQ(cobjs.front(), nums[0]);
		EXPECT_EQ(cobjs.back(), nums[2]);
	}

	EXPECT_TRUE(Check_totals(6));
}

TEST_F(hxarray_test, modification) {
	{
		static const int32_t nums[5] = { 91, 92, 93, 94, 95 };

		hxarray<test_object> objs;
		EXPECT_FALSE(objs);
		objs.assign(nums, nums + (sizeof nums / sizeof *nums));
		EXPECT_TRUE(objs);

		EXPECT_EQ(objs.capacity(), 5u);
		EXPECT_EQ(objs.size(), 5u);

		// 91, 92, 93, 94, 95

		objs.pop_back();
		objs.pop_back();
		objs.pop_back();

		test_object to;
		objs.push_back(to);
		objs.push_back((const test_object&)to);

		::new (objs.emplace_back_unconstructed()) test_object;

		// 91, 92, -1, -1, -2

		objs.erase_unordered(1); // move end down

		// 91, -2, -1, -1

		EXPECT_EQ(objs.size(), 4);

		static const int32_t nums_2[1] = { 99 };
		hxarray<test_object> objs2;
		objs2.assign(nums_2, nums_2 + 1);
		objs += objs2;

		// 91, -2, -1, -1, 99

		EXPECT_EQ(objs.size(), 5);
		EXPECT_EQ(objs[0].id, 91);
		EXPECT_EQ(objs[1].id, -2);
		EXPECT_EQ(objs[2].id, -1);
		EXPECT_EQ(objs[3].id, -1);
		EXPECT_EQ(objs[4].id, 99);
	}

	EXPECT_TRUE(Check_totals(11));
}

TEST_F(hxarray_test, for_each) {
	static const unsigned char nums[5] = { 91, 92, 93, 94, 95 };
	hxarray<int> objs;
	objs.assign(nums, nums + (sizeof nums / sizeof *nums));

	// 91, 92, 93, 94, 95
	objs.for_each([](int& x) { x -= 90; });

	// 1, 2, 3, 4, 5
	EXPECT_EQ(objs.size(), 5);
	EXPECT_EQ(objs[0], 1);
	EXPECT_EQ(objs[1], 2);
	EXPECT_EQ(objs[2], 3);
	EXPECT_EQ(objs[3], 4);
	EXPECT_EQ(objs[4], 5);

	// Run it empty for correctness
	objs.clear();
	struct Y { void operator()(int&) const { hxassertmsg(0, "internal error"); } } y;
	objs.for_each(y);
}

TEST_F(hxarray_test, resizing) {
	{
		static const int32_t nums[5] = { 51, 52, 53, 54, 55 };

		hxarray<test_object> objs;
		objs.reserve(10);
		objs.assign(nums);

		objs.resize(3);

		EXPECT_EQ(objs.size(), 3u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);

		objs.resize(4u);

		EXPECT_EQ(objs.size(), 4u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);
		EXPECT_EQ(objs[3].id, -1);
		EXPECT_EQ(objs.capacity(), 10u);

		objs.resize(10u);
		EXPECT_EQ(objs.size(), 10u);
		EXPECT_EQ(objs[9].id, -7);

		EXPECT_FALSE(objs.empty());
		objs.clear();
		EXPECT_EQ(objs.size(), 0u);
		EXPECT_TRUE(objs.empty());

		EXPECT_EQ(objs.capacity(), 10u);
	}

	EXPECT_TRUE(Check_totals(12));
}

TEST_F(hxarray_test, assignment) {
	{
		hxarray<test_object> objs;
		objs.reserve(1);

		test_object to;
		to.id = 67;
		objs.push_back(to);

		hxarray<test_object> objs2;
		objs2 = objs; // Assign to same type

		hxarray<test_object, 1> objs3;
		objs3 = objs; // Assign to different type

		hxarray<test_object> objs4(objs); // Construct from same type

		hxarray<test_object, 1> objs5(objs); // Construct from different type

		EXPECT_EQ(objs2.size(), 1u);
		EXPECT_EQ(objs3.size(), 1u);
		EXPECT_EQ(objs4.size(), 1u);
		EXPECT_EQ(objs5.size(), 1u);

		EXPECT_EQ(objs2[0].id, 67);
		EXPECT_EQ(objs3[0].id, 67);
		EXPECT_EQ(objs4[0].id, 67);
		EXPECT_EQ(objs5[0].id, 67);
	}

	EXPECT_TRUE(Check_totals(6));
}

#if HX_HOSTED
TEST_F(hxarray_test, initializer_list) {
	hxarray<int, 2> x = { 2, 7 };
	EXPECT_EQ(x[1], 7);

	hxarray<int> y { 12, 17 };
	EXPECT_EQ(y[1], 17);
}

TEST_F(hxarray_test, temporaries) {
	// test r-value dynamically allocated temporaries
	{
		hxsystem_allocator_scope allocator_scope(hxsystem_allocator_temporary_stack);

		hxarray<int> x(hxarray<int>({ 2, 7 }));
		hxarray<int> y = std::move(x); // should swap
		hxarray<int> z;
		hxswap(y, z);
		EXPECT_TRUE(x.empty());
		EXPECT_TRUE(y.empty());
		EXPECT_EQ(z[0], 2);
		EXPECT_EQ(z[1], 7);
	}
}
#endif
