// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxalgorithm.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

namespace {

// The hxhxalgorithm_test_ref_tracker_t tests check for correct use of references to temporaries.
class hxhxalgorithm_test_ref_tracker_t {
public:
	// This is not used by the sort code.
    explicit hxhxalgorithm_test_ref_tracker_t(int x) : value(x) { }

	// This is what is being used.

    hxhxalgorithm_test_ref_tracker_t(hxhxalgorithm_test_ref_tracker_t&& other) : value(other.value) {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
	}

	~hxhxalgorithm_test_ref_tracker_t() {
		::memset(&value, 0xefu, sizeof value);
	}

    hxhxalgorithm_test_ref_tracker_t& operator=(hxhxalgorithm_test_ref_tracker_t&& other) {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
		value = other.value;
		other.value = 0xefef;
        return *this;
    }

	// Called by hxkey_less below.
    bool operator<(const hxhxalgorithm_test_ref_tracker_t& other) const {
		// Technically legal but indicates an optimization issue.
		hxassert(this != &other);
        return value < other.value;
    }

    int value;

private:
	// This is what is not being used.

	hxhxalgorithm_test_ref_tracker_t() = delete;
    hxhxalgorithm_test_ref_tracker_t(const hxhxalgorithm_test_ref_tracker_t&) = delete;
	// Did you use hxmove?
    hxhxalgorithm_test_ref_tracker_t& operator=(const hxhxalgorithm_test_ref_tracker_t&) = delete;
    bool operator==(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
    bool operator!=(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
    bool operator>(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
    bool operator>=(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
    bool operator<=(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
	bool operator!(void) const = delete;
	operator bool(void) const = delete;
};

class hxhxalgorithm_test_iter_api_t {
public:
	explicit hxhxalgorithm_test_iter_api_t(hxhxalgorithm_test_ref_tracker_t* pointer) : m_pointer(pointer) { }
	hxhxalgorithm_test_iter_api_t(const hxhxalgorithm_test_iter_api_t& x) = default;
	hxhxalgorithm_test_iter_api_t& operator=(const hxhxalgorithm_test_iter_api_t& x) = default;

	// Require only the standard pointer operations. No array notation.
	hxhxalgorithm_test_ref_tracker_t& operator*(void) const { hxassert(m_pointer != hxnull); return *m_pointer; }

	hxhxalgorithm_test_iter_api_t& operator++(void) { hxassert(m_pointer != hxnull); ++m_pointer; return *this; }
	hxhxalgorithm_test_iter_api_t& operator--(void) { hxassert(m_pointer != hxnull); --m_pointer; return *this; }
	hxhxalgorithm_test_iter_api_t operator+(ptrdiff_t offset) const { hxassert(m_pointer != hxnull); return hxhxalgorithm_test_iter_api_t(m_pointer + offset); }
	hxhxalgorithm_test_iter_api_t operator-(ptrdiff_t offset) const { hxassert(m_pointer != hxnull); return hxhxalgorithm_test_iter_api_t(m_pointer - offset); }
	ptrdiff_t operator-(const hxhxalgorithm_test_iter_api_t& other) const { hxassert(m_pointer != hxnull); return m_pointer - other.m_pointer; }

	bool operator==(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer == other.m_pointer; }
	bool operator!=(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer != other.m_pointer; }
	bool operator<(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer < other.m_pointer; }
	bool operator>(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer > other.m_pointer; }
	bool operator<=(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer <= other.m_pointer; }
	bool operator>=(const hxhxalgorithm_test_iter_api_t& other) const { return m_pointer >= other.m_pointer; }
private:
	// This is what is not being used.

	// Not hxnull, hxnullptr. Return the "end" instead.
	hxhxalgorithm_test_iter_api_t(int null) = delete;
	hxhxalgorithm_test_iter_api_t(hxnullptr_t null) = delete;
	void operator[](int index) const = delete;

	// No post-increment or -decrement.
	hxhxalgorithm_test_iter_api_t operator++(int) = delete;
	hxhxalgorithm_test_iter_api_t operator--(int) = delete;

    hxhxalgorithm_test_ref_tracker_t& operator+=(const hxhxalgorithm_test_ref_tracker_t&) = delete;
    hxhxalgorithm_test_ref_tracker_t& operator-=(const hxhxalgorithm_test_ref_tracker_t&) = delete;
	bool operator&&(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
	bool operator||(const hxhxalgorithm_test_ref_tracker_t&) const = delete;
	bool operator!(void) const = delete;
	operator bool(void) const = delete;

	hxhxalgorithm_test_ref_tracker_t* m_pointer;
};

bool sort_iter_value_less(const hxhxalgorithm_test_ref_tracker_t& lhs, const hxhxalgorithm_test_ref_tracker_t& rhs) {
	return lhs.value < rhs.value;
}

bool sort_iter_value_greater(const hxhxalgorithm_test_ref_tracker_t& lhs, const hxhxalgorithm_test_ref_tracker_t& rhs) {
	return lhs.value > rhs.value;
}

} // namespace {

TEST(hxhxalgorithm_test, hxmerge_iterator_support) {
	hxhxalgorithm_test_ref_tracker_t left[3] = { hxhxalgorithm_test_ref_tracker_t(1), hxhxalgorithm_test_ref_tracker_t(3), hxhxalgorithm_test_ref_tracker_t(5) };
	hxhxalgorithm_test_ref_tracker_t right[3] = { hxhxalgorithm_test_ref_tracker_t(2), hxhxalgorithm_test_ref_tracker_t(4), hxhxalgorithm_test_ref_tracker_t(6) };
	hxhxalgorithm_test_ref_tracker_t dest[6] = {
		hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0),
		hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0)
	};

	hxmerge(hxhxalgorithm_test_iter_api_t(left), hxhxalgorithm_test_iter_api_t(left + 3),
		hxhxalgorithm_test_iter_api_t(right), hxhxalgorithm_test_iter_api_t(right + 3),
		hxhxalgorithm_test_iter_api_t(dest), sort_iter_value_less);

	const int expected_sorted[6] = { 1, 2, 3, 4, 5, 6 };
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest[i].value, expected_sorted[i]);
	}

	// Do it all over again with a GE functor and the parameters reversed.
	hxhxalgorithm_test_ref_tracker_t left_desc[3] = { hxhxalgorithm_test_ref_tracker_t(5), hxhxalgorithm_test_ref_tracker_t(3), hxhxalgorithm_test_ref_tracker_t(1) };
	hxhxalgorithm_test_ref_tracker_t right_desc[3] = { hxhxalgorithm_test_ref_tracker_t(6), hxhxalgorithm_test_ref_tracker_t(4), hxhxalgorithm_test_ref_tracker_t(2) };
	hxhxalgorithm_test_ref_tracker_t dest_desc[6] = {
		hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0),
		hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(0)
	};

	hxmerge(hxhxalgorithm_test_iter_api_t(left_desc), hxhxalgorithm_test_iter_api_t(left_desc + 3),
		hxhxalgorithm_test_iter_api_t(right_desc), hxhxalgorithm_test_iter_api_t(right_desc + 3),
		hxhxalgorithm_test_iter_api_t(dest_desc), sort_iter_value_greater);

	const int expected_desc[6] = { 6, 5, 4, 3, 2, 1 };
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest_desc[i].value, expected_desc[i]);
	}
}

TEST(hxhxalgorithm_test, hxbinary_search_iterator_support) {
	hxhxalgorithm_test_ref_tracker_t values[7] = {
		hxhxalgorithm_test_ref_tracker_t(-5), hxhxalgorithm_test_ref_tracker_t(-1), hxhxalgorithm_test_ref_tracker_t(0), hxhxalgorithm_test_ref_tracker_t(3),
		hxhxalgorithm_test_ref_tracker_t(5), hxhxalgorithm_test_ref_tracker_t(8), hxhxalgorithm_test_ref_tracker_t(12)
	};

	hxhxalgorithm_test_iter_api_t begin(values);
	hxhxalgorithm_test_iter_api_t end(values + 7);

	hxhxalgorithm_test_ref_tracker_t key_three(3);
	hxhxalgorithm_test_iter_api_t result = hxbinary_search(begin, end, key_three, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 3);

	hxhxalgorithm_test_ref_tracker_t key_high(12);
	result = hxbinary_search(begin, end, key_high, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 12);

	hxhxalgorithm_test_ref_tracker_t missing(7);
	result = hxbinary_search(begin, end, missing, sort_iter_value_less);
	EXPECT_EQ(result, end);

	// Empty list.
	result = hxbinary_search(begin, begin, key_three, sort_iter_value_less);
	EXPECT_EQ(result, begin);
}

TEST(hxhxalgorithm_test, sort_grinder) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(2);
	size_t max_size_mask = 0x7f;
	hxarray<hxhxalgorithm_test_ref_tracker_t> insertion_sorted; insertion_sorted.reserve(max_size_mask);
	hxarray<hxhxalgorithm_test_ref_tracker_t> heap_sorted; heap_sorted.reserve(max_size_mask);
	hxarray<hxhxalgorithm_test_ref_tracker_t> generic_sorted; generic_sorted.reserve(max_size_mask);

	for(int i=12; i--; ) {
		// Set up the arrays to be sorted.
		size_t size = (max_size_mask >> i) & rng;
		for(size_t j=size; j--;) {
			insertion_sorted.push_back(hxhxalgorithm_test_ref_tracker_t(rng.range(100, 200)));
			// Use the && constructor and not the const& one.
			heap_sorted.push_back(hxhxalgorithm_test_ref_tracker_t(0));
			generic_sorted.push_back(hxhxalgorithm_test_ref_tracker_t(0));
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

TEST(hxhxalgorithm_test, sort_grinder_generic) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(3);
	size_t max_size_mask = 0xffff;
	hxarray<hxhxalgorithm_test_ref_tracker_t> sorted; sorted.reserve(max_size_mask);
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
			sorted.push_back(hxhxalgorithm_test_ref_tracker_t(x));
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

// ==> TEST(hxhxalgorithm_test, sort_int_case).
namespace {

// Run some simple integer tests first.
bool sort_int(int a, int b) {
	return a < b;
}

bool sort_int_reverse(int a, int b) {
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

} // namespace {

TEST(hxhxalgorithm_test, sort_int_case) {
	// Instantiate and pass the sort templates as function pointers.
	do_sort_int_case(hxinsertion_sort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxheapsort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxsort<int*, bool (*)(int, int)>);
}

TEST(hxset_algorithms_test, int_pointer_ranges) {
	const int left[] = { 1, 3, 5, 7 };
	const int right[] = { 3, 4, 7, 9 };
	int dest_union[8] = { };
	int dest_intersection[4] = { };
	int dest_difference[4] = { };

	auto expect_range = [](const int* begin, const int* end, const int* expected) {
		for(const int* it = begin; it != end; ++it, ++expected) {
			EXPECT_EQ(*it, *expected);
		}
	};

	int* union_end = hxset_union(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_union+0);
	const int expected_union[] = { 1, 3, 4, 5, 7, 9 };
	EXPECT_EQ(union_end - dest_union, (ptrdiff_t)hxsize(expected_union));
	expect_range(dest_union, union_end, expected_union);

	int* intersection_end = hxset_intersection(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_intersection+0);
	const int expected_intersection[] = { 3, 7 };
	EXPECT_EQ(intersection_end - dest_intersection, (ptrdiff_t)hxsize(expected_intersection));
	expect_range(dest_intersection, intersection_end, expected_intersection);

	int* difference_end = hxset_difference(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_difference+0);
	const int expected_difference[] = { 1, 5 };
	EXPECT_EQ(difference_end - dest_difference, (ptrdiff_t)hxsize(expected_difference));
	expect_range(dest_difference, difference_end, expected_difference);
}

TEST(hxset_algorithms_test, hxarray_output_iterator_support) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	auto expect_hxarray = [](const hxarray<int>& actual, const int* expected, size_t count) {
		ASSERT_EQ(actual.size(), count);
		for(size_t i = 0; i < count; ++i) {
			EXPECT_EQ(actual[i], expected[i]);
		}
	};

	const int left[] = { 1, 2, 4 };
	const int right[] = { 2, 4, 5 };

	// hxmerge
	hxarray<int> merge_output;
	merge_output.reserve(hxsize(left) + hxsize(right) + 1u);
	merge_output.push_back(0);
	hxmerge<const int*, hxarray<int>&>(left+0, left + hxsize(left),
		right+0, right + hxsize(right),
		merge_output);
	const int expected_merge[] = { 0, 1, 2, 2, 4, 4, 5 };
	expect_hxarray(merge_output, expected_merge, hxsize(expected_merge));

	// hxset_union
	hxarray<int> union_output;
	union_output.reserve(hxsize(left) + hxsize(right) + 1u);
	union_output.push_back(0);
	hxset_union<const int*, hxarray<int>&>(left+0, left + hxsize(left),
		right+0, right + hxsize(right),
		union_output);
	const int expected_union[] = { 0, 1, 2, 4, 5 };
	expect_hxarray(union_output, expected_union, hxsize(expected_union));

	// hxset_intersection
	hxarray<int> intersection_output;
	intersection_output.reserve(hxsize(left) + 1u);
	intersection_output.push_back(0);
	hxset_intersection<const int*, hxarray<int>&>(left+0, left + hxsize(left),
		right+0, right + hxsize(right),
		intersection_output);
	const int expected_intersection[] = { 0, 2, 4 };
	expect_hxarray(intersection_output, expected_intersection, hxsize(expected_intersection));

	// hxset_difference
	hxarray<int> difference_output;
	difference_output.reserve(hxsize(left) + 1u);
	difference_output.push_back(0);
	hxset_difference<const int*, hxarray<int>&>(left+0, left + hxsize(left),
		right+0, right + hxsize(right),
		difference_output);
	const int expected_difference[] = { 0, 1 };
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

	hxmerge(left+0, left + left_count, right+0, right + right_count, dest+0);

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
	hxarray<hxhxalgorithm_test_ref_tracker_t> sorted; sorted.reserve(100);

	for(int i=100; i--; ) {
		int x = rng.range(0, 100);
		sorted.push_back(hxhxalgorithm_test_ref_tracker_t(x));
	}
	hxsort(sorted.begin(), sorted.end());

	for(size_t i=100u; i--; ) {
		hxhxalgorithm_test_ref_tracker_t t = hxmove(sorted[i]); // Don't pass an address that is in the array.
		hxhxalgorithm_test_ref_tracker_t* ptr = hxbinary_search(sorted.begin(), sorted.end(), t);
		// Assert logical equivalence without using ==. The hxhxalgorithm_test_ref_tracker_t* may point
		// elsewhere.
		EXPECT_TRUE(!(*ptr < t) && !(t < *ptr));
	}
}

// ==> TEST(hxhxalgorithm_test, iterator_support).
template<typename sort_callback_t>
static void do_sort_iter_case(const sort_callback_t& sort_callback) {
	const int initial_values[5] = { 2, 1, 0, 4, -5 };
	const int expected_two[5] = { 1, 2, 0, 4, -5 };
	const int expected_sorted[5] = { -5, 0, 1, 2, 4 };
	const int expected_descending[5] = { 4, 2, 1, 0, -5 };
	hxhxalgorithm_test_ref_tracker_t values[5] = {
		hxhxalgorithm_test_ref_tracker_t(initial_values[0]),
		hxhxalgorithm_test_ref_tracker_t(initial_values[1]),
		hxhxalgorithm_test_ref_tracker_t(initial_values[2]),
		hxhxalgorithm_test_ref_tracker_t(initial_values[3]),
		hxhxalgorithm_test_ref_tracker_t(initial_values[4])
	};

	auto reset = [&]() {
		for(size_t i = 0; i < 5; ++i) {
			values[i] = hxhxalgorithm_test_ref_tracker_t(initial_values[i]);
		}
	};

	auto expect_values = [&](const int (&expected)[5]) {
		for(size_t i = 0; i < 5; ++i) {
			EXPECT_EQ(values[i].value, expected[i]);
		}
	};

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values), sort_iter_value_less);
	expect_values(initial_values);

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 1), sort_iter_value_less);
	expect_values(initial_values);

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 2), sort_iter_value_less);
	expect_values(expected_two);

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_greater);
	expect_values(expected_descending);

	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);
}

TEST(hxhxalgorithm_test, iterator_support) {
	do_sort_iter_case([](hxhxalgorithm_test_iter_api_t begin, hxhxalgorithm_test_iter_api_t end, const auto& less) {
		hxinsertion_sort(begin, end, less);
	});

	do_sort_iter_case([](hxhxalgorithm_test_iter_api_t begin, hxhxalgorithm_test_iter_api_t end, const auto& less) {
		hxheapsort(begin, end, less);
	});

	do_sort_iter_case([](hxhxalgorithm_test_iter_api_t begin, hxhxalgorithm_test_iter_api_t end, const auto& less) {
		hxsort(begin, end, less);
	});
}
#endif // HX_CPLUSPLUS >= 201402L
