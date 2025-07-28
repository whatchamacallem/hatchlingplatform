// Copyright 2017-2025 Adrian Johnston

#include <hx/hxrandom.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxtest.hpp>
#include <limits.h>

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
enum {
	sort_api_default_constructor,
	sort_api_int_constructor,
	sort_api_const_ref_constructor,
	sort_api_temp_constructor,
	sort_api_destructor,
	sort_api_assignment_const_ref,
	sort_api_assignment_temp,
	sort_api_less_than,
	sort_api_count,
};

static int g_sort_api_histogram[sort_api_count];

class sort_api_t {
public:
    // Keep track of what is being used.

	sort_api_t() hxnoexcept : value(0) {
		++g_sort_api_histogram[sort_api_default_constructor];
	}

    explicit sort_api_t(int value) hxnoexcept : value(value) {
		++g_sort_api_histogram[sort_api_int_constructor];
	}

    sort_api_t(const sort_api_t& other) hxnoexcept : value(other.value) {
		++g_sort_api_histogram[sort_api_const_ref_constructor];
	}

    sort_api_t(sort_api_t&& other) hxnoexcept : value(hxmove(other.value)) {
		++g_sort_api_histogram[sort_api_temp_constructor];
	}

	~sort_api_t() {
		++g_sort_api_histogram[sort_api_destructor];
	}

    sort_api_t& operator=(const sort_api_t& other) hxnoexcept {
		++g_sort_api_histogram[sort_api_assignment_const_ref];
        if (this != &other) {
            value = other.value;
        }
        return *this;
    }

    sort_api_t& operator=(sort_api_t&& other) hxnoexcept {
		++g_sort_api_histogram[sort_api_assignment_temp];
        if (this != &other) {
            value = hxmove(other.value);
        }
        return *this;
    }

    bool operator<(const sort_api_t& other) const hxnoexcept {
		++g_sort_api_histogram[sort_api_less_than];
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
};

TEST(hxsort_test, sort_grinder) {
	hxrandom rng;
	hxarray<sort_api_t> api_objects; api_objects.reserve(0x40);
	hxarray<sort_api_t> insertion_sorted; insertion_sorted.reserve(0x40);
	hxarray<sort_api_t> heap_sorted; heap_sorted.reserve(0x40);
	hxarray<sort_api_t> generic_sorted; generic_sorted.reserve(0x40);

	for(int i=8; i--; ) {
		::memset(g_sort_api_histogram, 0x00, sizeof g_sort_api_histogram);

		// Generate a lot of equal values this time.
		for(int j=(0x2f >> i) & rng; j--;) {
			api_objects.push_back(sort_api_t(rng.range(100, 200)));
		}

		hxinsertion_sort(api_objects.begin(), api_objects.end());


		api_objects.clear();

	}



	do_sort_grinder_case(rng, api_objects, hxinsertion_sort<int, bool (*)(int, int)>);
	do_sort_grinder_case(rng, api_objects, hxheapsort<int, bool (*)(int, int)>);
	do_sort_grinder_case(rng, api_objects, hxsort<int, bool (*)(int, int)>);
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
