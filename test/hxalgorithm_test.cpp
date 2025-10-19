// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxalgorithm.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

namespace {

// Checks API use and correct use of references to temporaries.
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
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	tracker_t left[3] = { tracker_t(1), tracker_t(3), tracker_t(5) };
	tracker_t right[3] = { tracker_t(2), tracker_t(4), tracker_t(6) };
	tracker_t dest[6] = {
		tracker_t(0), tracker_t(0), tracker_t(0),
		tracker_t(0), tracker_t(0), tracker_t(0)
	};

	// "Performs a stable merge sort of two ordered ranges [begin0, end0) and"
	// "[begin1, end1) -> output."
	//   /-\ ascending merge: { 1, 3, 5 } + { 2, 4, 6 } => { 1, 2, 3, 4, 5, 6 }
	hxmerge(hxhxalgorithm_test_iter_api_t(left), hxhxalgorithm_test_iter_api_t(left + 3),
		hxhxalgorithm_test_iter_api_t(right), hxhxalgorithm_test_iter_api_t(right + 3),
		hxhxalgorithm_test_iter_api_t(dest), sort_iter_value_less);

	const int expected_sorted[6] = { 1, 2, 3, 4, 5, 6 };
	// Confirm merged buffer now tracks { 1, 2, 3, 4, 5, 6 } without disturbing tickets.
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest[i].value, expected_sorted[i]);
	}

	// Do it all over again with a GE functor and the parameters reversed.
	tracker_t left_desc[3] = { tracker_t(5), tracker_t(3), tracker_t(1) };
	tracker_t right_desc[3] = { tracker_t(6), tracker_t(4), tracker_t(2) };
	tracker_t dest_desc[6] = {
		tracker_t(0), tracker_t(0), tracker_t(0),
		tracker_t(0), tracker_t(0), tracker_t(0)
	};

	// "Assumes both [begin0, end0) and [begin1, end1) are ordered by the less functor."
	//   \-/ descending merge inputs { 5, 3, 1 } & { 6, 4, 2 } with greater-than
	//   ensure stable output { 6, 5, 4, 3, 2, 1 }
	hxmerge(hxhxalgorithm_test_iter_api_t(left_desc), hxhxalgorithm_test_iter_api_t(left_desc + 3),
		hxhxalgorithm_test_iter_api_t(right_desc), hxhxalgorithm_test_iter_api_t(right_desc + 3),
		hxhxalgorithm_test_iter_api_t(dest_desc), sort_iter_value_greater);

	const int expected_desc[6] = { 6, 5, 4, 3, 2, 1 };
	// Ensure reverse-ordered comparison places tickets into { 6, 5, 4, 3, 2, 1 } without swaps.
	for(size_t i = 0; i < 6; ++i) {
		EXPECT_EQ(dest_desc[i].value, expected_desc[i]);
	}
}

TEST(hxhxalgorithm_test, hxbinary_search_iterator_support) {
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	tracker_t values[7] = {
		tracker_t(-5), tracker_t(-1), tracker_t(0), tracker_t(3),
		tracker_t(5), tracker_t(8), tracker_t(12)
	};

	hxhxalgorithm_test_iter_api_t begin(values);
	hxhxalgorithm_test_iter_api_t end(values + 7);

	// "Performs a binary search in the range [first, last)." Confirm hits stay
	// in-bounds for { 3, 12 } without aliasing iterators.
	tracker_t key_three(3);
	hxhxalgorithm_test_iter_api_t result = hxbinary_search(begin, end, key_three, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 3);

	tracker_t key_high(12);
	result = hxbinary_search(begin, end, key_high, sort_iter_value_less);
	EXPECT_NE(result, end);
	EXPECT_EQ((*result).value, 12);

	// "Returns end if the value is not found." Validate misses { 7 } including
	// the degenerate case -> empty range.
	tracker_t missing(7);
	result = hxbinary_search(begin, end, missing, sort_iter_value_less);
	EXPECT_EQ(result, end);

	// Empty list.
	result = hxbinary_search(begin, begin, key_three, sort_iter_value_less);
	EXPECT_EQ(result, begin);
}

TEST(hxhxalgorithm_test, sort_grinder) {
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(2);
	size_t max_size_mask = 0x7f;
	hxarray<tracker_t> insertion_sorted; insertion_sorted.reserve(max_size_mask);
	hxarray<tracker_t> heap_sorted; heap_sorted.reserve(max_size_mask);
	hxarray<tracker_t> generic_sorted; generic_sorted.reserve(max_size_mask);

	for(int i=12; i-- != 0; ) {
		// Set up the arrays to be sorted.
		size_t size = (max_size_mask >> i) & rng;
		for(size_t j=size; j-- != 0u;) {
			insertion_sorted.push_back(tracker_t(rng.range(100, 200)));
			// Use the && constructor and not the const& one.
			heap_sorted.push_back(tracker_t(0));
			generic_sorted.push_back(tracker_t(0));
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
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(3);
	size_t max_size_mask = 0xffff;
	hxarray<tracker_t> sorted; sorted.reserve(max_size_mask);
	hxarray<int> histogram(20000, 0);

	for(int i=10; i-- != 0; ) {
		// Pick random values of increasing maximum value up to 2^16 and keep a
		// count of them.
		size_t size = (max_size_mask >> i) & rng;
		if(size <= 16) {
			continue;
		}
		for(size_t j=size; j-- != 0u;) {
			int x = rng.range(10000, 10000);
			sorted.push_back(tracker_t(x));
			++histogram[(size_t)x];
		}

		hxsort(sorted.begin(), sorted.end());

		// Check that all values are accounted for starting with the last one.
		// Confirm sort order with (j <= j+1) while walking down to the first
		// value. Note size > 16.
		--histogram[(size_t)sorted[size - 1].value];
		for(size_t j=size - 1u; j-- != 0u;) {
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

	// "The end parameter points just past the end of the array." Confirm size 0
	// and size 1 ranges keep { 2, 1, 0, 4, -5 } untouched.
	sort_callback(ints, ints, sort_int);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Nothing changed.

	// Sort 1 element.
	sort_callback(ints, ints + 1, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Still nothing changed.

	// "Sorts the elements in the range [begin, end) in comparison order using the insertion sort algorithm."
	// Expect head slice -> { 1, 2 } while tail remains { 0, 4, -5 }.
	sort_callback(ints, ints + 2, sort_int);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// "This version is intended for sorting large numbers of small objects."
	// Whole array ascends into { -5, 0, 1, 2, 4 }. Sorts all elements.
	sort_callback(ints, ints + 5, sort_int);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// Ensure reversed comparator yields { 4, 2, 1, 0, -5 } before restoring order.
	sort_callback(ints, ints + 5, sort_int_reverse);
	const int ints4[5] = { 4, 2, 1, 0, -5 };
	EXPECT_TRUE(::memcmp(ints, ints4, sizeof ints) == 0);

	// Run one more ascending pass to confirm stability: { -5, 0, 1, 2, 4 }.
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

	// "Forms the union of two ordered ranges [begin0, end0) and [begin1, end1) into output."
	//   [1 3 5 7] | [3 4 7 9] => { 1, 3, 4, 5, 7, 9 }
	int* union_end = hxset_union(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_union+0);
	const int expected_union[] = { 1, 3, 4, 5, 7, 9 };
	EXPECT_EQ(union_end - dest_union, (ptrdiff_t)hxsize(expected_union));
	expect_range(dest_union, union_end, expected_union);

	// "Only keys present in both ranges appear in the output."
	//   [1 3 5 7] & [3 4 7 9] => { 3, 7 }
	int* intersection_end = hxset_intersection(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_intersection+0);
	const int expected_intersection[] = { 3, 7 };
	EXPECT_EQ(intersection_end - dest_intersection, (ptrdiff_t)hxsize(expected_intersection));
	expect_range(dest_intersection, intersection_end, expected_intersection);

	// "The output contains keys that appear in the first range but not the second."
	//   [1 3 5 7] - [3 4 7 9] => { 1, 5 }
	int* difference_end = hxset_difference(left+0, left + hxsize(left), right+0, right + hxsize(right), dest_difference+0);
	const int expected_difference[] = { 1, 5 };
	EXPECT_EQ(difference_end - dest_difference, (ptrdiff_t)hxsize(expected_difference));
	expect_range(dest_difference, difference_end, expected_difference);
}

TEST(hxset_algorithms_test, hxarray_output_iterator_support) {
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	auto expect_hxarray = [](const hxarray<tracker_t>& actual, const int* expected, size_t count) {
		ASSERT_EQ(actual.size(), count);
		for(size_t i = 0; i < count; ++i) {
			EXPECT_EQ(actual[i].value, expected[i]);
		}
	};

	tracker_t left[] = { tracker_t(1), tracker_t(2), tracker_t(4) };
	tracker_t right[] = { tracker_t(2), tracker_t(4), tracker_t(5) };

	// hxmerge
	hxarray<tracker_t> merge_output;
	merge_output.reserve(hxsize(left) + hxsize(right) + 1u);
	merge_output.push_back(tracker_t(0));
	// "Passing a hxarray as an output iterator ... will append to the array."
	//   seed { 0 } then merge -> { 0, 1, 2, 2, 4, 4, 5 }
	hxmerge(left+0, left + hxsize(left),
		right+0, right + hxsize(right), merge_output);
	const int expected_merge[] = { 0, 1, 2, 2, 4, 4, 5 };
	expect_hxarray(merge_output, expected_merge, hxsize(expected_merge));

	// hxset_union
	hxarray<tracker_t> union_output;
	union_output.reserve(hxsize(left) + hxsize(right) + 1u);
	union_output.push_back(tracker_t(0));
	// "Passing a hxarray as an output iterator ... will append to the array."
	//   union payload extends { 0 } => { 0, 1, 2, 4, 5 }
	hxset_union(left+0, left + hxsize(left),
		right+0, right + hxsize(right), union_output);
	const int expected_union[] = { 0, 1, 2, 4, 5 };
	expect_hxarray(union_output, expected_union, hxsize(expected_union));

	// hxset_intersection
	hxarray<tracker_t> intersection_output;
	intersection_output.reserve(hxsize(left) + 1u);
	intersection_output.push_back(tracker_t(0));
	// "Only keys present in both ranges appear in the output."
	//   sentinel { 0 } + overlap { 2, 4 } => { 0, 2, 4 }
	hxset_intersection(left+0, left + hxsize(left),
		right+0, right + hxsize(right), intersection_output);
	const int expected_intersection[] = { 0, 2, 4 };
	expect_hxarray(intersection_output, expected_intersection, hxsize(expected_intersection));

	// hxset_difference
	hxarray<tracker_t> difference_output;
	difference_output.reserve(hxsize(left) + 1u);
	difference_output.push_back(tracker_t(0));
	// "The output contains keys that appear in the first range but not the second."
	//   sentinel { 0 } + diff { 1 } => { 0, 1 }
	hxset_difference(left+0, left + hxsize(left),
		right+0, right + hxsize(right), difference_output);
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

	// "Performs a stable merge sort of two ordered ranges [begin0, end0) and"
	// "[begin1, end1) -> output." Verify equal keys keep ticket order.
	hxmerge(left+0, left + left_count, right+0, right + right_count, dest+0);

	const hxmerge_test_record_t expected[] = {
		{ 1, 0 }, { 1, 1 }, { 3, 0 }, { 3, 1 },
		{ 5, 0 }, { 5, 1 }, { 5, 2 }, { 7, 0 }
	};
	// Stable result should read {(1,0), (1,1), (3,0), (3,1), (5,0), (5,1), (5,2), (7,0)} top-to-bottom.
	for(size_t i = 0; i < left_count + right_count; ++i) {
		EXPECT_EQ(dest[i].key, expected[i].key);
		EXPECT_EQ(dest[i].ticket, expected[i].ticket);
	}
}

TEST(hxbinary_search_test, simple_case) {
	int ints[5] = { 2, 5, 6, 88, 99 };
	int* ints_end = ints+5;

	// "Performs a binary search in the range [first, last)." Expect hits { 88, 2, 99 }
	// across const and mutable pointers, then confirm misses fall to the end iterator.
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

	// Size 0 range: begin == end so the return points at the sentinel.
	// Empty range returns end.
	result = hxbinary_search(ints, ints, 11, sort_int); // Zero size.
	EXPECT_TRUE(result == ints);
}

TEST(hxbinary_search_test, binary_search_grinder) {
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(4);
	hxarray<tracker_t> sorted; sorted.reserve(100);

	for(int i=100; i-- != 0; ) {
		int x = rng.range(0, 100);
		sorted.push_back(tracker_t(x));
	}
	// "Unsorted data will lead to errors." Force ascending order before the grinder.
	hxsort(sorted.begin(), sorted.end());

	// Every resident value should be rediscovered: pointer equality relaxed to
	// value comparison avoids aliasing when duplicates exist.
	for(size_t i=100u; i-- != 0u; ) {
		tracker_t t = hxmove(sorted[i]); // Don't pass an address that is in the array.
		tracker_t* ptr = hxbinary_search(sorted.begin(), sorted.end(), t);
		// Assert logical equivalence without using ==. The tracker_t* may point
		// elsewhere.
		EXPECT_TRUE(!(*ptr < t) && !(t < *ptr));
	}
}

// ==> TEST(hxhxalgorithm_test, iterator_support).
template<typename sort_callback_t>
static void do_sort_iter_case(const sort_callback_t& sort_callback) {
	using tracker_t = hxhxalgorithm_test_ref_tracker_t;

	const int initial_values[5] = { 2, 1, 0, 4, -5 };
	const int expected_two[5] = { 1, 2, 0, 4, -5 };
	const int expected_sorted[5] = { -5, 0, 1, 2, 4 };
	const int expected_descending[5] = { 4, 2, 1, 0, -5 };
	tracker_t values[5] = {
		tracker_t(initial_values[0]),
		tracker_t(initial_values[1]),
		tracker_t(initial_values[2]),
		tracker_t(initial_values[3]),
		tracker_t(initial_values[4])
	};

	auto reset = [&]() {
		for(size_t i = 0; i < 5; ++i) {
			values[i] = tracker_t(initial_values[i]);
		}
	};

	auto expect_values = [&](const int (&expected)[5]) {
		for(size_t i = 0; i < 5; ++i) {
			EXPECT_EQ(values[i].value, expected[i]);
		}
	};

	// "The end parameter points just past the last element in the range to sort."
	// Iterator wrapper should keep { 2, 1, 0, 4, -5 } unchanged for sizes 0 and 1.
	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values), sort_iter_value_less);
	expect_values(initial_values);

	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 1), sort_iter_value_less);
	expect_values(initial_values);

	// "Requires only the standard pointer operations. No array notation."
	// Confirm head slice { 2, 1 } becomes { 1, 2 } with iterator arithmetic.
	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 2), sort_iter_value_less);
	expect_values(expected_two);

	// "Sorts the elements in the range [begin, end) in comparison order using the insertion sort algorithm."
	// Iterator facade should deliver { -5, 0, 1, 2, 4 }.
	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);

	// Ensure alternate comparator flips to { 4, 2, 1, 0, -5 } before restoring order.
	reset();
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_greater);
	expect_values(expected_descending);

	// One more ascending pass confirms iterator facade remains stable.
	sort_callback(hxhxalgorithm_test_iter_api_t(values), hxhxalgorithm_test_iter_api_t(values + 5), sort_iter_value_less);
	expect_values(expected_sorted);
}

TEST(hxhxalgorithm_test, iterator_support) {
	// Exercise iterator facade with insertion, heap, and hybrid strategies to
	// guarantee the adapter satisfies each contract.
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
