// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxhash_table_nodes.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

static class hxhash_table_test* s_hxtest_current = 0;

class hxhash_table_test :
	public testing::Test
{
public:
	class test_object {
	public:
		test_object(void) {
			++s_hxtest_current->m_constructed;
			id = s_hxtest_current->m_next_id++;
		}
		~test_object(void) {
			++s_hxtest_current->m_destructed;
			id = -1;
		}

		void operator=(const test_object& x) { id = x.id; }
		void operator=(int32_t x) { id = x; }
		bool operator==(const test_object& x) const { return id == x.id; }
		bool operator==(int32_t x) const { return id == x; }

		operator float(void) const { return (float)id; }

		int32_t id;
	};

	class hxtest_integer : public hxhash_table_node_integer<int32_t> {
	public:
		hxtest_integer(int32_t k) : hxhash_table_node_integer(k) { }
		test_object value;
	};

	class hxtest_string : public hxhash_table_node_string<hxsystem_allocator_temporary_stack> {
	public:
		hxtest_string(const char* k) : hxhash_table_node_string(k) { }
		test_object value;
	};

	hxhash_table_test(void) {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = 0;
		s_hxtest_current = this;
	}
	~hxhash_table_test(void) {
		s_hxtest_current = 0;
	}

	int32_t m_constructed;
	int32_t m_destructed;
	int32_t m_next_id;
};

TEST_F(hxhash_table_test, null) {
		{
		typedef hxhash_table<hxtest_integer, 4> Table;
		Table table;
		EXPECT_EQ(table.size(), 0u);

		EXPECT_TRUE(table.begin() == table.end());
		EXPECT_TRUE(table.cbegin() == table.cend());
		EXPECT_TRUE(((const Table&)table).begin() == ((const Table&)table).cend());
		EXPECT_FALSE(table.begin() != table.end());
		EXPECT_FALSE(table.cbegin() != table.cend());
		EXPECT_FALSE(((const Table&)table).begin() != ((const Table&)table).cend());

		table.clear();
		EXPECT_EQ(table.load_factor(), 0.0f);

	}
	EXPECT_EQ(m_constructed, 0);
	EXPECT_EQ(m_destructed, 0);
}

TEST_F(hxhash_table_test, single) {
	hxsystem_allocator_scope temporary_stack_scope = hxsystem_allocator_temporary_stack;

	static const int k = 77;
	{
		typedef hxhash_table<hxtest_integer, 4> Table;
		Table table;
		hxtest_integer* node = hxnew<hxtest_integer>(k);
		table.insert_node(node);

		// Operations on a single node
		EXPECT_TRUE(table.begin() != table.end());
		EXPECT_TRUE(table.cbegin() != table.cend());
		EXPECT_TRUE(++table.begin() == table.end());
		EXPECT_TRUE(++table.cbegin() == table.cend());
		EXPECT_EQ(table.size(), 1u);
		EXPECT_EQ(table.count(k), 1u);
		EXPECT_TRUE(table[k].key() == k);
		EXPECT_TRUE(table[k].value.id == node->value.id);
		EXPECT_TRUE(table.insert_unique(k).value.id == node->value.id);
		EXPECT_TRUE(table.find(k) == node);
		EXPECT_TRUE(table.find(k, node) == hxnull);
		EXPECT_TRUE(((const Table&)table).find(k) == node);
		EXPECT_TRUE(((const Table&)table).find(k, node) == hxnull);

		// MODIFIES TABLE
		EXPECT_TRUE(table.extract(k) == node);
		EXPECT_TRUE(table.extract(k) == hxnull);

		table.insert_node(node);
		EXPECT_TRUE(table.find(k) == node);
		table.release_all();
		EXPECT_TRUE(table.find(k) == hxnull);
		EXPECT_EQ(table.size(), 0u);

		// Operations after single node was removed
		EXPECT_EQ(table.size(), 0u);
		EXPECT_EQ(table.count(k), 0u);
		EXPECT_TRUE(table.find(k) == hxnull);
		EXPECT_TRUE(((const Table&)table).find(k) == hxnull);

		// MODIFIES TABLE
		EXPECT_TRUE(table[k].key() == k);

		// Operations after a different node was allocated
		EXPECT_TRUE(table[k].value.id != node->value.id);
		EXPECT_EQ(table.size(), 1u);
		EXPECT_EQ(table.count(k), 1u);

		// MODIFIES TABLE: destructor also frees allocated item.
		hxdelete(node);
	}
	EXPECT_EQ(m_constructed, 2);
	EXPECT_EQ(m_destructed, 2);
}

TEST_F(hxhash_table_test, multiple) {
	static const int N = 78;
	hxsystem_allocator_scope temporary_stack_scope = hxsystem_allocator_temporary_stack;
	{
		// Table will be overloaded.
		typedef hxhash_table<hxtest_integer> Table;
		Table table;
		table.set_table_size_bits(5);

		// Insert N elements
		for(int i = 0; i < N; ++i) {
			EXPECT_EQ(table[i].value.id, i);
			EXPECT_EQ(table[i].key(), i);
		}

		// Check properties of N unique keys.
		int id_histogram[N] = {};
		EXPECT_EQ(table.size(), N);
		Table::iterator it = table.begin();
		Table::iterator cit = table.begin();
		for(int i = 0; i < N; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->value, i);
			EXPECT_TRUE(table.find(i, ti) == hxnull);

			// Iteration over.
			EXPECT_TRUE(it != table.end());
			EXPECT_TRUE(cit != table.cend());
			EXPECT_TRUE(it == cit);
			EXPECT_TRUE((unsigned int)it->value.id < (unsigned int)N);
			id_histogram[it->value.id]++;
			EXPECT_TRUE((unsigned int)cit->value.id < (unsigned int)N);
			id_histogram[cit->value.id]++;
			++cit;
			it++;
		}
		EXPECT_TRUE(table.end() == it);
		EXPECT_TRUE(table.cend() == cit);
		for(int i = 0; i < N; ++i) {
			EXPECT_EQ(id_histogram[i], 2);
		}

		// insert second N elements
		for(int i = 0; i < N; ++i) {
			hxtest_integer* ti = hxnew<hxtest_integer>(i);
			EXPECT_EQ(ti->value.id, i+N);
			table.insert_node(ti);
		}

		// Check properties of 2*N duplicate keys.
		int key_histogram[N] = {};
		EXPECT_EQ(table.size(), N * 2);
		it = table.begin();
		cit = table.begin();
		for(int i = 0; i < N; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->key(), i);
			const hxtest_integer* ti2 = ((const hxhash_table<hxtest_integer>&)table).find(i, ti); // test const version
			EXPECT_EQ(ti2->key(), i);
			EXPECT_TRUE(table.find(i, ti2) == hxnull);

			EXPECT_EQ(table.count(i), 2u);

			EXPECT_TRUE((unsigned int)it->key() < (unsigned int)N);
			key_histogram[it->key()]++;
			++it;
			EXPECT_TRUE((unsigned int)it->key() < (unsigned int)N);
			key_histogram[it->key()]++;
			it++;
			EXPECT_TRUE((unsigned int)cit->key() < (unsigned int)N);
			key_histogram[cit->key()]++;
			++cit;
			EXPECT_TRUE((unsigned int)cit->key() < (unsigned int)N);
			key_histogram[cit->key()]++;
			cit++;
		}
		EXPECT_TRUE(table.end() == it);
		EXPECT_TRUE(table.cend() == cit);
		for(int i = 0; i < N; ++i) {
			EXPECT_EQ(key_histogram[i], 4);
		}

		// Check that keys are distributed such that no bucket has more than 2x average.
		EXPECT_TRUE((table.load_factor() * 2.0f) > (float)table.load_max());

		// Erase keys [0..N/2), remove 1 of 2 of keys [N/2..N)
		for(int i = 0; i < (N/2); ++i) {
			EXPECT_EQ(table.erase(i), 2);
		}
		for(int i = (N/2); i < N; ++i) {
			hxtest_integer* ti = table.extract(i);
			EXPECT_TRUE(ti->key() == i);
			hxdelete(ti);
		}

		// Check properties of N/2 remaining keys.
		for(int i = 0; i < (N/2); ++i) {
			EXPECT_EQ(table.release_key(i), 0);
			EXPECT_TRUE(table.find(i) == hxnull);
		}
		for(int i = (N/2); i < N; ++i) {
			hxtest_integer* ti = table.find(i);
			EXPECT_EQ(ti->key(), i);
			EXPECT_TRUE(table.find(i, ti) == hxnull);
			EXPECT_EQ(table.count(i), 1u);
		}

		it = table.begin();
		cit = table.begin();
		for(int i = 0; i < (N/2); ++i) {
			++it;
			cit++;
		}
		EXPECT_TRUE(table.end() == it);
		EXPECT_TRUE(table.cend() == cit);
	}
	EXPECT_EQ(m_constructed, 2*N);
	EXPECT_EQ(m_destructed, 2*N);
}

TEST_F(hxhash_table_test, strings) {
	hxsystem_allocator_scope temporary_stack_scope = hxsystem_allocator_temporary_stack;

	static const char* colors[] = {
		"Red","Orange","Yellow",
		"Green","Cyan","Blue",
		"Indigo","Violet" };
	const int sz = sizeof colors / sizeof *colors;

	{
		typedef hxhash_table<hxtest_string, 4> Table;
		Table table;

		for(int i = sz; i--;) {
			EXPECT_TRUE(::strcmp(table[colors[i]].key(), colors[i]) == 0);
		}
		EXPECT_TRUE(table.find("Cyan") != hxnull);
		EXPECT_TRUE(table.find("Pink") == hxnull);

	}
	EXPECT_EQ(m_constructed, sz);
	EXPECT_EQ(m_destructed, sz);
}
