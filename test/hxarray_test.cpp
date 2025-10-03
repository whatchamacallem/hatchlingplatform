// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

#include <limits.h>

#if !HX_NO_LIBCXX
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

		test_object(const test_object& x) {
			++s_hxtest_current->m_constructed;
			id = x.id;
		}
		explicit test_object(int32_t x) {
			hxassert(x >= 0); // User supplied IDs are positive
			++s_hxtest_current->m_constructed;
			id = x;
		}
		~test_object(void) {
			++s_hxtest_current->m_destructed;
			id = -1;
		}

		void operator=(const test_object& x) { id = x.id; }
		bool operator==(int32_t x) const { return id == x; }
		bool operator==(const test_object& x) const { return id == x.id; }
		bool operator<(const test_object& x) const { return id < x.id; }

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

	bool check_totals(size_t total) const {
		return m_constructed == total && m_destructed == total;
	}

	size_t m_constructed;
	size_t m_destructed;
	int32_t m_next_id;
};

struct hxarray_test_move_tracker {
	int32_t value;
	bool moved_from;

	hxarray_test_move_tracker(void)
		: value(0), moved_from(false) { }

	explicit hxarray_test_move_tracker(int32_t value_)
		: value(value_), moved_from(false) { }

	hxarray_test_move_tracker(const hxarray_test_move_tracker& other_)
		: value(other_.value), moved_from(false) { }

	hxarray_test_move_tracker(hxarray_test_move_tracker&& other_)
		: value(other_.value), moved_from(false) { other_.moved_from = true; }

	hxarray_test_move_tracker& operator=(const hxarray_test_move_tracker& other_) {
		value = other_.value;
		moved_from = false;
		return *this;
	}

	hxarray_test_move_tracker& operator=(hxarray_test_move_tracker&& other_) {
		value = other_.value;
		moved_from = false;
		other_.moved_from = true;
		return *this;
	}

	bool operator==(const hxarray_test_move_tracker& other_) const {
		return value == other_.value && moved_from == other_.moved_from;
	}
};

template<typename T>
class hxarray_test_pointer_range {
public:
	hxarray_test_pointer_range(T* begin_, T* end_)
		: begin_(begin_), end_(end_) { }

	T* begin() { return begin_; }
	T* end() { return end_; }

	const T& operator[](size_t index_) const { return begin_[index_]; }
	T& operator[](size_t index_) { return begin_[index_]; }

private:
	T* begin_;
	T* end_;
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

	EXPECT_TRUE(check_totals(8));
}

TEST_F(hxarray_test, iteration) {
	{
		static const int32_t nums[3] = { 21, 22, 23 };

		hxarray<test_object, 10u> objs;
		objs.push_back(test_object(nums[0]));
		objs.push_back(test_object(nums[1]));
		objs.push_back(test_object(nums[2]));

		const hxarray<test_object, 10u>& cobjs = objs;

		uint32_t counter = 0u;
		for(hxarray<test_object, 10u>::iterator it = objs.begin(); it != objs.end(); ++it) {
			EXPECT_EQ(it->id, objs[counter].id);
			EXPECT_EQ(it->id, nums[counter]);
			++counter;
		}

		counter = 0u;
		for(hxarray<test_object, 10u>::const_iterator it = cobjs.begin();
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

	EXPECT_TRUE(check_totals(6));
}

TEST_F(hxarray_test, get) {
	{
		hxarray<test_object, 4u> objs; objs.reserve(4u);
		objs.emplace_back(10);
		objs.emplace_back(20);

		const hxarray<test_object, 4u>& cobjs = objs;

		EXPECT_EQ(objs.get(0), objs.begin());
		EXPECT_EQ(objs.get(1), objs.begin() + 1);
		EXPECT_EQ(objs.get(2), hxnull);

		EXPECT_EQ(cobjs.get(0), cobjs.begin());
		EXPECT_EQ(cobjs.get(1), cobjs.begin() + 1);
		EXPECT_EQ(cobjs.get(2), hxnull);

		objs.pop_back();
		EXPECT_EQ(objs.get(1), hxnull);
	}

	EXPECT_TRUE(check_totals(2));
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

		objs.emplace_back();

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

	EXPECT_TRUE(check_totals(11));
}

TEST_F(hxarray_test, emplace_back) {
	{
		hxarray<test_object> objs;
		objs.reserve(3u);

		test_object& default_inserted = objs.emplace_back();
		EXPECT_EQ(objs.data(), &default_inserted);
		EXPECT_EQ(default_inserted.id, -1);

		test_object original(42);
		test_object& copy_inserted = objs.emplace_back(original);
		EXPECT_EQ(objs.data() + 1, &copy_inserted);
		EXPECT_EQ(copy_inserted.id, original.id);

		test_object& value_inserted = objs.emplace_back(77);
		EXPECT_EQ(objs.data() + 2, &value_inserted);
		EXPECT_EQ(value_inserted.id, 77);

		EXPECT_EQ(objs.size(), 3u);
		EXPECT_EQ(objs.back().id, 77);
	}

	EXPECT_TRUE(check_totals(4));
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
		objs = nums;

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

	EXPECT_TRUE(check_totals(25));
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

	EXPECT_TRUE(check_totals(6));
}

TEST(hxarray_test, assign_range_from_const) {
	const int32_t assigned_element_ints[] = { 4, 7, 11, 18 };
	const hxarray<hxarray_test_move_tracker> assigned_elements = assigned_element_ints;
	const size_t assigned_count = sizeof assigned_element_ints / sizeof *assigned_element_ints;

	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(assigned_count + 1u);
	elements.push_back(91);

	hxarray_test_pointer_range<const hxarray_test_move_tracker> range(
		assigned_elements.begin(), assigned_elements.end());
	elements.assign_range(range);

	EXPECT_EQ(elements.size(), assigned_count);
	EXPECT_EQ(elements[0], assigned_elements[0]);
	EXPECT_EQ(elements[1], assigned_elements[1]);
	EXPECT_EQ(elements[2], assigned_elements[2]);
	EXPECT_EQ(elements[3], assigned_elements[3]);
	for(size_t i = 0u; i < assigned_count; ++i) {
		EXPECT_FALSE(elements[i].moved_from);
		EXPECT_FALSE(assigned_elements[i].moved_from);
	}
}

TEST(hxarray_test, assign_range_from_mutable_range) {
	static hxarray_test_move_tracker source_elements[] = {
		hxarray_test_move_tracker(2),
		hxarray_test_move_tracker(3),
		hxarray_test_move_tracker(5)
	};
	const size_t source_count = sizeof source_elements / sizeof *source_elements;

	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(source_count);

	hxarray_test_pointer_range<hxarray_test_move_tracker> range(
		source_elements, source_elements + source_count);
	elements.assign_range(range);

	EXPECT_EQ(elements.size(), source_count);
	EXPECT_EQ(elements[0].value, 2);
	EXPECT_EQ(elements[1].value, 3);
	EXPECT_EQ(elements[2].value, 5);
	for(size_t i = 0u; i < source_count; ++i) {
		EXPECT_FALSE(elements[i].moved_from);
		EXPECT_FALSE(source_elements[i].moved_from);
	}
}

TEST(hxarray_test, assign_range_from_rvalue) {
	static hxarray_test_move_tracker source_elements[] = {
		hxarray_test_move_tracker(5),
		hxarray_test_move_tracker(9),
		hxarray_test_move_tracker(13)
	};
	const size_t source_count = sizeof source_elements / sizeof *source_elements;

	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(source_count);
	elements.assign_range(hxarray_test_pointer_range(
		source_elements, source_elements + source_count));

	EXPECT_EQ(elements.size(), source_count);
	EXPECT_EQ(elements[0].value, 5);
	EXPECT_EQ(elements[1].value, 9);
	EXPECT_EQ(elements[2].value, 13);
	for(size_t i = 0u; i < source_count; ++i) {
		EXPECT_FALSE(elements[i].moved_from);
		EXPECT_TRUE(source_elements[i].moved_from);
	}
}

TEST(hxarray_test, push_back_move_tracker) {
	hxarray_test_move_tracker source(42);
	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(3u);
	elements.push_back(hxmove(source));

	EXPECT_EQ(elements.size(), 1u);
	EXPECT_EQ(elements[0].value, 42);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_TRUE(source.moved_from);

	// Confirm the array does not get moved.
	hxarray_test_move_tracker other(84);
	elements.push_back(other);

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[1].value, 84);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_FALSE(other.moved_from);
}

TEST(hxarray_test, plus_equals_move_tracker_element) {
	hxarray_test_move_tracker source(5);
	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(3u);
	elements += hxmove(source);

	EXPECT_EQ(elements.size(), 1u);
	EXPECT_EQ(elements[0].value, 5);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_TRUE(source.moved_from);

	// Confirm the array does not get moved.
	hxarray_test_move_tracker other(11);
	elements += other;

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[1].value, 11);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_FALSE(other.moved_from);
}

TEST(hxarray_test, plus_equals_move_tracker_array) {
	hxarray_test_move_tracker initial(1);
	static const int32_t appended_values[] = { 3, 5, 7 };

	hxarray<hxarray_test_move_tracker> move_target;
	move_target.reserve(5u);
	move_target.push_back(initial);

	hxarray<hxarray_test_move_tracker> copy_target;
	copy_target.reserve(5u);
	copy_target.push_back(initial);

	EXPECT_FALSE(initial.moved_from);

	hxarray<hxarray_test_move_tracker> move_source;
	move_source = appended_values;

	hxarray<hxarray_test_move_tracker> copy_source;
	copy_source = appended_values;

	// Confirm the array of temporaries does actually get moved.
	move_target += hxmove(move_source);

	EXPECT_EQ(move_target.size(), 4u);
	EXPECT_EQ(move_target[0].value, 1);
	EXPECT_EQ(move_target[1].value, 3);
	EXPECT_EQ(move_target[2].value, 5);
	EXPECT_EQ(move_target[3].value, 7);
	for(size_t i = 0u; i < move_target.size(); ++i) {
		EXPECT_FALSE(move_target[i].moved_from);
	}
	EXPECT_EQ(move_source.size(), 3u);
	for(size_t i = 0u; i < move_source.size(); ++i) {
		EXPECT_TRUE(move_source[i].moved_from);
	}

	// Confirm the lvalue source does not get moved.
	copy_target += copy_source;

	EXPECT_EQ(copy_target.size(), 4u);
	EXPECT_EQ(copy_target[0].value, 1);
	EXPECT_EQ(copy_target[1].value, 3);
	EXPECT_EQ(copy_target[2].value, 5);
	EXPECT_EQ(copy_target[3].value, 7);
	for(size_t i = 0u; i < copy_target.size(); ++i) {
		EXPECT_FALSE(copy_target[i].moved_from);
	}
	EXPECT_EQ(copy_source.size(), 3u);
	for(size_t i = 0u; i < copy_source.size(); ++i) {
		EXPECT_FALSE(copy_source[i].moved_from);
	}
}

TEST(hxarray_test, insert_move_tracker_move_and_copy) {
	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(4u);

	hxarray_test_move_tracker initial(10);
	elements.push_back(initial);
	EXPECT_FALSE(initial.moved_from);

	hxarray_test_move_tracker move_source(20);
	elements.insert(elements.begin(), hxmove(move_source));

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[0].value, 20);
	EXPECT_EQ(elements[1].value, 10);
	EXPECT_TRUE(move_source.moved_from);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);

	hxarray_test_move_tracker copy_source(30);
	elements.insert(2u, copy_source);

	EXPECT_EQ(elements.size(), 3u);
	EXPECT_EQ(elements[0].value, 20);
	EXPECT_EQ(elements[1].value, 10);
	EXPECT_EQ(elements[2].value, 30);
	EXPECT_FALSE(copy_source.moved_from);
	for(size_t i = 0u; i < elements.size(); ++i) {
		EXPECT_FALSE(elements[i].moved_from);
	}
}

TEST(hxarray_test, emplace_back_move_tracker_forwarding) {
	hxarray<hxarray_test_move_tracker> elements;
	elements.reserve(3u);

	hxarray_test_move_tracker copy_source(40);
	hxarray_test_move_tracker move_source(50);

	hxarray_test_move_tracker& copy_inserted = elements.emplace_back(copy_source);
	EXPECT_EQ(&copy_inserted, &elements[0]);
	EXPECT_EQ(elements[0].value, 40);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(copy_source.moved_from);

	hxarray_test_move_tracker& move_inserted = elements.emplace_back(hxmove(move_source));
	EXPECT_EQ(&move_inserted, &elements[1]);
	EXPECT_EQ(elements[1].value, 50);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_TRUE(move_source.moved_from);

	EXPECT_EQ(elements.size(), 2u);
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

	EXPECT_TRUE(check_totals(22));
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

	EXPECT_TRUE(check_totals(9));
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

	EXPECT_TRUE(check_totals(18)); // <-- This is why we don't use insert.
}
#endif

TEST_F(hxarray_test, c_initializer_list) {
	int i0[] = { 2, 7 };
	hxarray<int, 2> x(i0);
	EXPECT_EQ(x[1], 7);

	int i1[] = { 12, 17 };
	hxarray<int> y(i1);
	EXPECT_EQ(y[1], 17);

	y = i0;
	EXPECT_EQ(y[1], 7);

	hxarray<char, HX_MAX_LINE> z("prefix array 1");
	while(z[0] != 'a') {
		z.erase((size_t)0);
	}
	EXPECT_TRUE(::strcmp(z.data(), "array 1") == 0);

	z = "array 2";
	EXPECT_TRUE(::strcmp(z.data(), "array 2") == 0);
}

#if !HX_NO_LIBCXX
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
