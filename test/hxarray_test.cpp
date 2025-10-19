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

static class hxarray_test_f* s_hxtest_current = hxnull;

class hxarray_test_f :
	public testing::Test
{
public:
	class hxtest_object {
	public:
		hxtest_object(void) {
			++s_hxtest_current->m_constructed;
			id = s_hxtest_current->m_next_id--;
			moved_from = false;
		}

		hxtest_object(const hxtest_object& x) {
			++s_hxtest_current->m_constructed;
			id = x.id;
			moved_from = false;
		}
		explicit hxtest_object(int32_t x) {
			EXPECT_TRUE(x >= 0); // User supplied IDs are positive.
			++s_hxtest_current->m_constructed;
			id = x;
			moved_from = false;
		}
		hxtest_object(hxtest_object&& x) {
			++s_hxtest_current->m_constructed;
			id = x.id;
			moved_from = false;
			x.id = 0xefef; // Poison value;
			x.moved_from = true;
		}
		~hxtest_object(void) {
			++s_hxtest_current->m_destructed;
			id = 0xefef; // Poison value;
			moved_from = true;
		}

		void operator=(const hxtest_object& x) {
			hxassert(this != &x);
			id = x.id;
			moved_from = false;
		}
		hxtest_object& operator=(hxtest_object&& x) {
			hxassert(this != &x);
			id = x.id;
			moved_from = false;
			x.id = 0xefef;
			x.moved_from = true;
			return *this;
		}
		bool operator==(int32_t x) const { return id == x; }
		bool operator==(const hxtest_object& x) const { return id == x.id; }
		bool operator<(const hxtest_object& x) const { return id < x.id; }

		bool moved_from;
		int32_t id;
	};

	hxarray_test_f(void) {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = -1;
		s_hxtest_current = this;
	}
	~hxarray_test_f(void) {
		hxassertrelease(m_constructed == m_destructed, "hxarray_test_f Test object lifecycle error.");
		s_hxtest_current = 0;
	}

	bool check_totals(size_t total) const {
		return m_constructed == total && m_destructed == total;
	}

	size_t m_constructed;
	size_t m_destructed;
	int32_t m_next_id;
};

template<typename T>
class hxarray_test_pointer_range {
public:
	hxarray_test_pointer_range(T* b, T* e)
		: begin_ptr(b), end_ptr(e) { }

	T* begin() { return begin_ptr; }
	T* end() { return end_ptr; }

	const T& operator[](size_t index) const { return begin_ptr[index]; }
	T& operator[](size_t index) { return begin_ptr[index]; }

private:
	T* begin_ptr;
	T* end_ptr;
};

template<typename array_t>
static bool hxarray_test_is_max_heap(const array_t& heap) {
	const size_t size = heap.size();
	for(size_t parent = 0; parent != size; ++parent) {
		const size_t left = (parent << 1) + 1;
		const size_t right = left + 1;
		if(left < size && hxkey_less(heap[parent], heap[left])) {
			return false;
		}
		if(right < size && hxkey_less(heap[parent], heap[right])) {
			return false;
		}
	}
	return true;
}

TEST_F(hxarray_test_f, empty_full) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxarray<hxtest_object, hxallocator_dynamic_capacity> a;
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(a.full());
	a.reserve(1);
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(!a.full());
	a.push_back(hxtest_object());
	EXPECT_TRUE(!a.empty());
	EXPECT_FALSE(a[0].moved_from);
	EXPECT_TRUE(a.full());
	a.pop_back();
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(!a.full());
}

TEST_F(hxarray_test_f, iteration) {
	{
		static const int32_t nums[3] = { 21, 22, 23 };

		hxarray<hxtest_object, 10u> objs;
		objs.push_back(hxtest_object(nums[0]));
		objs.push_back(hxtest_object(nums[1]));
		objs.push_back(hxtest_object(nums[2]));

		const hxarray<hxtest_object, 10u>& cobjs = objs;

		uint32_t counter = 0u;
		for(hxarray<hxtest_object, 10u>::iterator it = objs.begin(); it != objs.end(); ++it) {
			EXPECT_EQ(it->id, objs[counter].id);
			EXPECT_EQ(it->id, nums[hxmin<uint32_t>(counter, 2u)]);
			EXPECT_FALSE(objs[counter].moved_from);
			++counter;
		}

		counter = 0u;
		for(hxarray<hxtest_object, 10u>::const_iterator it = cobjs.begin();
				it != cobjs.end(); ++it) {
			EXPECT_EQ(it->id, objs[counter].id);
			EXPECT_EQ(it->id, nums[hxmin<uint32_t>(counter, 2u)]);
			EXPECT_FALSE(cobjs[counter].moved_from);
			++counter;
		}

		EXPECT_EQ(objs.front(), nums[0]);
		EXPECT_EQ(objs.back(), nums[2]);
		EXPECT_EQ(cobjs.front(), nums[0]);
		EXPECT_EQ(cobjs.back(), nums[2]);
	}

	EXPECT_TRUE(check_totals(6));
}

TEST_F(hxarray_test_f, get) {
	{
		hxarray<hxtest_object, 4u> objs; objs.reserve(4u);
		objs.emplace_back(10);
		objs.emplace_back(20);

		const hxarray<hxtest_object, 4u>& cobjs = objs;

		// "Returns a const T* to the element at index or hxnull otherwise." Validate lookups
		// for indices { 0, 1, 2 } on mutable + const views.
		EXPECT_EQ(objs.get(0), objs.begin());
		EXPECT_EQ(objs.get(1), objs.begin() + 1);
		EXPECT_EQ(objs.get(2), hxnullptr);

		EXPECT_EQ(cobjs.get(0), cobjs.begin());
		EXPECT_EQ(cobjs.get(1), cobjs.begin() + 1);
		EXPECT_EQ(cobjs.get(2), hxnullptr);

		objs.pop_back();
		EXPECT_EQ(objs.get(1), hxnullptr);
	}

	EXPECT_TRUE(check_totals(2));
}

TEST_F(hxarray_test_f, modification) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		static const int32_t nums[5] = { 91, 92, 93, 94, 95 };

		hxarray<hxtest_object> objs(nums);
		EXPECT_FALSE(objs.empty());

		EXPECT_EQ(objs.capacity(), 5u);
		EXPECT_EQ(objs.size(), 5u);

		// 91, 92, 93, 94, 95

		objs.pop_back();
		objs.pop_back();
		objs.pop_back();

		hxtest_object to;
		objs.push_back(to);
		objs.push_back((const hxtest_object&)to);

		objs.emplace_back();

		// 91, 92, -1, -1, -2

		objs.erase_unordered(1); // Move end down.

		// 91, -2, -1, -1

		EXPECT_EQ(objs.size(), 4);

		static const int32_t nums_2[1] = { 99 };
		const hxarray<hxtest_object> objs2(nums_2);
		objs += objs2;

		// 91, -2, -1, -1, 99

		EXPECT_EQ(objs.size(), 5);
		EXPECT_EQ(objs[0].id, 91);
		EXPECT_EQ(objs[1].id, -2);
		EXPECT_EQ(objs[2].id, -1);
		EXPECT_EQ(objs[3].id, -1);
		EXPECT_EQ(objs[4].id, 99);
		for(size_t i = 0u; i < objs.size(); ++i) {
			EXPECT_FALSE(objs[i].moved_from);
		}
	}

	EXPECT_TRUE(check_totals(11));
}

TEST(hxarray_test, push_heap_preserves_heap_property) {
	static const int values[] = { 3, 7, 1, 9, 5, 8 };
	const size_t value_count = hxsize(values);

	hxarray<int, 16u> heap;
	int max_value = INT_MIN;
	for(size_t index = 0; index < value_count; ++index) {
		const int value = values[index];
		heap.push_heap(value);
		if(value > max_value) {
			max_value = value;
		}
		EXPECT_TRUE(hxarray_test_is_max_heap(heap));
		EXPECT_EQ(heap.front(), max_value);
	}

	EXPECT_EQ(heap.size(), value_count);
}

TEST(hxarray_test, pop_heap_preserves_heap_on_removal) {
	static const int values[] = { 5, 12, 3, 7, 9, 4, 15, 5 };
	const size_t value_count = hxsize(values);

	hxarray<int, 16u> heap;
	for(size_t index = 0; index < value_count; ++index) {
		heap.push_heap(values[index]);
	}

	EXPECT_TRUE(hxarray_test_is_max_heap(heap));
	EXPECT_EQ(heap.size(), value_count);

	hxarray<int, 16u> removed;
	size_t expected_size = value_count;
	while(!heap.empty()) {
		const int root_value = heap.front();
		removed.push_back(root_value);
		heap.pop_heap();
		--expected_size;
		EXPECT_EQ(heap.size(), expected_size);
		EXPECT_TRUE(hxarray_test_is_max_heap(heap));
		for(size_t index = 0; index < heap.size(); ++index) {
			EXPECT_LE(heap[index], root_value);
		}
	}

	EXPECT_TRUE(heap.empty());
	EXPECT_EQ(removed.size(), value_count);

	static const int expected[] = { 15, 12, 9, 7, 5, 5, 4, 3 };
	for(size_t index = 0; index < value_count; ++index) {
		EXPECT_EQ(removed[index], expected[index]);
	}
}

TEST(hxarray_test, erase_if_heap_removes_matching_values) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const int values[] = { 14, 3, 7, 2, 9, 5, 8, 1, 6 };
	const size_t value_count = hxsize(values);

	hxarray<int, 16u> heap;
	size_t expected_removed = 0u;
	for(size_t index = 0; index < value_count; ++index) {
		if((values[index] & 1) == 0) {
			++expected_removed;
		}
		heap.push_heap(values[index]);
	}

	EXPECT_TRUE(hxarray_test_is_max_heap(heap));

	const size_t removed = heap.erase_if_heap([](int value) -> bool {
		return (value & 1) == 0;
	});
	EXPECT_EQ(removed, expected_removed);
	EXPECT_EQ(heap.size(), value_count - expected_removed);
	EXPECT_TRUE(hxarray_test_is_max_heap(heap));

	int previous = INT_MAX;
	while(!heap.empty()) {
		const int root = heap.front();
		EXPECT_EQ(root & 1, 1);
		EXPECT_LE(root, previous);
		previous = root;
		heap.pop_heap();
	}

	EXPECT_TRUE(heap.empty());
	EXPECT_EQ(heap.erase_if_heap([](int) { return false; }), 0u);
}

TEST(hxarray_test, make_heap_builds_max_heap) {
	static const int values[] = { 12, 3, 17, 8, 5, 14, 6 };
	hxarray<int, 7> heap;
	heap.assign(values, values + hxsize(values));

	heap.make_heap();
	EXPECT_TRUE(hxarray_test_is_max_heap(heap));
	EXPECT_EQ(heap.front(), 17);
}

TEST(hxarray_test, insertion_sort_orders_elements) {
	static const int unsorted[] = { 9, 2, 7, 4, 4, 1 };
	hxarray<int, 6> values(unsorted);

	values.insertion_sort();

	static const int expected[] = { 1, 2, 4, 4, 7, 9 };
	for(size_t index = 0; index < hxsize(expected); ++index) {
		EXPECT_EQ(values[index], expected[index]);
	}
}

TEST(hxarray_test, sort_orders_elements) {
	static const int unsorted[] = { 13, -5, 7, 0, 13, 2 };
	hxarray<int, 6> values(unsorted);

	values.sort();

	static const int expected[] = { -5, 0, 2, 7, 13, 13 };
	for(size_t index = 0; index < hxsize(expected); ++index) {
		EXPECT_EQ(values[index], expected[index]);
	}
}

TEST_F(hxarray_test_f, emplace_back) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		hxarray<hxtest_object> objs;
		objs.reserve(3u);

		const hxtest_object& default_inserted = objs.emplace_back();
		EXPECT_EQ(objs.data(), &default_inserted);
		EXPECT_EQ(default_inserted.id, -1);
		EXPECT_FALSE(default_inserted.moved_from);

		hxtest_object original(42);
		const hxtest_object& move_inserted = objs.emplace_back(hxmove(original));
		EXPECT_EQ(objs.data() + 1, &move_inserted);
		EXPECT_FALSE(move_inserted.moved_from);
		EXPECT_TRUE(original.moved_from);

		const hxtest_object& value_inserted = objs.emplace_back(77);
		EXPECT_EQ(objs.data() + 2, &value_inserted);
		EXPECT_EQ(value_inserted.id, 77);
		EXPECT_FALSE(value_inserted.moved_from);

		EXPECT_EQ(objs.size(), 3u);
		EXPECT_EQ(objs.back().id, 77);
	}

	EXPECT_TRUE(check_totals(4));
}

TEST(hxarray_test, for_each_invokes_functors) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	static const unsigned char nums[5] = { 91, 92, 93, 94, 95 };
	hxarray<int> objs(nums);

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
	struct hxarray_test_x_t { int n; hxarray_test_x_t() : n(0) { }; void operator()(int&) { ++n; } } x;
	objs.for_each(x);
	EXPECT_EQ(x.n, 5);

	// Run it empty for correctness.
	objs.clear();
	struct hxarray_test_y_t { void operator()(int&) const { hxassertmsg(0, "internal error"); } } y;
	objs.for_each(y);
}

TEST(hxarray_test, generate_n_appends_functor_results) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxarray<int> values;
	values.reserve(6u);

	values.push_back(42);

	int next_value = 5;
	int call_count = 0;
	values.generate_n(3u, [&]() -> int {
		++call_count;
		return next_value++;
	});

	EXPECT_EQ(call_count, 3);
	EXPECT_EQ(values.size(), 4u);
	EXPECT_EQ(values[0], 42);
	EXPECT_EQ(values[1], 5);
	EXPECT_EQ(values[2], 6);
	EXPECT_EQ(values[3], 7);

	values.generate_n(0u, []() -> int {
		hxassertmsg(0, "generate_n invoked for zero size");
		return 0;
	});
}

TEST(hxarray_test, all_of_any_of) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	static const unsigned char nums[5] = { 91, 92, 93, 94, 95 };
	hxarray<int> objs(nums);

	EXPECT_TRUE(objs.all_of([](const int& x) { return x > 0; }));
	EXPECT_FALSE(objs.all_of([](const int& x) { return x < 95; }));

	int all_calls = 0;
	EXPECT_FALSE(objs.all_of([&](const int& value) -> bool {
		++all_calls;
		return value < 93;
	}));
	EXPECT_EQ(all_calls, 3);

	int any_calls = 0;
	EXPECT_TRUE(objs.any_of([&](const int& value) -> bool {
		++any_calls;
		return value == 94;
	}));
	EXPECT_EQ(any_calls, 4);

	int miss_calls = 0;
	EXPECT_FALSE(objs.any_of([&](const int& value) -> bool {
		++miss_calls;
		return value == -1;
	}));
	EXPECT_EQ(miss_calls, 5);

	objs.clear();
	auto empty_predicate = [](const int&) -> bool {
		hxassertmsg(0, "internal error");
		return false;
	};
	EXPECT_TRUE(objs.all_of(empty_predicate));
	EXPECT_FALSE(objs.any_of(empty_predicate));
}

TEST(hxarray_test, binary_search) {
	static const int sorted_values[] = { 1, 3, 5, 7, 9 };
	hxarray<int, 5> values(sorted_values);

	const int* const_missing = ((const hxarray<int, 5>&)values).binary_search(4);
	EXPECT_EQ(const_missing, values.end());

	int* mutable_found = values.binary_search(7);
	EXPECT_EQ(mutable_found, values.begin() + 3);

	EXPECT_EQ(values.binary_search(2), values.end());
}

TEST(hxarray_test, find_returns_first_match) {
	static const int values_source[] = { 2, 4, 4, 8, 16 };
	hxarray<int, 5> values(values_source);

	const int* const_pos = ((const hxarray<int, 5>&)values).find(4);
	EXPECT_EQ(const_pos, values.begin() + 1);

	const int* const_missing = ((const hxarray<int, 5>&)values).find(32);
	EXPECT_EQ(const_missing, values.end());

	int* mutable_pos = values.find(8);
	EXPECT_EQ(mutable_pos, values.begin() + 3);

	const int* const_predicate = ((const hxarray<int, 5>&)values).find_if([](int value) {
		return value >= 8;
	});
	EXPECT_EQ(const_predicate, values.begin() + 3);

	const int* const_predicate_missing = ((const hxarray<int, 5>&)values).find_if([](int value) {
		return value < 0;
	});
	EXPECT_EQ(const_predicate_missing, values.end());

	int* mutable_predicate = values.find_if([](int& value) {
		return (value & 1) == 0 && value > 4;
	});
	EXPECT_EQ(mutable_predicate, values.begin() + 3);

#if HX_CPLUSPLUS >= 202002L
	if(int* t = values.find_if([](int& value) { return value == 8; }); t == values.end()) {
		ADD_FAILURE();
	}
#endif
}

TEST(hxarray_test, erase_if) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	static const int nums[5] = { 1, 2, 3, 4, 5 };
	hxarray<int, 5> objs(nums);

	int remove_calls = 0;
	auto remove_even = [&](int& value) -> bool {
		++remove_calls;
		return (value & 1) == 0;
	};
	EXPECT_EQ(objs.erase_if(remove_even), 2u);
	EXPECT_EQ(remove_calls, 5);

	static const int expected[] = { 1, 5, 3 };
	for(size_t i = 0; i < 3; ++i) {
		EXPECT_EQ(objs[i], expected[i]);
	}

	EXPECT_EQ(objs.erase_if([](int x) { return x == 1; }), 1u);
	EXPECT_EQ(objs.size(), 2u);

	objs.clear();
	auto empty_predicate = [](int&) -> bool {
		hxassertmsg(0, "internal error");
		return false;
	};
	EXPECT_EQ(objs.erase_if(empty_predicate), 0u);
}

TEST_F(hxarray_test_f, resizing) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		static const int32_t nums[5] = { 51, 52, 53, 54, 55 };

		hxarray<hxtest_object> objs(12);
		objs.reserve(10); // Reserve less than is being used.
		objs = nums;

		// Use the two-argument version to delete it.
		objs.resize(3, hxtest_object());

		EXPECT_EQ(objs.size(), 3u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);
		for(size_t i = 0u; i < objs.size(); ++i) {
			EXPECT_FALSE(objs[i].moved_from);
		}

		objs.resize(4u);

		EXPECT_EQ(objs.size(), 4u);
		EXPECT_EQ(objs[0].id, 51);
		EXPECT_EQ(objs[2].id, 53);
		EXPECT_EQ(objs[3].id, -14);
		EXPECT_EQ(objs.capacity(), 12u);
		for(size_t i = 0u; i < objs.size(); ++i) {
			EXPECT_FALSE(objs[i].moved_from);
		}

		objs.resize(10u);
		EXPECT_EQ(objs.size(), 10u);
		EXPECT_EQ(objs[9].id, -20);
		EXPECT_FALSE(objs[9].moved_from);

		EXPECT_FALSE(objs.empty());
		objs.clear();
		EXPECT_EQ(objs.size(), 0u);
		EXPECT_TRUE(objs.empty());

		EXPECT_EQ(objs.capacity(), 12u);
	}

	EXPECT_TRUE(check_totals(25));
}

TEST_F(hxarray_test_f, assignment) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		hxarray<hxtest_object> objs;
		objs.reserve(1);

		hxtest_object to;
		to.id = 67;
		objs.push_back(to);

		hxarray<hxtest_object> objs2;
		objs2 = objs; // Assign to the same type.

		hxarray<hxtest_object, 1> objs3;
		objs3 = objs; // Assign to a different type.

		hxarray<hxtest_object> objs4(objs); // Construct from the same type.
		hxarray<hxtest_object, 1> objs5(objs); // Construct from a different type.

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

#if HX_CPLUSPLUS >= 202002L
TEST_F(hxarray_test_f, assign_range_from_rvalue) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxtest_object source_elements[] = {
		hxtest_object(5),
		hxtest_object(9),
		hxtest_object(13)
	};
	const size_t source_count = hxsize(source_elements);

	hxarray<hxtest_object> elements;
	elements.reserve(source_count);
	elements.assign_range(hxarray_test_pointer_range(
		source_elements, source_elements + source_count));

	EXPECT_EQ(elements.size(), source_count);
	EXPECT_EQ(elements[0].id, 5);
	EXPECT_EQ(elements[1].id, 9);
	EXPECT_EQ(elements[2].id, 13);
	for(size_t i = 0u; i < source_count; ++i) {
		EXPECT_FALSE(elements[i].moved_from);
		EXPECT_TRUE(source_elements[i].moved_from);
	}
}

TEST_F(hxarray_test_f, assign_range_from_const) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	const int32_t assigned_element_ints[] = { 4, 7, 11, 18 };
	const hxarray<hxtest_object> assigned_elements = assigned_element_ints;
	const size_t assigned_count = hxsize(assigned_element_ints);

	hxarray<hxtest_object> elements;
	elements.reserve(assigned_count + 1u);
	elements.push_back(hxtest_object(91));

	hxarray_test_pointer_range<const hxtest_object> range(
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

TEST_F(hxarray_test_f, assign_range_from_mutable_range) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxtest_object source_elements[] = {
		hxtest_object(2),
		hxtest_object(3),
		hxtest_object(5)
	};
	const size_t source_count = hxsize(source_elements);

	hxarray<hxtest_object> elements;
	elements.reserve(source_count);

	hxarray_test_pointer_range<hxtest_object> range(
		source_elements, source_elements + source_count);
	elements.assign_range(range);

	EXPECT_EQ(elements.size(), source_count);
	EXPECT_EQ(elements[0].id, 2);
	EXPECT_EQ(elements[1].id, 3);
	EXPECT_EQ(elements[2].id, 5);
	for(size_t i = 0u; i < source_count; ++i) {
		EXPECT_FALSE(elements[i].moved_from);
		EXPECT_FALSE(source_elements[i].moved_from);
	}
}
#endif

TEST_F(hxarray_test_f, push_back_move_tracker) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxtest_object source(42);
	hxarray<hxtest_object> elements;
	elements.reserve(3u);
	elements.push_back(hxmove(source));

	EXPECT_EQ(elements.size(), 1u);
	EXPECT_EQ(elements[0].id, 42);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_TRUE(source.moved_from);

	// Confirm the array does not get moved.
	hxtest_object x(84);
	elements.push_back(x);

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[1].id, 84);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_FALSE(x.moved_from);
}

TEST_F(hxarray_test_f, plus_equals_move_tracker_element) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxtest_object source(5);
	hxarray<hxtest_object> elements;
	elements.reserve(3u);
	elements += hxmove(source);

	EXPECT_EQ(elements.size(), 1u);
	EXPECT_EQ(elements[0].id, 5);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_TRUE(source.moved_from);

	// Confirm the array does not get moved.
	const hxtest_object x(11);
	elements += x;

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[1].id, 11);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_FALSE(x.moved_from);
}

TEST_F(hxarray_test_f, plus_equals_move_tracker_array) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxtest_object initial(1);
	static const int32_t appended_values[] = { 3, 5, 7 };

	hxarray<hxtest_object> move_target;
	move_target.reserve(5u);
	move_target.push_back(initial);

	hxarray<hxtest_object> copy_target;
	copy_target.reserve(5u);
	copy_target.push_back(initial);

	EXPECT_FALSE(initial.moved_from);

	hxarray<hxtest_object> move_source;
	move_source = appended_values;

	hxarray<hxtest_object> copy_source;
	copy_source = appended_values;

	// Confirm that the array of temporaries actually gets moved.
	move_target += hxmove(move_source);

	EXPECT_EQ(move_target.size(), 4u);
	EXPECT_EQ(move_target[0].id, 1);
	EXPECT_EQ(move_target[1].id, 3);
	EXPECT_EQ(move_target[2].id, 5);
	EXPECT_EQ(move_target[3].id, 7);
	for(size_t i = 0u; i < move_target.size(); ++i) {
		EXPECT_FALSE(move_target[i].moved_from);
	}
	EXPECT_EQ(move_source.size(), 3u);
	for(size_t i = 0u; i < move_source.size(); ++i) {
		EXPECT_TRUE(move_source[i].moved_from);
	}

	// Confirm that the lvalue source does not get moved.
	copy_target += copy_source;

	EXPECT_EQ(copy_target.size(), 4u);
	EXPECT_EQ(copy_target[0].id, 1);
	EXPECT_EQ(copy_target[1].id, 3);
	EXPECT_EQ(copy_target[2].id, 5);
	EXPECT_EQ(copy_target[3].id, 7);
	for(size_t i = 0u; i < copy_target.size(); ++i) {
		EXPECT_FALSE(copy_target[i].moved_from);
	}
	EXPECT_EQ(copy_source.size(), 3u);
	for(size_t i = 0u; i < copy_source.size(); ++i) {
		EXPECT_FALSE(copy_source[i].moved_from);
	}
}

TEST_F(hxarray_test_f, insert_move_tracker_move_and_copy) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxarray<hxtest_object> elements;
	elements.reserve(4u);

	hxtest_object initial(10);
	elements.push_back(initial);
	EXPECT_FALSE(initial.moved_from);

	hxtest_object move_source(20);
	elements.insert(elements.begin(), hxmove(move_source));

	EXPECT_EQ(elements.size(), 2u);
	EXPECT_EQ(elements[0].id, 20);
	EXPECT_EQ(elements[1].id, 10);
	EXPECT_TRUE(move_source.moved_from);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(elements[1].moved_from);

	hxtest_object copy_source(30);
	elements.insert(2u, copy_source);

	EXPECT_EQ(elements.size(), 3u);
	EXPECT_EQ(elements[0].id, 20);
	EXPECT_EQ(elements[1].id, 10);
	EXPECT_EQ(elements[2].id, 30);
	EXPECT_FALSE(copy_source.moved_from);
	for(size_t i = 0u; i < elements.size(); ++i) {
		EXPECT_FALSE(elements[i].moved_from);
	}
}

TEST_F(hxarray_test_f, emplace_back_move_tracker_forwarding) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxarray<hxtest_object> elements;
	elements.reserve(3u);

	hxtest_object copy_source(40);
	hxtest_object move_source(50);

	const hxtest_object& copy_inserted = elements.emplace_back(copy_source);
	EXPECT_EQ(&copy_inserted, &elements[0]);
	EXPECT_EQ(elements[0].id, 40);
	EXPECT_FALSE(elements[0].moved_from);
	EXPECT_FALSE(copy_source.moved_from);

	const hxtest_object& move_inserted = elements.emplace_back(hxmove(move_source));
	EXPECT_EQ(&move_inserted, &elements[1]);
	EXPECT_EQ(elements[1].id, 50);
	EXPECT_FALSE(elements[1].moved_from);
	EXPECT_TRUE(move_source.moved_from);

	EXPECT_EQ(elements.size(), 2u);
}

TEST(hxarray_iterators, cbegin_cend) {
	hxarray<int, 4u> values;
	values.push_back(1);
	values.push_back(3);
	values.push_back(5);

	const hxarray<int, 4u>& const_values = values;
	const int expected[] = { 1, 3, 5 };
	const size_t expected_count = hxsize(expected);
	size_t index = 0u;

	for (hxarray<int, 4u>::const_iterator it = const_values.cbegin(); it != const_values.cend(); ++it) {
		ASSERT_LT(index, expected_count);
		EXPECT_EQ(*it, expected[hxmin<size_t>(index, 2u)]);
		++index;
	}

	EXPECT_EQ(index, const_values.size());
	EXPECT_EQ(const_values.cbegin(), const_values.begin());
	EXPECT_EQ(const_values.cend(), const_values.end());
	EXPECT_EQ(const_values.cbegin() + const_values.size(), const_values.cend());
}

// std::initializer_list is great for writing test code for hxarray. Not sure
// what else it is useful for besides test data.
#if HX_CPLUSPLUS >= 202002L && !HX_NO_LIBCXX
TEST_F(hxarray_test_f, plus_equals) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		hxarray<hxtest_object> objs;
		objs.reserve(10);

		objs += hxarray<hxtest_object>{ 1, 7, 11 };

		hxarray<hxtest_object> objs2 { 10, 70, 110 };
		objs += objs2;

		hxarray<hxtest_object> objs3 { 1, 7, 11, 10, 70, 110 };

		EXPECT_TRUE(hxkey_equal(objs, objs3));
		EXPECT_FALSE(hxkey_less(objs, objs3));

		// Compare unequal lengths with a temporary.
		hxtest_object t(440);
		objs += t;
		EXPECT_FALSE(hxkey_equal(objs, objs3));
		EXPECT_TRUE(hxkey_less(objs3, objs));

		// Compare equal lengths with a non-temporary.
		objs.resize(5);
		objs += hxtest_object(220);
		EXPECT_FALSE(hxkey_equal(objs, objs3));
		EXPECT_TRUE(hxkey_less(objs3, objs));
	}

	EXPECT_TRUE(check_totals(22));
}

TEST_F(hxarray_test_f, erase) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		hxarray<hxtest_object> objs { 1, 2, 3, 4, 5 };
		objs.erase(1);
		objs.erase(objs.begin() + 2);

		static const int expected_values[] = { 1, 3, 5 };
		hxarray<hxtest_object> expected(expected_values);
		EXPECT_TRUE(hxkey_equal(objs, expected));

		objs.erase(objs.begin());
		objs.erase(objs.end() - 1);

		EXPECT_TRUE(hxkey_equal(objs[0], 3));
		EXPECT_EQ(objs.size(), 1u);
	}

	EXPECT_TRUE(check_totals(8));
}

TEST_F(hxarray_test_f, insert) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		// The numeric constant zero is also a pointer. It seems more convenient
		// to allow both indices and pointers than to worry about it.
		hxarray<hxtest_object> objs; objs.reserve(5);
		objs.push_back(hxtest_object(3));
		objs.insert(objs.begin(), hxtest_object(1)); // Inserting at the beginning.
		objs.insert(2, hxtest_object(5)); // Inserting past the end.

		hxarray<hxtest_object> expected { 1, 3, 5 };
		EXPECT_TRUE(hxkey_equal(objs, expected));
		for(size_t i = 0u; i < objs.size(); ++i) {
			EXPECT_FALSE(objs[i].moved_from);
		}

		objs.insert(1, hxtest_object(2));
		objs.insert(3, hxtest_object(4));

		hxarray<hxtest_object> final_expected { 1, 2, 3, 4, 5 };
		EXPECT_TRUE(hxkey_equal(objs, final_expected));
		for(size_t i = 0u; i < objs.size(); ++i) {
			EXPECT_FALSE(objs[i].moved_from);
		}
	}

	EXPECT_TRUE(check_totals(18)); // <-- This is why we do not use insert.
}
#endif

TEST(hxarray_test, memcpy_clones_contents) {
	hxarray<unsigned char,5> source;
	source.resize(5u);
	unsigned char index = 0u;
	for(auto& it : source) { it = ++index; }

	hxarray<unsigned char,5> destination;
	destination.memcpy(source);

	EXPECT_EQ(destination.size(), source.size());
	while(index-- != 0u) {
		EXPECT_EQ(destination[index], source[index]);
	}
}

TEST(hxarray_test, memset_sets_bytes) {
	hxarray<unsigned char,6> bytes;
	bytes.resize(6u, 0u);
	bytes.memset(0xab);
	for(size_t index = 0u; index < bytes.size(); ++index) {
		EXPECT_EQ((int)bytes[index], 0xab);
	}
}

TEST(hxarray_test, c_strings) {
	hxarray<char, HX_MAX_LINE> z("prefix array 1");
	while(z[0] != 'a') {
		z.erase((size_t)0);
	}
	EXPECT_STREQ(z.data(), "array 1");

	z = "array 2";
	EXPECT_STREQ(z.data(), "array 2");
}

#if !HX_NO_LIBCXX
TEST(hxarray_test, initializer_list_brace_support) {
	const hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxarray<int, 2> x = { 2, 7 };
	EXPECT_EQ(x[1], 7);

	hxarray<int> y { 12, 17 };
	EXPECT_EQ(y[1], 17);
}
#endif

TEST(hxarray_test, swaps) {
	const hxsystem_allocator_scope allocator_scope(hxsystem_allocator_temporary_stack);

	hxarray<int> x(hxarray<int>({ 2, 7 }));
	hxarray<int> y = hxmove(x); // Should swap.
	hxarray<int> z;
	hxswap(y, z);
	EXPECT_TRUE(x.empty());
	EXPECT_TRUE(y.empty());
	EXPECT_EQ(z[0], 2);
	EXPECT_EQ(z[1], 7);
}
