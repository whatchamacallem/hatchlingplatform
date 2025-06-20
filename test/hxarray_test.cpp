// Copyright 2017-2025 Adrian Johnston

#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

#include <limits.h>

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
#include <utility>
#endif

HX_REGISTER_FILENAME_HASH

static class hxarray_test* s_hxtest_current = hxnull;

class hxarray_test :
	public testing::Test
{
public:
	struct Test_object {
		Test_object() {
			++s_hxtest_current->m_constructed;
			id = (INT_MIN < s_hxtest_current->m_next_id) ? s_hxtest_current->m_next_id-- : 0;
			constructor = 0;
		}

		Test_object(const Test_object& rhs) {
			++s_hxtest_current->m_constructed;
			id = rhs.id;
			constructor = rhs.constructor;
		}
		explicit Test_object(int32_t x) {
			hxassert(x >= 0); // User supplied IDs are positive
			++s_hxtest_current->m_constructed;
			id = x;
			constructor = 0;
		}
		~Test_object() {
			++s_hxtest_current->m_destructed;
			id = ~0u;
		}

		void operator=(const Test_object& rhs) { id = rhs.id; }
		bool operator==(int32_t x) const { return id == x; }

		int32_t id;
		int32_t constructor;
	};

	hxarray_test() {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = -1;
		s_hxtest_current = this;
	}
	~hxarray_test() {
		s_hxtest_current = 0;
	}

	bool Check_totals(size_t total) const {
		return m_constructed == total && m_destructed == total;
	}

	size_t m_constructed;
	size_t m_destructed;
	int32_t m_next_id;
};

TEST_F(hxarray_test, Null) {
	{
		Test_object to0;
		Test_object to1;
		ASSERT_EQ(to0.id, -1);
		ASSERT_EQ(to1.id, -2);
	}
	ASSERT_TRUE(Check_totals(2));
}

TEST_F(hxarray_test, Empty_full) {
	hxarray<Test_object, hxallocator_dynamic_capacity> a;
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(a.full());
	a.reserve(1);
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(!a.full());
	a.push_back(Test_object());
	ASSERT_TRUE(!a.empty());
	ASSERT_TRUE(a.full());
	a.pop_back();
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(!a.full());
}

TEST_F(hxarray_test, Allocators) {
	hxarray<Test_object> objs_dynamic;
	objs_dynamic.reserve(10u);
	hxarray<Test_object, 10u> objs_static;

	ASSERT_EQ(objs_dynamic.size(), 0u);
	ASSERT_EQ(objs_static.size(), 0u);

	objs_dynamic.push_back(Test_object(20));
	objs_dynamic.push_back(Test_object(21));
	objs_static.push_back(Test_object(20));
	objs_static.push_back(Test_object(21));

	ASSERT_EQ(objs_dynamic.size(), 2u);
	ASSERT_EQ(objs_dynamic[0], 20);
	ASSERT_EQ(objs_dynamic[1], 21);
	ASSERT_EQ(objs_static.size(), 2u);
	ASSERT_EQ(objs_static[0], 20);
	ASSERT_EQ(objs_static[1], 21);

	objs_dynamic.clear();
	objs_static.clear();

	ASSERT_TRUE(Check_totals(8));
}

TEST_F(hxarray_test, Iteration) {
	{
		static const int32_t nums[3] = { 21, 22, 23 };

		hxarray<Test_object, 10u> objs;
		objs.push_back(Test_object(nums[0]));
		objs.push_back(Test_object(nums[1]));
		objs.push_back(Test_object(nums[2]));

		const hxarray<Test_object, 10u>& cobjs = objs;

		int32_t counter = 0;
		for (hxarray<Test_object, 10u>::iterator it = objs.begin(); it != objs.end(); ++it) {
			ASSERT_EQ(it->id, objs[counter].id);
			ASSERT_EQ(it->id, nums[counter]);
			++counter;
		}

		counter = 0;
		for (hxarray<Test_object, 10u>::const_iterator it = cobjs.begin();
				it != cobjs.end(); ++it) {
			ASSERT_EQ(it->id, objs[counter].id);
			ASSERT_EQ(it->id, nums[counter]);
			++counter;
		}

		ASSERT_EQ(objs.front(), nums[0]);
		ASSERT_EQ(objs.back(), nums[2]);
		ASSERT_EQ(cobjs.front(), nums[0]);
		ASSERT_EQ(cobjs.back(), nums[2]);
	}

	ASSERT_TRUE(Check_totals(6));
}

TEST_F(hxarray_test, Modification) {
	{
		static const int32_t nums[5] = { 91, 92, 93, 94, 95 };

		hxarray<Test_object> objs;
		objs.assign(nums, nums + (sizeof nums / sizeof *nums));

		ASSERT_EQ(objs.capacity(), 5u);
		ASSERT_EQ(objs.size(), 5u);

		// 91, 92, 93, 94

		objs.pop_back();
		objs.pop_back();
		objs.pop_back();

		Test_object to;
		objs.push_back(to);
		objs.push_back((const Test_object&)to);

		::new (objs.emplace_back_unconstructed()) Test_object;

		// 91, 92, -1, -2, -3

		objs.erase_unordered(1);

		// 91, -2, -1

		ASSERT_EQ(objs[0].id, 91);
		ASSERT_EQ(objs[1].id, -2);
		ASSERT_EQ(objs[2].id, -1);
	}

	ASSERT_TRUE(Check_totals(9));
}

TEST_F(hxarray_test, Resizing) {
	{
		static const int32_t nums[5] = { 51, 52, 53, 54, 55 };

		hxarray<Test_object> objs;
		objs.reserve(10);
		objs.assign(nums);

		objs.resize(3);

		ASSERT_EQ(objs.size(), 3u);
		ASSERT_EQ(objs[0].id, 51);
		ASSERT_EQ(objs[2].id, 53);

		objs.resize(4u);

		ASSERT_EQ(objs.size(), 4u);
		ASSERT_EQ(objs[0].id, 51);
		ASSERT_EQ(objs[2].id, 53);
		ASSERT_EQ(objs[3].id, -1);
		ASSERT_EQ(objs.capacity(), 10u);

		objs.resize(10u);
		ASSERT_EQ(objs.size(), 10u);
		ASSERT_EQ(objs[9].id, -7);

		ASSERT_FALSE(objs.empty());
		objs.clear();
		ASSERT_EQ(objs.size(), 0u);
		ASSERT_TRUE(objs.empty());

		ASSERT_EQ(objs.capacity(), 10u);
	}

	ASSERT_TRUE(Check_totals(12));
}

TEST_F(hxarray_test, Assignment) {
	{
		hxarray<Test_object> objs;
		objs.reserve(1);

		Test_object to;
		to.id = 67;
		objs.push_back(to);

		hxarray<Test_object> objs2;
		objs2 = objs; // Assign to same type

		hxarray<Test_object, 1> objs3;
		objs3 = objs; // Assign to different type

		hxarray<Test_object> objs4(objs); // Construct from same type

		hxarray<Test_object, 1> objs5(objs); // Construct from different type

		ASSERT_EQ(objs2.size(), 1u);
		ASSERT_EQ(objs3.size(), 1u);
		ASSERT_EQ(objs4.size(), 1u);
		ASSERT_EQ(objs5.size(), 1u);

		ASSERT_EQ(objs2[0].id, 67);
		ASSERT_EQ(objs3[0].id, 67);
		ASSERT_EQ(objs4[0].id, 67);
		ASSERT_EQ(objs5[0].id, 67);
	}

	ASSERT_TRUE(Check_totals(6));
}

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
TEST_F(hxarray_test, Initializer_list) {
	hxarray<int, 2> x = { 2, 7 };
	ASSERT_EQ(x[1], 7);

	hxarray<int> y { 12, 17 };
	ASSERT_EQ(y[1], 17);
}

TEST_F(hxarray_test, Temporaries) {
	// test r-value dynamically allocated temporaries
	{
		hxmemory_allocator_scope allocator_scope(hxmemory_allocator_Temporary_stack);

		hxarray<int> x(hxarray<int>({ 2, 7 }));
		hxarray<int> y = std::move(x); // should swap
		hxarray<int> z;
		hxswap(y, z);
		ASSERT_TRUE(x.empty());
		ASSERT_TRUE(y.empty());
		ASSERT_EQ(z[0], 2);
		ASSERT_EQ(z[1], 7);
	}
}
#endif
