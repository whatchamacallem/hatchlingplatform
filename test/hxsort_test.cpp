// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxrandom.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxtest.hpp>
#include <hx/hxarray.hpp>
#include <limits.h>
#include <stdio.h>

HX_REGISTER_FILENAME_HASH

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

	// sort 0 elements
	sort_callback_(ints, ints, sort_int);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // nothing changed

	// sort 1 element
	sort_callback_(ints, ints + 1, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // still nothing changed

	// sort 2 elements
	sort_callback_(ints, ints + 2, sort_int);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// sort all
	sort_callback_(ints, ints + 5, sort_int);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// sort reversed
	sort_callback_(ints, ints + 5, sort_int_reverse);
	const int ints4[5] = { 4, 2, 1, 0, -5 };
	EXPECT_TRUE(::memcmp(ints, ints4, sizeof ints) == 0);

	// sort reversed back to sorted
	sort_callback_(ints, ints + 5, sort_int);
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);
}

TEST(hxsort_test, sort_int_case) {
	// Instantiate and pass the sort templates as functors.
	do_sort_int_case(hxinsertion_sort<int, bool (*)(int, int)>);
	do_sort_int_case(hxheapsort<int, bool (*)(int, int)>);
	do_sort_int_case(hxsort<int, bool (*)(int, int)>);
}

TEST(hxbinary_search_test, simple_case) {
	int ints[5] = { 2, 5, 6, 88, 99 };
	int* result = hxbinary_search(ints+0, ints+5, 88, sort_int);
	EXPECT_TRUE(result != hxnull && *result == 88);

	const int* cresult = hxbinary_search((const int*)ints+0, (const int*)ints+5, 2, sort_int);
	EXPECT_TRUE(cresult != hxnull && *cresult == 2);

	cresult = hxbinary_search((const int*)ints+0, (const int*)ints+5, 99);
	EXPECT_TRUE(cresult != hxnull && *cresult == 99);

	result = hxbinary_search(ints+0, ints+5, 0);
	EXPECT_TRUE(result == hxnull);

	result = hxbinary_search(ints+0, ints+5, 100);
	EXPECT_TRUE(result == hxnull);

	result = hxbinary_search(ints+0, ints+5, 7);
	EXPECT_TRUE(result == hxnull);
}

// The sort_api_t tests check for correct use of references to temporaries.
class sort_api_t {
public:
	// This is not used by the sort code.
    explicit sort_api_t(int value_) noexcept : value(value_) { }

	// This is what is being used.

    sort_api_t(sort_api_t&& other) noexcept : value(other.value) {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
	}

	~sort_api_t() {
		::memset(&value, 0xef, sizeof value);
	}

    sort_api_t& operator=(sort_api_t&& other) noexcept {
		// Callee may leave itself in an unusable state or crash.
		hxassert(this != &other);
		value = other.value;
        return *this;
    }

	// Called by hxkey_less below.
    bool operator<(const sort_api_t& other) const noexcept {
		// Technically legal but indicates an optimization issue.
		hxassert(this != &other);
        return value < other.value;
    }

    int value;

private:
	// This is what is not being used.

	sort_api_t() = delete;
    sort_api_t(const sort_api_t& other) = delete;
    sort_api_t& operator=(const sort_api_t& other) = delete;
    bool operator==(const sort_api_t& other) const = delete;
    bool operator!=(const sort_api_t& other) const = delete;
    bool operator>(const sort_api_t& other) const = delete;
    bool operator>=(const sort_api_t& other) const = delete;
    bool operator<=(const sort_api_t& other) const = delete;
    sort_api_t operator+(const sort_api_t& other) const = delete;
    sort_api_t operator-(const sort_api_t& other) const = delete;
    sort_api_t operator*(const sort_api_t& other) const = delete;
    sort_api_t operator/(const sort_api_t& other) const = delete;
    sort_api_t operator%(const sort_api_t& other) const = delete;
    sort_api_t& operator+=(const sort_api_t& other) = delete;
    sort_api_t& operator-=(const sort_api_t& other) = delete;
    sort_api_t& operator*=(const sort_api_t& other) = delete;
    sort_api_t& operator/=(const sort_api_t& other) = delete;
    sort_api_t& operator%=(const sort_api_t& other) = delete;
	bool operator&(const sort_api_t& other) const = delete;
	bool operator|(const sort_api_t& other) const = delete;
	bool operator^(const sort_api_t& other) const = delete;
	sort_api_t operator~(void) const = delete;
	sort_api_t operator<<(const sort_api_t& other) const = delete;
	sort_api_t operator>>(const sort_api_t& other) const = delete;
	sort_api_t& operator&=(const sort_api_t& other) = delete;
	sort_api_t& operator|=(const sort_api_t& other) = delete;
	sort_api_t& operator^=(const sort_api_t& other) = delete;
	sort_api_t& operator<<=(const sort_api_t& other) = delete;
	sort_api_t& operator>>=(const sort_api_t& other) = delete;
	bool operator&&(const sort_api_t& other) const = delete;
	bool operator||(const sort_api_t& other) const = delete;
	bool operator!(void) const = delete;
};

TEST(hxsort_test, sort_grinder) {
	hxrandom rng(2);
	size_t max_size_mask = 0x7f;
	hxarray<sort_api_t> insertion_sorted; insertion_sorted.reserve(max_size_mask);
	hxarray<sort_api_t> heap_sorted; heap_sorted.reserve(max_size_mask);
	hxarray<sort_api_t> generic_sorted; generic_sorted.reserve(max_size_mask);

	for(int i=12; i--; ) {
		// Set up the arrays to be sorted.
		size_t size = (max_size_mask >> i) & rng;
		for(size_t j=size; j--;) {
			insertion_sorted.push_back(sort_api_t(rng.range(100, 200)));
			// Use the && constructor and not the const& one.
			heap_sorted.push_back(sort_api_t(0));
			generic_sorted.push_back(sort_api_t(0));
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
	hxrandom rng(3);
	size_t max_size_mask = 0xffff;
	hxarray<sort_api_t> sorted; sorted.reserve(max_size_mask);
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
			sorted.push_back(sort_api_t(x));
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
			EXPECT_FALSE(hxkey_less(&sorted[j + 1], &sorted[j]));
		}
		for(size_t j=20000u; j-- > 10000u;) {
			EXPECT_EQ(histogram[j], 0);
		}
		sorted.clear();
	}
}

TEST(hxbinary_search_test, binary_search_grinder) {
	hxrandom rng(4);
	hxarray<sort_api_t> sorted; sorted.reserve(100);

	for(int i=100; i--; ) {
		int x = rng.range(0, 100);
		sorted.push_back(sort_api_t(x));
	}
	hxsort(sorted.begin(), sorted.end());

	for(size_t i=100u; i--; ) {
		sort_api_t t = (sort_api_t&&)sorted[i]; // Don't pass an address that is in the array.
		sort_api_t* ptr = hxbinary_search(sorted.begin(), sorted.end(), t);
		// Assert logical equivalence without using ==. The pointer may point
		// elsewhere.
		EXPECT_TRUE(!(*ptr < t) && !(t < *ptr));
	}
}
