// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxsort.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// The hxsort_test_api_t tests check for correct use of references to temporaries.
class hxsort_test_api_t {
public:
	// This is not used by the sort code.
    explicit hxsort_test_api_t(int value_) : value(value_) { }

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
	hxsort_test_iter_api_t operator++(int) { hxassert(m_pointer != hxnull); hxsort_test_iter_api_t temp_(*this); ++m_pointer; return temp_; }
	hxsort_test_iter_api_t& operator--(void) { hxassert(m_pointer != hxnull); --m_pointer; return *this; }
	hxsort_test_iter_api_t operator+(ptrdiff_t offset_) const { hxassert(m_pointer != hxnull); return hxsort_test_iter_api_t(m_pointer + offset_); }
	hxsort_test_iter_api_t operator-(ptrdiff_t offset_) const { hxassert(m_pointer != hxnull); return hxsort_test_iter_api_t(m_pointer - offset_); }
	ptrdiff_t operator-(const hxsort_test_iter_api_t& other_) const { hxassert(m_pointer != hxnull); return m_pointer - other_.m_pointer; }

	bool operator==(const hxsort_test_iter_api_t& other_) const { return m_pointer == other_.m_pointer; }
	bool operator!=(const hxsort_test_iter_api_t& other_) const { return m_pointer != other_.m_pointer; }
	bool operator<(const hxsort_test_iter_api_t& other_) const { return m_pointer < other_.m_pointer; }
	bool operator>(const hxsort_test_iter_api_t& other_) const { return m_pointer > other_.m_pointer; }
	bool operator<=(const hxsort_test_iter_api_t& other_) const { return m_pointer <= other_.m_pointer; }
	bool operator>=(const hxsort_test_iter_api_t& other_) const { return m_pointer >= other_.m_pointer; }
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

static bool sort_iter_value_less(const hxsort_test_api_t& lhs_, const hxsort_test_api_t& rhs_) {
	return lhs_.value < rhs_.value;
}

static bool sort_iter_value_greater(const hxsort_test_api_t& lhs_, const hxsort_test_api_t& rhs_) {
	return lhs_.value > rhs_.value;
}

TEST(hxsort_test, hxmerge_iterator_support) {
	hxsort_test_api_t left_[3] = { hxsort_test_api_t(1), hxsort_test_api_t(3), hxsort_test_api_t(5) };
	hxsort_test_api_t right_[3] = { hxsort_test_api_t(2), hxsort_test_api_t(4), hxsort_test_api_t(6) };
	hxsort_test_api_t dest_[6] = {
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0),
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0)
	};

	hxmerge(hxsort_test_iter_api_t(left_), hxsort_test_iter_api_t(left_ + 3),
		hxsort_test_iter_api_t(right_), hxsort_test_iter_api_t(right_ + 3),
		hxsort_test_iter_api_t(dest_), sort_iter_value_less);

	const int expected_sorted_[6] = { 1, 2, 3, 4, 5, 6 };
	for(size_t i_ = 0; i_ < 6; ++i_) {
		EXPECT_EQ(dest_[i_].value, expected_sorted_[i_]);
	}

	// Do it all over again with a GE functor and the parameters reversed.
	hxsort_test_api_t left_desc_[3] = { hxsort_test_api_t(5), hxsort_test_api_t(3), hxsort_test_api_t(1) };
	hxsort_test_api_t right_desc_[3] = { hxsort_test_api_t(6), hxsort_test_api_t(4), hxsort_test_api_t(2) };
	hxsort_test_api_t dest_desc_[6] = {
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0),
		hxsort_test_api_t(0), hxsort_test_api_t(0), hxsort_test_api_t(0)
	};

	hxmerge(hxsort_test_iter_api_t(left_desc_), hxsort_test_iter_api_t(left_desc_ + 3),
		hxsort_test_iter_api_t(right_desc_), hxsort_test_iter_api_t(right_desc_ + 3),
		hxsort_test_iter_api_t(dest_desc_), sort_iter_value_greater);

	const int expected_desc_[6] = { 6, 5, 4, 3, 2, 1 };
	for(size_t i_ = 0; i_ < 6; ++i_) {
		EXPECT_EQ(dest_desc_[i_].value, expected_desc_[i_]);
	}
}

TEST(hxsort_test, hxbinary_search_iterator_support) {
	hxsort_test_api_t values_[7] = {
		hxsort_test_api_t(-5), hxsort_test_api_t(-1), hxsort_test_api_t(0), hxsort_test_api_t(3),
		hxsort_test_api_t(5), hxsort_test_api_t(8), hxsort_test_api_t(12)
	};

	hxsort_test_iter_api_t begin_(values_);
	hxsort_test_iter_api_t end_(values_ + 7);

	hxsort_test_api_t key_three_(3);
	hxsort_test_iter_api_t result_ = hxbinary_search(begin_, end_, key_three_, sort_iter_value_less);
	EXPECT_NE(result_, end_);
	EXPECT_EQ((*result_).value, 3);

	hxsort_test_api_t key_high_(12);
	result_ = hxbinary_search(begin_, end_, key_high_, sort_iter_value_less);
	EXPECT_NE(result_, end_);
	EXPECT_EQ((*result_).value, 12);

	hxsort_test_api_t missing_(7);
	result_ = hxbinary_search(begin_, end_, missing_, sort_iter_value_less);
	EXPECT_EQ(result_, end_);

	// Empty list.
	result_ = hxbinary_search(begin_, begin_, key_three_, sort_iter_value_less);
	EXPECT_EQ(result_, begin_);
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
			// Use pointers just to show that they are dereferenced by hxkey_less.
			EXPECT_FALSE(hxkey_less(sorted.get(j + 1), sorted.get(j)));
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

template<typename sort_callback_t_>
void do_sort_int_case(const sort_callback_t_& sort_callback_) {
	int ints[5] = { 2, 1, 0, 4, -5 };

	// Sort 0 elements.
	sort_callback_(ints, ints, sort_int);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Nothing changed.

	// Sort 1 element.
	sort_callback_(ints, ints + 1, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // Still nothing changed.

	// Sort 2 elements.
	sort_callback_(ints, ints + 2, sort_int);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// Sort all elements.
	sort_callback_(ints, ints + 5, sort_int);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// Sort in reverse order.
	sort_callback_(ints, ints + 5, sort_int_reverse);
	const int ints4[5] = { 4, 2, 1, 0, -5 };
	EXPECT_TRUE(::memcmp(ints, ints4, sizeof ints) == 0);

	// Sort the reversed array back into ascending order.
	sort_callback_(ints, ints + 5, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);
}

template<typename sort_callback_t_>
static void do_sort_iter_case(const sort_callback_t_& sort_callback_) {
	const int initial_values_[5] = { 2, 1, 0, 4, -5 };
	const int expected_two_[5] = { 1, 2, 0, 4, -5 };
	const int expected_sorted_[5] = { -5, 0, 1, 2, 4 };
	const int expected_descending_[5] = { 4, 2, 1, 0, -5 };
	hxsort_test_api_t values_[5] = {
		hxsort_test_api_t(initial_values_[0]),
		hxsort_test_api_t(initial_values_[1]),
		hxsort_test_api_t(initial_values_[2]),
		hxsort_test_api_t(initial_values_[3]),
		hxsort_test_api_t(initial_values_[4])
	};

	auto reset_ = [&]() {
		for(size_t i_ = 0; i_ < 5; ++i_) {
			values_[i_] = hxsort_test_api_t(initial_values_[i_]);
		}
	};

	auto expect_values_ = [&](const int (&expected_)[5]) {
		for(size_t i_ = 0; i_ < 5; ++i_) {
			EXPECT_EQ(values_[i_].value, expected_[i_]);
		}
	};

	reset_();
	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_), sort_iter_value_less);
	expect_values_(initial_values_);

	reset_();
	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_ + 1), sort_iter_value_less);
	expect_values_(initial_values_);

	reset_();
	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_ + 2), sort_iter_value_less);
	expect_values_(expected_two_);

	reset_();
	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_ + 5), sort_iter_value_less);
	expect_values_(expected_sorted_);

	reset_();
	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_ + 5), sort_iter_value_greater);
	expect_values_(expected_descending_);

	sort_callback_(hxsort_test_iter_api_t(values_), hxsort_test_iter_api_t(values_ + 5), sort_iter_value_less);
	expect_values_(expected_sorted_);
}

TEST(hxsort_test, sort_int_case) {
	// Instantiate and pass the sort templates as function pointers.
	do_sort_int_case(hxinsertion_sort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxheapsort<int*, bool (*)(int, int)>);
	do_sort_int_case(hxsort<int*, bool (*)(int, int)>);
}

TEST(hxmerge_test, preserves_stable_ordering) {
	struct hxmerge_test_record_t {
		int key;
		int ticket;

		bool operator<(const hxmerge_test_record_t& other_) const {
			return key < other_.key;
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
	do_sort_iter_case([](hxsort_test_iter_api_t begin_, hxsort_test_iter_api_t end_, const auto& less_) {
		hxinsertion_sort(begin_, end_, less_);
	});

	do_sort_iter_case([](hxsort_test_iter_api_t begin_, hxsort_test_iter_api_t end_, const auto& less_) {
		hxheapsort(begin_, end_, less_);
	});

	do_sort_iter_case([](hxsort_test_iter_api_t begin_, hxsort_test_iter_api_t end_, const auto& less_) {
		hxsort(begin_, end_, less_);
	});
}
#endif // HX_CPLUSPLUS >= 201402L
