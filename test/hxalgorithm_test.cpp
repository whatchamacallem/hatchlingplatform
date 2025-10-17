// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxalgorithm.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// The hxsort_test_api_t tests check for correct use of references to temporaries.
class hxsort_test_api_t {
public:
	// This is not used by the sort code.
    explicit hxsort_test_api_t(int x) : value(x) { }

	// This is what is being used.

    hxsort_test_api_t(hxsort_test_api_t&& other) : value(other.value) {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
	}

	~hxsort_test_api_t() {
		::memset(&value, 0xefu, sizeof value);
	}

    hxsort_test_api_t& operator=(hxsort_test_api_t&& other) {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
		value = other.value;
        return *this;
    }

	// Called by hxkey_less below.
    bool operator<(const hxsort_test_api_t& other) const {
		// Technically legal but indicates an optimization issue.
		hxassert(this != &other);
        return value < other.value;
    }

    int value;

private:
	// This is what is not being used.

	hxsort_test_api_t() = delete;
    hxsort_test_api_t(const hxsort_test_api_t&) = delete;
	// Did you use hxmove?
    hxsort_test_api_t& operator=(const hxsort_test_api_t&) = delete;
    bool operator==(const hxsort_test_api_t&) const = delete;
    bool operator!=(const hxsort_test_api_t&) const = delete;
    bool operator>(const hxsort_test_api_t&) const = delete;
    bool operator>=(const hxsort_test_api_t&) const = delete;
    bool operator<=(const hxsort_test_api_t&) const = delete;
	bool operator!(void) const = delete;
	operator bool(void) const = delete;
};

class hxsort_test_iter_api_t {
public:
	explicit hxsort_test_iter_api_t(hxsort_test_api_t* pointer) : m_pointer(pointer) { }
	hxsort_test_iter_api_t(const hxsort_test_iter_api_t& x) = default;
	hxsort_test_iter_api_t& operator=(const hxsort_test_iter_api_t& x) = default;

	// Require only the standard pointer operations. No array notation.
	hxsort_test_api_t& operator*(void) const { hxassert(m_pointer != hxnull); return *m_pointer; }

	hxsort_test_iter_api_t& operator++(void) { hxassert(m_pointer != hxnull); ++m_pointer; return *this; }
	hxsort_test_iter_api_t operator++(int) { hxassert(m_pointer != hxnull); hxsort_test_iter_api_t temp(*this); ++m_pointer; return temp; }
	hxsort_test_iter_api_t& operator--(void) { hxassert(m_pointer != hxnull); --m_pointer; return *this; }
	hxsort_test_iter_api_t operator+(ptrdiff_t offset) const { hxassert(m_pointer != hxnull); return hxsort_test_iter_api_t(m_pointer + offset); }
	hxsort_test_iter_api_t operator-(ptrdiff_t offset) const { hxassert(m_pointer != hxnull); return hxsort_test_iter_api_t(m_pointer - offset); }
	ptrdiff_t operator-(const hxsort_test_iter_api_t& other) const { hxassert(m_pointer != hxnull); return m_pointer - other.m_pointer; }

	bool operator==(const hxsort_test_iter_api_t& other) const { return m_pointer == other.m_pointer; }
	bool operator!=(const hxsort_test_iter_api_t& other) const { return m_pointer != other.m_pointer; }
	bool operator<(const hxsort_test_iter_api_t& other) const { return m_pointer < other.m_pointer; }
	bool operator>(const hxsort_test_iter_api_t& other) const { return m_pointer > other.m_pointer; }
	bool operator<=(const hxsort_test_iter_api_t& other) const { return m_pointer <= other.m_pointer; }
	bool operator>=(const hxsort_test_iter_api_t& other) const { return m_pointer >= other.m_pointer; }
private:
	// This is what is not being used.

	// Not hxnull, hxnullptr. Return the "end" instead.
	hxsort_test_iter_api_t(int null) = delete;
	hxsort_test_iter_api_t(hxnullptr_t null) = delete;
	void operator[](int index) const = delete;

    hxsort_test_api_t& operator+=(const hxsort_test_api_t&) = delete;
    hxsort_test_api_t& operator-=(const hxsort_test_api_t&) = delete;
	bool operator&&(const hxsort_test_api_t&) const = delete;
	bool operator||(const hxsort_test_api_t&) const = delete;
	bool operator!(void) const = delete;
	operator bool(void) const = delete;

	hxsort_test_api_t* m_pointer;
};

static bool sort_iter_value_less(const hxsort_test_api_t& lhs, const hxsort_test_api_t& rhs) {
	return lhs.value < rhs.value;
}

static bool sort_iter_value_greater(const hxsort_test_api_t& lhs, const hxsort_test_api_t& rhs) {
	return lhs.value > rhs.value;
}

TEST(hxsort_test, hxmerge_iterator_support) {
	hxsort_test_api_t left[3] = { hxsort_test_api_t(1), hxsort_test_api_t(3), hxsort_test_api_t(5) };
	hxsort_test_api_t right[3] = { hxsort_test_api_t(2), hxsort_test_api_t(4), hxsort_test_api_t(6) };
	hxsort_test_api_t dest[6] = {
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0),
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0)
	};

	hxmerge(hxsort_test_iter_api_t(left), hxsort_test_iter_api_t(left + 3),
		hxsort_test_iter_api_t(right), hxsort_test_iter_api_t(right + 3),
		hxsort_test_iter_api_t(dest), sort_iter_value_less);

	const int expected_sorted[6] = { 1, 2, 3, 4, 5, 6 };
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest[i].value, expected_sorted[i]);
	}

	// Do it all over again with a GE functor and the parameters reversed.
	hxsort_test_api_t left_desc[3] = { hxsort_test_api_t(5), hxsort_test_api_t(3), hxsort_test_api_t(1) };
	hxsort_test_api_t right_desc[3] = { hxsort_test_api_t(6), hxsort_test_api_t(4), hxsort_test_api_t(2) };
	hxsort_test_api_t dest_desc[6] = {
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0),
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0)
	};

	hxmerge(hxsort_test_iter_api_t(left_desc), hxsort_test_iter_api_t(left_desc + 3),
		hxsort_test_iter_api_t(right_desc), hxsort_test_iter_api_t(right_desc + 3),
		hxsort_test_iter_api_t(dest_desc), sort_iter_value_greater);

	const int expected_desc[6] = { 6, 5, 4, 3, 2, 1 };
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest_desc[i].value, expected_desc[i]);
	}
}

TEST(hxsort_test, hxbinary_search_iterator_support) {
	hxsort_test_api_t values[7] = {
		hxsort_test_api_t(-5), hxsort_test_api_t(-1), hxsort_test_api_t(0), hxsort_test_api_t(3),
		hxsort_test_api_t(5), hxsort_test_api_t(8), hxsort_test_api_t(12)
	};

	hxsort_test_iter_api_t begin(values);
	hxsort_test_iter_api_t end(values + 7);

	hxsort_test_api_t key_three(3);
	hxsort_test_iter_api_t result = hxbinary_search(begin, end, key_three, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 3);

	hxsort_test_api_t key_high(12);
	result = hxbinary_search(begin, end, key_high, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 12);

	hxsort_test_api_t missing(7);
	result = hxbinary_search(begin, end, missing, sort_iter_value_less);
	EXPECT_EQ(result, end);

	// Empty list.
	result = hxbinary_search(begin, begin, key_three, sort_iter_value_less);
	EXPECT_EQ(result, begin);
}

TEST(hxsort_test, sort_grinder) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(2);
	size_t max_size_mask = 0x7f;
	hxarray<hxsort_test_api_t> insertion_sorted; insertion_sorted.reserve(max_size_mask);
	hxarray<hxsort_test_api_t> heap_sorted; heap_sorted.reserve(max_size_mask);
	hxarray<hxsort_test_api_t> generic_sorted; generic_sorted.reserve(max_size_mask);

	for(int i=12; i--; ) {
		// Set up the arrays to be sorted.
		size_t size = (max_size_mask >> i) & rng;
		for(size_t j=size; j--;) {
			insertion_sorted.push_back(hxsort_test_api_t(rng.range(100, 200)));
			// Use the && constructor and not the const& one.
			heap_sorted.push_back(hxsort_test_api_t(0));
			generic_sorted.push_back(hxsort_test_api_t(0));
		}

		::memcpy((void*)heap_sorted.data(), insertion_sorted.data(), insertion_sorted.size_bytes());
		::memcpy((void*)generic_sorted.data(), insertion_sorted.data(), insertion_sorted.size_bytes());

		hxinsertion_sort(insertion_sorted.begin(), insertion_sorted.end());
		hxheapsort(heap_sorted.begin(), heap_sorted.end());
		hxsort(generic_sorted.begin(), generic_sorted.end());

		// Compare the three results to confirm they are sorted.
		ASSERT_EQ(::memcmp(insertion_sorted.data(), heap_sorted.data(), insertion_sorted.size_bytes()), 0);
		ASSERT_EQ(::memcmp(insertion_sorted.data(), generic_sorted.data(), insertion_sorted.size_bytes()), 0);

		insertion_sorted.clear();
		heap_sorted.clear();
		generic_sorted.clear();
	}
}

TEST(hxsort_test, sort_grinder_generic) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(3);
	size_t max_size_mask = 0xffff;
	hxarray<hxsort_test_api_t> sorted; sorted.reserve(max_size_mask);
	hxarray<int> histogram(20000, 0);

	for(int i=10; i--; ) {
		// Pick random values of increasing maximum value up to 2^16 and keep a
		// count of them.
		size_t size = (max_size_mask >> i) & rng;
		if(size <= 16) {
			continue;
		}
		for(size_t j=size; j--;) {
			int x = rng.range(10000, 10000);
			sorted.push_back(hxsort_test_api_t(x));
			++histogram[(size_t)x];
		}

		hxsort(sorted.begin(), sorted.end());

		// Check that all values are accounted for starting with the last one.
		// Confirm sort order with (j <= j+1) while walking down to the first
		// value. Note size > 16.
		--histogram[(size_t)sorted[size - 1].value];
		for(size_t j=size - 1u; j--;) {
			--histogram[(size_t)sorted[j].value];
			EXPECT_FALSE(hxkey_less(*sorted.get(j + 1), *sorted.get(j)));
		}
		for(size_t j=20000u; j-- > 10000u;) {
			EXPECT_EQ(histogram[j], 0);
		}
		sorted.clear();
	}
}

#if HX_CPLUSPLUS >= 201402L // C++14 only.

// Run some simple integer tests first.
static bool sort_int(int a, int b) {
	return a < b;
}

static bool sort_int_reverse(int a, int b) {
	return a > b;
}

template<typename sort_callback_t>
void do_sort_int_case(const sort_callback_t& sort_callback) {
	int ints[5] = { 2, 1, 0, 4, -5 };

	// Sort 0 elements.
	sort_callback(ints, ints, sort_int);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Nothing changed.

	// Sort 1 element.
	sort_callback(ints, ints + 1, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Still nothing changed.

	// Sort 2 elements.
	sort_callback(ints, ints + 2, sort_int);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// Sort all elements.
	sort_callback(ints, ints + 5, sort_int);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// Sort in reverse order.
	sort_callback(ints, ints + 5, sort_int_reverse);
	const int ints4[5] = { 4, 2, 1, 0, -5 };
	EXPECT_TRUE(::memcmp(ints, ints4, sizeof ints) == 0);

	// Sort the reversed array back into ascending order.
	sort_callback(ints, ints + 5, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);
}

template<typename sort_callback_t>
static void do_sort_iter_case(const sort_callback_t& sort_callback) {
	const int initial_values[5] = { 2, 1, 0, 4, -5 };
	const int expected_two[5] = { 1, 2, 0, 4, -5 };
	const int expected_sorted[5] = { -5, 0, 1, 2, 4 };
	const int expected_descending[5] = { 4, 2, 1, 0, -5 };
	hxsort_test_api_t values[5] = {
		hxsort_test_api_t(initial_values[0]),
		hxsort_test_api_t(initial_values[1]),
		hxsort_test_api_t(initial_values[2]),
		hxsort_test_api_t(initial_values[3]),
		hxsort_test_api_t(initial_values[4])
	};

	auto reset = [&]() {
		for(size_t i = 0; i < 5; ++i) {
			values[i] = hxsort_test_api_t(initial_values[i]);
		}
	};

	auto expect_values = [&](const int (&expected)[5]) {
		for(size_t i = 0; i < 5; ++i) {
			EXPECT_EQ(values[i].value, expected[i]);
		}
	};

	reset();
	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values), sort_iter_value_less);
	expect_values(initial_values);

	reset();
	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values + 1), sort_iter_value_less);
	expect_values(initial_values);

	reset();
	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values + 2), sort_iter_value_less);
	expect_values(expected_two);

	reset();
	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);

	reset();
	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values + 5), sort_iter_value_greater);
	expect_values(expected_descending);

	sort_callback(hxsort_test_iter_api_t(values), hxsort_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);
}

TEST(hxsort_test, sort_int_case) {
	// Instantiate and pass the sort templates as function pointers.
	do_sort_int_case(hxinsertion_sort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxheapsort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxsort<int*, bool (*)(int, int)>);
}

TEST(hxset_algorithms_test, int_pointer_ranges) {
	int left[] = { 1, 3, 5, 7 };
	int right[] = { 3, 4, 7, 9 };
	int dest_union[8] = { };
	int dest_intersection[4] = { };
	int dest_difference[4] = { };

	auto expect_range = [](const int* begin, const int* end, const int* expected) {
		for(const int* it = begin; it != end; ++it, ++expected) {
			EXPECT_EQ(*it, *expected);
		}
	};

	int* union_end = hxset_union(left, left + hxsize(left), right, right + hxsize(right), dest_union);
	const int expected_union[] = { 1, 3, 4, 5, 7, 9 };
	EXPECT_EQ(union_end - dest_union, (ptrdiff_t)hxsize(expected_union));
	expect_range(dest_union, union_end, expected_union);

	int* intersection_end = hxset_intersection(left, left + hxsize(left), right, right + hxsize(right), dest_intersection);
	const int expected_intersection[] = { 3, 7 };
	EXPECT_EQ(intersection_end - dest_intersection, (ptrdiff_t)hxsize(expected_intersection));
	expect_range(dest_intersection, intersection_end, expected_intersection);

	int* difference_end = hxset_difference(left, left + hxsize(left), right, right + hxsize(right), dest_difference);
	const int expected_difference[] = { 1, 5 };
	EXPECT_EQ(difference_end - dest_difference, (ptrdiff_t)hxsize(expected_difference));
	expect_range(dest_difference, difference_end, expected_difference);
}

TEST(hxset_algorithms_test, hxarray_output_iterator_support) {
	auto expect_hxarray = [](const hxarray<int>& actual, const int* expected, size_t count) {
		ASSERT_EQ(actual.size(), count);
		for(size_t i = 0; i < count; ++i) {
			EXPECT_EQ(actual[i], expected[i]);
		}
	};

	int merge_left[] = { 1, 3, 5 };
	int merge_right[] = { 2, 4, 6 };
	hxarray<int> merge_output;
	merge_output.reserve(hxsize(merge_left) + hxsize(merge_right) + 1u);
	merge_output.push_back(0);

	hxmerge(merge_left, merge_left + hxsize(merge_left),
		merge_right, merge_right + hxsize(merge_right),
		merge_output);
	merge_output.pop_back();

	const int expected_merge[] = { 1, 2, 3, 4, 5, 6 };
	expect_hxarray(merge_output, expected_merge, hxsize(expected_merge));

	int union_left[] = { 1, 3, 5, 7 };
	int union_right[] = { 3, 4, 7, 9 };
	hxarray<int> union_output;
	union_output.reserve(hxsize(union_left) + hxsize(union_right) + 1u);
	union_output.push_back(0);

	hxset_union(union_left, union_left + hxsize(union_left),
		union_right, union_right + hxsize(union_right),
		union_output);
	union_output.pop_back();

	const int expected_union[] = { 1, 3, 4, 5, 7, 9 };
	expect_hxarray(union_output, expected_union, hxsize(expected_union));

	int intersection_left[] = { 1, 3, 5, 7 };
	int intersection_right[] = { 3, 4, 7, 9 };
	hxarray<int> intersection_output;
	intersection_output.reserve(hxsize(intersection_left) + 1u);
	intersection_output.push_back(0);

	hxset_intersection(intersection_left, intersection_left + hxsize(intersection_left),
		intersection_right, intersection_right + hxsize(intersection_right),
		intersection_output);
	intersection_output.pop_back();

	const int expected_intersection[] = { 3, 7 };
	expect_hxarray(intersection_output, expected_intersection, hxsize(expected_intersection));

	int difference_left[] = { 1, 3, 5, 7 };
	int difference_right[] = { 3, 4, 7, 9 };
	hxarray<int> difference_output;
	difference_output.reserve(hxsize(difference_left) + 1u);
	difference_output.push_back(0);

	hxset_difference(difference_left, difference_left + hxsize(difference_left),
		difference_right, difference_right + hxsize(difference_right),
		difference_output);
	difference_output.pop_back();

	const int expected_difference[] = { 1, 5 };
	expect_hxarray(difference_output, expected_difference, hxsize(expected_difference));
}

TEST(hxmerge_test, preserves_stable_ordering) {
	struct hxmerge_test_record_t {
		int key;
		int ticket;

		bool operator<(const hxmerge_test_record_t& other) const {
			return key < other.key;
		}
	};

	hxmerge_test_record_t left[] = {
		{ 1, 0 }, { 3, 0 }, { 5, 0 }, { 5, 1 }
	};
	hxmerge_test_record_t right[] = {
		{ 1, 1 }, { 3, 1 }, { 5, 2 }, { 7, 0 }
	};
	hxmerge_test_record_t dest[8] = { };

	const size_t left_count = hxsize(left);
	const size_t right_count = hxsize(right);

	hxmerge(left, left + left_count, right, right + right_count, dest);

	const hxmerge_test_record_t expected[] = {
		{ 1, 0 }, { 1, 1 }, { 3, 0 }, { 3, 1 },
		{ 5, 0 }, { 5, 1 }, { 5, 2 }, { 7, 0 }
	};
	for(size_t i = 0; i < left_count + right_count; ++i) {
		EXPECT_EQ(dest[i].key, expected[i].key);
		EXPECT_EQ(dest[i].ticket, expected[i].ticket);
	}
}

TEST(hxbinary_search_test, simple_case) {
	int ints[5] = { 2, 5, 6, 88, 99 };
	int* ints_end = ints+5;

	// hxbinary_search returns end when not found.
	int* result = hxbinary_search(ints, ints+5, 88, sort_int);
	EXPECT_TRUE(result != ints_end && *result == 88);
	const int* cresult = hxbinary_search((const int*)ints, (const int*)ints+5, 2, sort_int);
	EXPECT_TRUE(cresult != ints_end && *cresult == 2);
	cresult = hxbinary_search((const int*)ints, (const int*)ints+5, 99);
	EXPECT_TRUE(cresult != ints_end && *cresult == 99);

	result = hxbinary_search(ints, ints+5, 0);
	EXPECT_TRUE(result == ints_end);
	result = hxbinary_search(ints, ints+5, 100);
	EXPECT_TRUE(result == ints_end);
	result = hxbinary_search(ints, ints+5, 7);
	EXPECT_TRUE(result == ints_end);

	// Empty range returns end.
	result = hxbinary_search(ints, ints, 11, sort_int); // Zero size.
	EXPECT_TRUE(result == ints);
}

TEST(hxbinary_search_test, binary_search_grinder) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(4);
	hxarray<hxsort_test_api_t> sorted; sorted.reserve(100);

	for(int i=100; i--; ) {
		int x = rng.range(0, 100);
		sorted.push_back(hxsort_test_api_t(x));
	}
	hxsort(sorted.begin(), sorted.end());

	for(size_t i=100u; i--; ) {
		hxsort_test_api_t t = hxmove(sorted[i]); // Don't pass an address that is in the array.
		hxsort_test_api_t* ptr = hxbinary_search(sorted.begin(), sorted.end(), t);
		// Assert logical equivalence without using ==. The hxsort_test_api_t* may point
		// elsewhere.
		EXPECT_TRUE(!(*ptr < t) && !(t < *ptr));
	}
}

TEST(hxsort_test, iterator_support) {
	do_sort_iter_case([](hxsort_test_iter_api_t begin, hxsort_test_iter_api_t end, const auto& less) {
		hxinsertion_sort(begin, end, less);
	});

	do_sort_iter_case([](hxsort_test_iter_api_t begin, hxsort_test_iter_api_t end, const auto& less) {
		hxheapsort(begin, end, less);
	});

	do_sort_iter_case([](hxsort_test_iter_api_t begin, hxsort_test_iter_api_t end, const auto& less) {
		hxsort(begin, end, less);
	});
}
#endif // HX_CPLUSPLUS >= 201402L
