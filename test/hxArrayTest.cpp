// Copyright 2017-2025 Adrian Johnston

#include <hx/hxArray.hpp>
#include <hx/hxTest.hpp>

#include <limits.h>

#if HX_CPLUSPLUS >= 201103L
#include <utility>
#endif

HX_REGISTER_FILENAME_HASH

static class hxArrayTest* s_hxTestCurrent = hxnull;

class hxArrayTest :
	public testing::Test
{
public:
	struct TestObject {
		TestObject() {
			++s_hxTestCurrent->m_constructed;
			id = (INT_MIN < s_hxTestCurrent->m_nextId) ? s_hxTestCurrent->m_nextId-- : 0;
			constructor = 0;
		}

		TestObject(const TestObject& rhs) {
			++s_hxTestCurrent->m_constructed;
			id = rhs.id;
			constructor = rhs.constructor;
		}
		explicit TestObject(int32_t x) {
			hxAssert(x >= 0); // User supplied IDs are positive
			++s_hxTestCurrent->m_constructed;
			id = x;
			constructor = 0;
		}
		~TestObject() {
			++s_hxTestCurrent->m_destructed;
			id = ~0u;
		}

		void operator=(const TestObject& rhs) { id = rhs.id; }
		bool operator==(int32_t x) const { return id == x; }

		int32_t id;
		int32_t constructor;
	};

	hxArrayTest() {
		hxAssert(s_hxTestCurrent == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_nextId = -1;
		s_hxTestCurrent = this;
	}
	~hxArrayTest() {
		s_hxTestCurrent = 0;
	}

	bool CheckTotals(size_t total) const {
		return m_constructed == total && m_destructed == total;
	}

	size_t m_constructed;
	size_t m_destructed;
	int32_t m_nextId;
};

TEST_F(hxArrayTest, Null) {
	{
		TestObject to0;
		TestObject to1;
		ASSERT_EQ(to0.id, -1);
		ASSERT_EQ(to1.id, -2);
	}
	ASSERT_TRUE(CheckTotals(2));
}

TEST_F(hxArrayTest, EmptyFull) {
	hxArray<TestObject, hxAllocatorDynamicCapacity> a;
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(a.full());
	a.reserve(1);
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(!a.full());
	a.pushBack(TestObject());
	ASSERT_TRUE(!a.empty());
	ASSERT_TRUE(a.full());
	a.popBack();
	ASSERT_TRUE(a.empty());
	ASSERT_TRUE(!a.full());
}

TEST_F(hxArrayTest, Allocators) {
	hxArray<TestObject> objsDynamic;
	objsDynamic.reserve(10u);
	hxArray<TestObject, 10u> objsStatic;

	ASSERT_EQ(objsDynamic.size(), 0u);
	ASSERT_EQ(objsStatic.size(), 0u);

	objsDynamic.pushBack(TestObject(20));
	objsDynamic.pushBack(TestObject(21));
	objsStatic.pushBack(TestObject(20));
	objsStatic.pushBack(TestObject(21));

	ASSERT_EQ(objsDynamic.size(), 2u);
	ASSERT_EQ(objsDynamic[0], 20);
	ASSERT_EQ(objsDynamic[1], 21);
	ASSERT_EQ(objsStatic.size(), 2u);
	ASSERT_EQ(objsStatic[0], 20);
	ASSERT_EQ(objsStatic[1], 21);

	objsDynamic.clear();
	objsStatic.clear();

	ASSERT_TRUE(CheckTotals(8));
}

TEST_F(hxArrayTest, Iteration) {
	{
		static const int32_t nums[3] = { 21, 22, 23 };

		hxArray<TestObject, 10u> objs;
		objs.pushBack(TestObject(nums[0]));
		objs.pushBack(TestObject(nums[1]));
		objs.pushBack(TestObject(nums[2]));

		const hxArray<TestObject, 10u>& cobjs = objs;

		int32_t counter = 0;
		for (hxArray<TestObject, 10u>::iterator it = objs.begin(); it != objs.end(); ++it) {
			ASSERT_EQ(it->id, objs[counter].id);
			ASSERT_EQ(it->id, nums[counter]);
			++counter;
		}

		counter = 0;
		for (hxArray<TestObject, 10u>::constIterator it = cobjs.begin();
				it != cobjs.end(); ++it) {
			ASSERT_EQ(it->id, objs[counter].id);
			ASSERT_EQ(it->id, nums[counter]);
			++counter;
		}

		ASSERT_EQ(objs.front(), nums[0]);
		ASSERT_EQ(objs.back(), nums[2]);
		ASSERT_EQ(cobjs.front(), nums[0]);
		ASSERT_EQ(cobjs.back(), nums[2]);
	}

	ASSERT_TRUE(CheckTotals(6));
}

TEST_F(hxArrayTest, Modification) {
	{
		static const int32_t nums[5] = { 91, 92, 93, 94, 95 };

		hxArray<TestObject> objs;
		objs.assign(nums, nums + (sizeof nums / sizeof *nums));

		ASSERT_EQ(objs.capacity(), 5u);
		ASSERT_EQ(objs.size(), 5u);

		// 91, 92, 93, 94

		objs.popBack();
		objs.popBack();
		objs.popBack();

		TestObject to;
		objs.pushBack(to);
		objs.pushBack((const TestObject&)to);

		::new (objs.emplaceBackUnconstructed()) TestObject;

		// 91, 92, -1, -2, -3

		objs.eraseUnordered(1);

		// 91, -2, -1

		ASSERT_EQ(objs[0].id, 91);
		ASSERT_EQ(objs[1].id, -2);
		ASSERT_EQ(objs[2].id, -1);
	}

	ASSERT_TRUE(CheckTotals(9));
}

TEST_F(hxArrayTest, Resizing) {
	{
		static const int32_t nums[5] = { 51, 52, 53, 54, 55 };

		hxArray<TestObject> objs;
		objs.reserve(10);
		objs.assign(nums);

		objs.resize(3);

		ASSERT_EQ(objs.size(), 3u);
		ASSERT_EQ(objs[0].id, 51);
		ASSERT_EQ(objs[2].id, 53);

		objs.resize(4u);

		ASSERT_EQ(objs.size(), 4u);
		ASSERT_EQ(objs[0].id, 51);
		ASSERT_EQ(objs[2].id, 53);
		ASSERT_EQ(objs[3].id, -1);
		ASSERT_EQ(objs.capacity(), 10u);

		objs.resize(10u);
		ASSERT_EQ(objs.size(), 10u);
		ASSERT_EQ(objs[9].id, -7);

		ASSERT_FALSE(objs.empty());
		objs.clear();
		ASSERT_EQ(objs.size(), 0u);
		ASSERT_TRUE(objs.empty());

		ASSERT_EQ(objs.capacity(), 10u);
	}

	ASSERT_TRUE(CheckTotals(12));
}

TEST_F(hxArrayTest, Assignment) {
	{
		hxArray<TestObject> objs;
		objs.reserve(1);

		TestObject to;
		to.id = 67;
		objs.pushBack(to);

		hxArray<TestObject> objs2;
		objs2 = objs; // Assign to same type

		hxArray<TestObject, 1> objs3;
		objs3 = objs; // Assign to different type

		hxArray<TestObject> objs4(objs); // Construct from same type

		hxArray<TestObject, 1> objs5(objs); // Construct from different type

		ASSERT_EQ(objs2.size(), 1u);
		ASSERT_EQ(objs3.size(), 1u);
		ASSERT_EQ(objs4.size(), 1u);
		ASSERT_EQ(objs5.size(), 1u);

		ASSERT_EQ(objs2[0].id, 67);
		ASSERT_EQ(objs3[0].id, 67);
		ASSERT_EQ(objs4[0].id, 67);
		ASSERT_EQ(objs5[0].id, 67);
	}

	ASSERT_TRUE(CheckTotals(6));
}

#if HX_CPLUSPLUS >= 201103L
#include <utility>
TEST_F(hxArrayTest, InitializerList) {
	hxArray<int, 2> x = { 2, 7 };
	ASSERT_EQ(x[1], 7);

	hxArray<int> y { 12, 17 };
	ASSERT_EQ(y[1], 17);
}

TEST_F(hxArrayTest, Temporaries) {
	// test r-value dynamically allocated temporaries
	{
		hxMemoryAllocatorScope allocatorScope(hxMemoryAllocator_TemporaryStack);

		hxArray<int> x(hxArray<int>({ 2, 7 }));
		hxArray<int> y = std::move(x); // should swap
		hxArray<int> z;
		hxswap(y, z);
		ASSERT_TRUE(x.empty());
		ASSERT_TRUE(y.empty());
		ASSERT_EQ(z[0], 2);
		ASSERT_EQ(z[1], 7);
	}
}
#endif
