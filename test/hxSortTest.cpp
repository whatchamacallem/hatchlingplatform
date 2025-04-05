// Copyright 2017-2025 Adrian Johnston

#include <hx/hxSort.h>
#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------

class hxRadixSortTest :
	public testing::Test
{
public:
	template<typename Key>
	struct TestObject {
		TestObject(Key k) : id(k) { }
		~TestObject() { id = (Key)0; }
		bool operator<(const TestObject& rhs) const { return id < rhs.id; }
		Key id;
	};

	template<typename Key>
	void generate(hxArray<TestObject<Key> >& a, uint32_t size, uint32_t mask, Key offset) {
		a.reserve(size);
		for(uint32_t i= size;i--;) {
			uint32_t x = m_prng() & mask;
			a.push_back((Key)x - offset);
		}
	}

	template<typename Key>
	static int qSortCompare(const void* a, const void* b) {
		if (*(const TestObject<Key>*)a < *(const TestObject<Key>*)b) { return -1; }
		if (*(const TestObject<Key>*)b < *(const TestObject<Key>*)a) { return 1; }
		return 0;
	}

	template<typename Key>
	void test(uint32_t size, uint32_t mask, Key offset) {
		hxMemoryManagerScope temporaryStack(hxMemoryManagerId_TemporaryStack);

		// Generate test data
		hxArray<TestObject<Key> > a;
		generate<Key>(a, size, mask, offset);

		// Copy and sort test data
		hxArray<TestObject<Key> > b(a);
		::qsort(b.data(), b.size(), sizeof(TestObject<Key>), qSortCompare<Key>);

		// Radix sort
		hxRadixSort<Key, TestObject<Key> > rs; rs.reserve(size);
		for (uint32_t i = size; i--;) {
			rs.insert(a[i].id, &a[i]);
		}

		rs.sort(hxMemoryManagerId_TemporaryStack);

		ASSERT_EQ(b.size(), size);
		ASSERT_EQ(rs.size(), size);

		typename hxRadixSort<Key, TestObject<Key> >::iterator it = rs.begin();
		typename hxRadixSort<Key, TestObject<Key> >::const_iterator cit = rs.cbegin();

		for (uint32_t i=0u; i < size; ++i) {
			ASSERT_EQ(b[i].id, rs[i].id);
			ASSERT_EQ(b[i].id, (*it++).id);
			ASSERT_EQ(b[i].id, (cit++)->id);
		}

		ASSERT_EQ(it, rs.end());
		ASSERT_EQ(cit, rs.cend());
	}

	hxTestRandom m_prng;
};

// ----------------------------------------------------------------------------

TEST_F(hxRadixSortTest, Null) {
	hxRadixSort<uint32_t, const char> rs;

	rs.sort(hxMemoryManagerId_TemporaryStack);
	ASSERT_EQ(rs.size(), 0u);
	ASSERT_TRUE(rs.empty());

	rs.reserve(1u);
	rs.insert(123u, "s");

	rs.sort(hxMemoryManagerId_TemporaryStack);
	ASSERT_EQ(rs.size(), 1u);
	ASSERT_EQ(rs[0], 's');
	ASSERT_EQ(*rs.get(0), 's');
	ASSERT_TRUE(!rs.empty());
}

TEST_F(hxRadixSortTest, Uint32) {
	test<uint32_t>(20u, 0x7fu, 0u); // check insertion sort
	test<uint32_t>(100u, 0x7fu, 0u);
	test<uint32_t>(1000u, 0x7fffu, 0u);
	test<uint32_t>(10000u, ~(uint32_t)0, 0u);
}

TEST_F(hxRadixSortTest, Int32) {
	test<int32_t>(20u, 0x7fu, 0x3f); // check insertion sort
	test<int32_t>(100u, 0x7fu, 0x3f);
	test<int32_t>(1000u, 0x7fffu, 0x3fff);
	test<int32_t>(10000u, ~(uint32_t)0, 0);
}

TEST_F(hxRadixSortTest, Float) {
	test<float>(200u, 0x7fu, (float)0x3f); // check insertion sort
	test<float>(100u, 0x7fu, (float)0x3f);
	test<float>(1000u, 0x7fffu, (float)0x3fff);
	test<float>(10000u, ~(uint32_t)0, 0.0f);
}

TEST_F(hxRadixSortTest, Types) {
	test<uint8_t>(100u, 0x7fu, 0x3fu);
	test<int8_t>(100u, 0x7fu, 0x3f);
	test<uint16_t>(100u, 0x7fu, 0x3fu);
	test<int16_t>(100u, 0x7fu, 0x3f);
}

static int hxSortCompareTest(const int a, const int b) {
	return a < b;
}

TEST(hxInsertionSortTest, SortCompareCCase) {
	int ints[5] = { 2, 1, 0, 4, -5 };

	// sort 0 elements
	hxInsertionSort<int, int(*)(int a, int b)>(ints, ints, hxSortCompareTest);
	const int ints1[5] = { 2, 1, 0, 4, -5 };
	ASSERT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // nothing changed

	// sort 1 element
	hxInsertionSort<int>(ints, ints + 1, hxSortCompareTest);
	ASSERT_TRUE(::memcmp(ints, ints1, sizeof ints) == 0); // still nothing changed

	// sort 2 elements
	hxInsertionSort(ints, ints + 2);
	const int ints2[5] = { 1, 2, 0, 4, -5 };
	ASSERT_TRUE(::memcmp(ints, ints2, sizeof ints) == 0);

	// sort all
	hxInsertionSort(ints, ints + 5, hxSortCompareTest);
	const int ints3[5] = { -5, 0, 1, 2, 4 };
	ASSERT_TRUE(::memcmp(ints, ints3, sizeof ints) == 0); // sorted
}

TEST(hxBinarySearchTest, SimpleCase) {
	int ints[5] = { 2, 5, 6, 88, 99 };
	int* result = hxBinarySearch(ints+0, ints+5, 88, hxSortCompareTest);
	ASSERT_TRUE(result != hxnull && *result == 88);

	const int* cresult = hxBinarySearch((const int*)ints+0, (const int*)ints+5, 2, hxSortCompareTest);
	ASSERT_TRUE(cresult != hxnull && *cresult == 2);

	cresult = hxBinarySearch((const int*)ints+0, (const int*)ints+5, 99);
	ASSERT_TRUE(cresult != hxnull && *cresult == 99);

	result = hxBinarySearch(ints+0, ints+5, 0);
	ASSERT_TRUE(result == hxnull);

	result = hxBinarySearch(ints+0, ints+5, 100);
	ASSERT_TRUE(result == hxnull);

	result = hxBinarySearch(ints+0, ints+5, 7);
	ASSERT_TRUE(result == hxnull);
}


