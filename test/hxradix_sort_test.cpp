// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxradix_sort.hpp>
#include <hx/hxrandom.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxmemory_manager.h>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

class hxradix_sort_test_f :
	public testing::Test
{
public:
	template<typename key_t>
	class hxtest_object {
	public:
		hxtest_object(key_t k) : id(k) { }
		~hxtest_object(void) { id = (key_t)0; }
		bool operator<(const hxtest_object& x) const { return id < x.id; }
		key_t id;
	};

	// Move everything onto the temp stack.
	hxradix_sort_test_f(void) : m_temporary_stack_scope(hxsystem_allocator_temporary_stack) { }

	template<typename key_t>
	void generate(hxarray<hxtest_object<key_t> >& a, uint32_t size, uint32_t mask, key_t offset) {
		a.reserve(size);
		for(uint32_t i= size;i--;) {
			uint32_t x = m_prng() & mask;
			a.push_back((key_t)((key_t)x - offset));
		}
	}

	template<typename key_t>
	static int q_sort_compare(const void* a, const void* b) {
		if(*(const hxtest_object<key_t>*)a < *(const hxtest_object<key_t>*)b) { return -1; }
		if(*(const hxtest_object<key_t>*)b < *(const hxtest_object<key_t>*)a) { return 1; }
		return 0;
	}

	template<typename key_t>
	void test_range_and_type(uint32_t size, uint32_t mask, key_t offset) {
		// Generate test data.
		hxarray<hxtest_object<key_t> > a;
		generate<key_t>(a, size, mask, offset);

		// Copy and sort the test data.
		hxarray<hxtest_object<key_t> > b(a);
		::qsort(b.data(), b.size(), sizeof(hxtest_object<key_t>), q_sort_compare<key_t>);

		// Radix sort using 8-bit digits.

		hxarray<hxradix_sort_key<key_t, hxtest_object<key_t>*>> rs; rs.reserve(size);
		for(uint32_t i = size; i--;) {
			rs.emplace_back(a[i].id, &a[i]);
		}

		hxradix_sort(rs.begin(), rs.end());

		EXPECT_EQ(b.size(), size);
		EXPECT_EQ(rs.size(), size);

		for(uint32_t i=0u; i < size; ++i) {
			EXPECT_EQ(b[i].id, rs[i].get_value()->id);
		}

		// Do it again with 11-bit digits using regular push_back.

		rs.clear();
		for(uint32_t i = size; i--;) {
			rs.push_back(hxradix_sort_key<key_t, hxtest_object<key_t>*>(a[i].id, &a[i]));
		}

		hxradix_sort11(rs.begin(), rs.end());

		EXPECT_EQ(b.size(), size);
		EXPECT_EQ(rs.size(), size);

		for(uint32_t i=0u; i < size; ++i) {
			EXPECT_EQ(b[i].id, rs[i].get_value()->id);
		}
	}

	// Move all tests to the temp stack.
	hxsystem_allocator_scope m_temporary_stack_scope;
	hxrandom m_prng;
};

TEST_F(hxradix_sort_test_f, null) {
	hxarray<hxradix_sort_key<uint32_t, const char*>> rs;
	rs.reserve(1u);

	hxradix_sort(rs.begin(), rs.end());
	EXPECT_EQ(rs.size(), 0u);
	EXPECT_TRUE(rs.empty());

	rs.push_back(hxradix_sort_key<uint32_t, const char*>(123u, "s"));

	hxradix_sort(rs.begin(), rs.end());
	EXPECT_EQ(rs.size(), 1u);
	EXPECT_EQ(rs[0].get_value()[0], 's');
	EXPECT_TRUE(!rs.empty());
}

TEST_F(hxradix_sort_test_f, null11) {
	hxarray<hxradix_sort_key<uint32_t, const char*>> rs;
	rs.reserve(1u);

	hxradix_sort11(rs.begin(), rs.end());
	EXPECT_EQ(rs.size(), 0u);
	EXPECT_TRUE(rs.empty());

	rs.push_back(hxradix_sort_key<uint32_t, const char*>(123u, "s"));

	hxradix_sort11(rs.begin(), rs.end());
	EXPECT_EQ(rs.size(), 1u);
	EXPECT_EQ(rs[0].get_value()[0], 's');
	EXPECT_TRUE(!rs.empty());
}

TEST_F(hxradix_sort_test_f, uint32) {
	test_range_and_type<uint32_t>(20u, 0x7fu, 0u); // Check insertion sort.
	test_range_and_type<uint32_t>(100u, 0x7fu, 0u);
	test_range_and_type<uint32_t>(1000u, 0x7fffu, 0u);
	test_range_and_type<uint32_t>(10000u, ~(uint32_t)0, 0u);
}

TEST_F(hxradix_sort_test_f, int32) {
	test_range_and_type<int32_t>(20u, 0x7fu, 0x3f); // Check insertion sort.
	test_range_and_type<int32_t>(100u, 0x7fu, 0x3f);
	test_range_and_type<int32_t>(1000u, 0x7fffu, 0x3fff);
	test_range_and_type<int32_t>(10000u, ~(uint32_t)0, 0);
}

TEST_F(hxradix_sort_test_f, float) {
	test_range_and_type<float>(200u, 0x7fu, (float)0x3f); // Check insertion sort.
	test_range_and_type<float>(100u, 0x7fu, (float)0x3f);
	test_range_and_type<float>(1000u, 0x7fffu, (float)0x3fff);
	test_range_and_type<float>(10000u, ~(uint32_t)0, 0.0f);
}

TEST_F(hxradix_sort_test_f, types) {
	test_range_and_type<uint8_t>(100u, 0x7fu, 0x3fu);
	test_range_and_type<int8_t>(100u, 0x7fu, 0x3f);
	test_range_and_type<uint16_t>(100u, 0x7fu, 0x3fu);
	test_range_and_type<int16_t>(100u, 0x7fu, 0x3f);
}
