// Copyright 2017-2025 Adrian Johnston

#include <hx/hxrandom.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxradix_sort.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

class hxradix_sort_test :
	public testing::Test
{
public:
	template<typename key_t>
	class test_object {
	public:
		test_object(key_t k) : id(k) { }
		~test_object(void) { id = (key_t)0; }
		bool operator<(const test_object& rhs) const { return id < rhs.id; }
		key_t id;
	};

	template<typename key_t>
	void generate(hxarray<test_object<key_t> >& a, uint32_t size, uint32_t mask, key_t offset) {
		a.reserve(size);
		for(uint32_t i= size;i--;) {
			uint32_t x = m_prng_() & mask;
			a.push_back((key_t)x - offset);
		}
	}

	template<typename key_t>
	static int q_sort_compare(const void* a, const void* b) {
		if (*(const test_object<key_t>*)a < *(const test_object<key_t>*)b) { return -1; }
		if (*(const test_object<key_t>*)b < *(const test_object<key_t>*)a) { return 1; }
		return 0;
	}

	template<typename key_t>
	void test_range_and_type(uint32_t size, uint32_t mask, key_t offset) {
		hxsystem_allocator_scope temporary_stack(hxsystem_allocator_temporary_stack);

		// Generate test_range_and_type data
		hxarray<test_object<key_t> > a;
		generate<key_t>(a, size, mask, offset);

		// Copy and sort test_range_and_type data
		hxarray<test_object<key_t> > b(a);
		::qsort(b.data(), b.size(), sizeof(test_object<key_t>), q_sort_compare<key_t>);

		// Radix sort
		hxradix_sort<key_t, test_object<key_t> > rs; rs.reserve(size);
		for (uint32_t i = size; i--;) {
			rs.insert(a[i].id, &a[i]);
		}

		rs.sort(hxsystem_allocator_temporary_stack);

		EXPECT_EQ(b.size(), size);
		EXPECT_EQ(rs.size(), size);

		typename hxradix_sort<key_t, test_object<key_t> >::iterator it = rs.begin();
		typename hxradix_sort<key_t, test_object<key_t> >::const_iterator cit = rs.cbegin();

		for (uint32_t i=0u; i < size; ++i) {
			EXPECT_EQ(b[i].id, rs[i].id);
			EXPECT_EQ(b[i].id, (*it++).id);
			EXPECT_EQ(b[i].id, (cit++)->id);
		}

		EXPECT_EQ(it, rs.end());
		EXPECT_EQ(cit, rs.cend());
	}

	hxrandom m_prng_;
};

TEST_F(hxradix_sort_test, null) {
	hxradix_sort<uint32_t, const char> rs;

	rs.sort(hxsystem_allocator_temporary_stack);
	EXPECT_EQ(rs.size(), 0u);
	EXPECT_TRUE(rs.empty());

	rs.reserve(1u);
	rs.insert(123u, "s");

	rs.sort(hxsystem_allocator_temporary_stack);
	EXPECT_EQ(rs.size(), 1u);
	EXPECT_EQ(rs[0], 's');
	EXPECT_EQ(*rs.get(0), 's');
	EXPECT_TRUE(!rs.empty());
}

TEST_F(hxradix_sort_test, uint32) {
	test_range_and_type<uint32_t>(20u, 0x7fu, 0u); // check insertion sort
	test_range_and_type<uint32_t>(100u, 0x7fu, 0u);
	test_range_and_type<uint32_t>(1000u, 0x7fffu, 0u);
	test_range_and_type<uint32_t>(10000u, ~(uint32_t)0, 0u);
}

TEST_F(hxradix_sort_test, int32) {
	test_range_and_type<int32_t>(20u, 0x7fu, 0x3f); // check insertion sort
	test_range_and_type<int32_t>(100u, 0x7fu, 0x3f);
	test_range_and_type<int32_t>(1000u, 0x7fffu, 0x3fff);
	test_range_and_type<int32_t>(10000u, ~(uint32_t)0, 0);
}

TEST_F(hxradix_sort_test, float) {
	test_range_and_type<float>(200u, 0x7fu, (float)0x3f); // check insertion sort
	test_range_and_type<float>(100u, 0x7fu, (float)0x3f);
	test_range_and_type<float>(1000u, 0x7fffu, (float)0x3fff);
	test_range_and_type<float>(10000u, ~(uint32_t)0, 0.0f);
}

TEST_F(hxradix_sort_test, types) {
	test_range_and_type<uint8_t>(100u, 0x7fu, 0x3fu);
	test_range_and_type<int8_t>(100u, 0x7fu, 0x3f);
	test_range_and_type<uint16_t>(100u, 0x7fu, 0x3fu);
	test_range_and_type<int16_t>(100u, 0x7fu, 0x3f);
}

static int hxsort_compare_test(const int a, const int b) {
	return a < b;
}

static int hxsort_compare_reverse_test(const int a, const int b) {
	return a > b;
}

TEST(hxinsertion_sort_test, sort_compare_case) {
	int ints[5] = { 2, 1, 0, 4, -5 };

	// sort 0 elements
	hxinsertion_sort<int, int(*)(int a, int b)>(ints, ints, hxsort_compare_test);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // nothing changed

	// sort 1 element
	hxinsertion_sort<int>(ints, ints + 1, hxsort_compare_test);
	EXPECT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // still nothing changed

	// sort 2 elements
	hxinsertion_sort(ints, ints + 2);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	EXPECT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// sort all
	hxinsertion_sort(ints, ints + 5, hxsort_compare_test);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// sort already sorted
	hxinsertion_sort(ints, ints + 5, hxsort_compare_test);
	EXPECT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0);

	// sort reversed
	hxinsertion_sort(ints, ints + 5, hxsort_compare_reverse_test);
	const int ints4[5] = { 4, 2, 1, 0, -5 };
	EXPECT_TRUE(::memcmp(ints, ints4, sizeof ints) == 0);
}

TEST(hxbinary_search_test, simple_case) {
	int ints[5] = { 2, 5, 6, 88, 99 };
	int* result = hxbinary_search(ints+0, ints+5, 88, hxsort_compare_test);
	EXPECT_TRUE(result != hxnull && *result == 88);

	const int* cresult = hxbinary_search((const int*)ints+0, (const int*)ints+5, 2, hxsort_compare_test);
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
