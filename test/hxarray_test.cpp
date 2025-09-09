// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

#include <limits.h>

#if !HX_FREESTANDING
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
			id = s_hxtest_current->m_next_id--;
		}

		test_object(const test_object& rhs) {
			++s_hxtest_current->m_constructed;
			id = rhs.id;
		}
		explicit test_object(int32_t x) {
			hxassert(x >= 0); // User supplied IDs are positive
			++s_hxtest_current->m_constructed;
			id = x;
		}
		~test_object(void) {
			++s_hxtest_current->m_destructed;
			id = ~0u;
		}

		void operator=(const test_object& rhs) { id = rhs.id; }
		bool operator==(int32_t x) const { return id == x; }
		bool operator==(const test_object& rhs) const { return id == rhs.id; }
		bool operator<(const test_object& rhs) const { return id < rhs.id; }

		int32_t id;
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
		objs.assign(nums, nums + (sizeof nums / sizeof *nums));
		EXPECT_FALSE(objs.empty());

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

	// 91, 92, 93, 94, 95. Lambdas are typically temporaries.
	objs.for_each([](int& x) { x -= 90; });

	hxarray<int>& objs_ref = objs;

	// 1, 2, 3, 4, 5
	EXPECT_EQ(objs_ref.size(), 5);
	EXPECT_EQ(objs_ref[0], 1);
	EXPECT_EQ(objs_ref[1], 2);
	EXPECT_EQ(objs_ref[2], 3);
	EXPECT_EQ(objs_ref[3], 4);
	EXPECT_EQ(objs_ref[4], 5);

	// Count the objects with a non-temporary functor.
	struct X { int n; X() : n(0) { }; void operator()(int&) { ++n; } } x;
	objs.for_each(x);
	EXPECT_EQ(x.n, 5);

	// Run it empty for correctness
	objs.clear();
	struct Y { void operator()(int&) const { hxassertmsg(0, "internal error"); } } y;
	objs.for_each(y);
}

TEST_F(hxarray_test, resizing) {
	{
		static const int32_t nums[5] = { 51, 52, 53, 54, 55 };

		hxarray<test_object> objs(12);
		objs.reserve(10); // reserve less than is being used.
		objs.assign(nums);

		// Use the 2 arg version to delete it.
		objs.resize(3, test_object());

		EXPECT_EQ(objs.size(), 3u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);

		objs.resize(4u);

		EXPECT_EQ(objs.size(), 4u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);
		EXPECT_EQ(objs[3].id, -14);
		EXPECT_EQ(objs.capacity(), 12u);

		objs.resize(10u);
		EXPECT_EQ(objs.size(), 10u);
		EXPECT_EQ(objs[9].id, -20);

		EXPECT_FALSE(objs.empty());
		objs.clear();
		EXPECT_EQ(objs.size(), 0u);
		EXPECT_TRUE(objs.empty());

		EXPECT_EQ(objs.capacity(), 12u);
	}

	EXPECT_TRUE(Check_totals(25));
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

#if HX_CPLUSPLUS >= 202002L
TEST_F(hxarray_test, plus_equals) {
	{
		hxarray<test_object> objs;
		objs.reserve(10);

		objs += hxarray<test_object>{ 1, 7, 11 };

		hxarray<test_object> objs2 { 10, 70, 110 };
		objs += objs2;

		hxarray<test_object> objs3 { 1, 7, 11, 10, 70, 110 };

		EXPECT_TRUE(hxkey_equal(objs, objs3));
		EXPECT_FALSE(hxkey_less(objs, objs3));

		// Compare inequal length and a temp.
		test_object t(440);
		objs += t;
		EXPECT_FALSE(hxkey_equal(objs, objs3));
		EXPECT_TRUE(hxkey_less(objs3, objs));

		// Compare equal length and a non-temp.
		objs.resize(5);
		objs += test_object(220);
		EXPECT_FALSE(hxkey_equal(objs, objs3));
		EXPECT_TRUE(hxkey_less(objs3, objs));
	}

	EXPECT_TRUE(Check_totals(22));
}

TEST_F(hxarray_test, erase) {
	{
		hxarray<test_object> objs { 1, 2, 3, 4, 5 };
		objs.erase(1);
		objs.erase(objs.begin() + 2);

		hxarray<test_object> expected { 1, 3, 5 };
		EXPECT_TRUE(hxkey_equal(objs, expected));

		objs.erase(objs.begin());
		objs.erase(objs.end() - 1);

		hxarray<test_object> final_expected { 3 };
		EXPECT_TRUE(hxkey_equal(objs, final_expected));
	}

	EXPECT_TRUE(Check_totals(9));
}

TEST_F(hxarray_test, insert) {
	{
		// The numeric constant zero is also a pointer. It seems more convenient
		// to allow both indicies and pointers than to worry about it.
		hxarray<test_object> objs; objs.reserve(5);
		objs.push_back(test_object(3));
		objs.insert(objs.begin(), test_object(1)); // Inserting at beginning.
		objs.insert(2, test_object(5)); // Inserting past the end.

		hxarray<test_object> expected { 1, 3, 5 };
		EXPECT_TRUE(hxkey_equal(objs, expected));

		objs.insert(1, test_object(2));
		objs.insert(3, test_object(4));

		hxarray<test_object> final_expected { 1, 2, 3, 4, 5 };
		EXPECT_TRUE(hxkey_equal(objs, final_expected));
	}

	EXPECT_TRUE(Check_totals(18)); // <-- This is why we don't use insert.
}
#endif

#if !HX_FREESTANDING
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
