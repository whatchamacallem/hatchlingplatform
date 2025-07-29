// Copyright 2017-2025 Adrian Johnston

#include <hx/hxrandom.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxtest.hpp>
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
	do_sort_int_case(hxinsertion_sort<int, bool (*)(int, int)>);
	do_sort_int_case(hxheapsort<int, bool (*)(int, int)>);
	do_sort_int_case(hxsort<int, bool (*)(int, int)>);
}

// Now things get serious. Keep histograms of operator activity and start grinding.
static struct sort_stats_t {
	int default_constructor;
	int int_constructor;
	int const_ref_constructor;
	int temp_constructor;
	int destructor;
	int assignment_const_ref;
	int assignment_temp;
	int less_than;
} g_sort_api_histogram;

static void print_stats(sort_stats_t& stats_) {
	printf( "default_constructor: %d int_constructor: %d const_ref_constructor: %d temp_constructor: %d\n"
			"destructor: %d assignment_const_ref: %d assignment_temp: %d less_than: %d\n",
		stats_.default_constructor,
		stats_.int_constructor,
		stats_.const_ref_constructor,
		stats_.temp_constructor,
		stats_.destructor,
		stats_.assignment_const_ref,
		stats_.assignment_temp,
		stats_.less_than);
}

static int add_stats(sort_stats_t& stats_) {
	return stats_.default_constructor
		 + stats_.int_constructor
		 + stats_.const_ref_constructor
		 + stats_.temp_constructor
		 + stats_.destructor
		 + stats_.assignment_const_ref
		 + stats_.assignment_temp
		 + stats_.less_than;
}

class sort_api_t {
public:
    // Keep track of what is being used.

	sort_api_t() hxnoexcept : value(0) {
		++g_sort_api_histogram.default_constructor;
	}

    explicit sort_api_t(int value_) hxnoexcept : value(value_) {
		++g_sort_api_histogram.int_constructor;
	}

    sort_api_t(const sort_api_t& other) hxnoexcept : value(other.value) {
		hxassert(this != &other);
		++g_sort_api_histogram.const_ref_constructor;
	}

    sort_api_t(sort_api_t&& other) hxnoexcept : value(hxmove(other.value)) {
		hxassert(this != &other);
		++g_sort_api_histogram.temp_constructor;
	}

	~sort_api_t() {
		++g_sort_api_histogram.destructor;
	}

    sort_api_t& operator=(const sort_api_t& other) hxnoexcept {
		hxassert(this != &other);
		++g_sort_api_histogram.assignment_const_ref;
		value = other.value;
        return *this;
    }

    sort_api_t& operator=(sort_api_t&& other) hxnoexcept {
		hxassert(this != &other);
		++g_sort_api_histogram.assignment_temp;
		value = hxmove(other.value);
        return *this;
    }

    bool operator<(const sort_api_t& other) const hxnoexcept {
		hxassert(this != &other);
		++g_sort_api_histogram.less_than;
        return value < other.value;
    }

    int value;

private:
    // Keep track of what isn't being used.

    bool operator==(const sort_api_t& other) const hxdelete_fn;
    bool operator!=(const sort_api_t& other) const hxdelete_fn;
    bool operator>(const sort_api_t& other) const hxdelete_fn;
    bool operator>=(const sort_api_t& other) const hxdelete_fn;
    bool operator<=(const sort_api_t& other) const hxdelete_fn;
    sort_api_t operator+(const sort_api_t& other) const hxdelete_fn;
    sort_api_t operator-(const sort_api_t& other) const hxdelete_fn;
    sort_api_t operator*(const sort_api_t& other) const hxdelete_fn;
    sort_api_t operator/(const sort_api_t& other) const hxdelete_fn;
    sort_api_t operator%(const sort_api_t& other) const hxdelete_fn;
    sort_api_t& operator+=(const sort_api_t& other) hxdelete_fn;
    sort_api_t& operator-=(const sort_api_t& other) hxdelete_fn;
    sort_api_t& operator*=(const sort_api_t& other) hxdelete_fn;
    sort_api_t& operator/=(const sort_api_t& other) hxdelete_fn;
    sort_api_t& operator%=(const sort_api_t& other) hxdelete_fn;
	bool operator&(const sort_api_t& other) const hxdelete_fn;
	bool operator|(const sort_api_t& other) const hxdelete_fn;
	bool operator^(const sort_api_t& other) const hxdelete_fn;
	sort_api_t operator~(void) const hxdelete_fn;
	sort_api_t operator<<(const sort_api_t& other) const hxdelete_fn;
	sort_api_t operator>>(const sort_api_t& other) const hxdelete_fn;
	sort_api_t& operator&=(const sort_api_t& other) hxdelete_fn;
	sort_api_t& operator|=(const sort_api_t& other) hxdelete_fn;
	sort_api_t& operator^=(const sort_api_t& other) hxdelete_fn;
	sort_api_t& operator<<=(const sort_api_t& other) hxdelete_fn;
	sort_api_t& operator>>=(const sort_api_t& other) hxdelete_fn;
	bool operator&&(const sort_api_t& other) const hxdelete_fn;
	bool operator||(const sort_api_t& other) const hxdelete_fn;
	bool operator!(void) const hxdelete_fn;
};

TEST(hxsort_test, sort_grinder) {
	hxrandom rng(2);
	int max_size_mask = 0x2f;
	hxarray<sort_api_t> insertion_sorted; insertion_sorted.reserve(max_size_mask);
	hxarray<sort_api_t> heap_sorted; heap_sorted.reserve(max_size_mask);
	hxarray<sort_api_t> generic_sorted; generic_sorted.reserve(max_size_mask);

	for(int i=8; i--; ) {
		// Set up the arrays to be sorted.
		int size = (max_size_mask >> i) & rng;
		for(int j=size; j--;) {
			insertion_sorted.push_back(sort_api_t(rng.range(100, 200)));
		}

		heap_sorted.resize(size);
		::memcpy(heap_sorted.data(), insertion_sorted.data(), insertion_sorted.size_bytes());

		generic_sorted.resize(size);
		::memcpy(generic_sorted.data(), insertion_sorted.data(), insertion_sorted.size_bytes());

		// Clear the histogram and run hxinsertion_sort.
		::memset(&g_sort_api_histogram, 0x00, sizeof g_sort_api_histogram);
		hxinsertion_sort(insertion_sorted.begin(), insertion_sorted.end());
		puts("-----------------------------------------------------------------");
		printf("insertion_sorted %d -> %d\n", size, add_stats(g_sort_api_histogram));
		print_stats(g_sort_api_histogram);

		// Clear the histogram and run hxheapsort.
		::memset(&g_sort_api_histogram, 0x00, sizeof g_sort_api_histogram);
		hxheapsort(heap_sorted.begin(), heap_sorted.end());
		puts("-----------------------------------------------------------------");
		printf("heap_sorted %d -> %d\n", size, add_stats(g_sort_api_histogram));
		print_stats(g_sort_api_histogram);

		// Clear the histogram and run hxinsertion_sort.
		::memset(&g_sort_api_histogram, 0x00, sizeof g_sort_api_histogram);
		hxsort(generic_sorted.begin(), generic_sorted.end());
		puts("-----------------------------------------------------------------");
		printf("generic_sorted %d -> %d\n", size, add_stats(g_sort_api_histogram));
		print_stats(g_sort_api_histogram);

		// Compare the three results to confirm they are sorted.
		ASSERT_EQ(::memcmp(insertion_sorted.data(), heap_sorted.data(), insertion_sorted.size_bytes()), 0);
		ASSERT_EQ(::memcmp(insertion_sorted.data(), generic_sorted.data(), insertion_sorted.size_bytes()), 0);

		insertion_sorted.clear();
		heap_sorted.clear();
		generic_sorted.clear();
	}
}

TEST(xxx, sort_grinder_generic) {
	hxrandom rng(3);
	int max_size_mask = 0xffff;
	hxarray<sort_api_t> sorted; sorted.reserve(max_size_mask);
	hxarray<int> histogram(20000, 0);

	for(int i=10; i--; ) {
		// Pick random values of increasing maximum value up to 2^16 and keep a count of them.
		int size = (max_size_mask >> i) & rng;
		if(size <= 16) {
			continue;
		}
		for(int j=size; j--;) {
			int x = rng.range(10000, 10000);
			sorted.push_back(sort_api_t(x));
			++histogram[x];
		}

		::memset(&g_sort_api_histogram, 0x00, sizeof g_sort_api_histogram);
		hxsort(sorted.begin(), sorted.end());

		puts("-----------------------------------------------------------------");
		printf("sorted %d -> %d\n", size, add_stats(g_sort_api_histogram));
		print_stats(g_sort_api_histogram);

		// Check that all values are accounted for starting with the last one.
		// Confirm sort order with (j <= j+1) while walking down to the first
		// value. Note size > 16.
		--histogram[sorted[size - 1].value];
		for(int j=size - 1; j--;) {
			--histogram[sorted[j].value];
			EXPECT_LE(sorted[j], sorted[j + 1]);
		}
		for(int j=20000; j-- > 10000;) {
			EXPECT_EQ(histogram[j], 0);
		}
		sorted.clear();
	}
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

TEST(hxbinary_search_test, binary_search_grinder) {
	hxrandom rng(4);
	hxarray<sort_api_t> sorted; sorted.reserve(100);

	for(int i=100; i--; ) {
		int x = rng.range(0, 100);
		sorted.push_back(sort_api_t(x));
	}
	hxsort(sorted.begin(), sorted.end());

	for(int i=100; i--; ) {
		sort_api_t* ptr = hxbinary_search(sorted.begin(), sorted.end(), sorted[i]);
		// Assert logical equivalence without using ==. The pointer may point
		// elsewhere.
		EXPECT_TRUE(!(*ptr < sorted[i]) && !(sorted[i] < *ptr));
	}
}
