// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxhash_table_nodes.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

static class hxhash_table_test_f* s_hxtest_current = 0;

class hxhash_table_test_f :
	public testing::Test
{
public:
	class hxtest_object {
	public:
		hxtest_object(void) {
			++s_hxtest_current->m_constructed;
			id = s_hxtest_current->m_next_id++;
		}
		~hxtest_object(void) {
			++s_hxtest_current->m_destructed;
			id = -1;
		}

		void operator=(const hxtest_object& x) { id = x.id; }
		void operator=(int32_t x) { id = x; }
		bool operator==(const hxtest_object& x) const { return id == x.id; }
		bool operator==(int32_t x) const { return id == x; }

		operator float(void) const { return (float)id; }

		int32_t id;
	};

	class hxtest_integer : public hxhash_table_node_integer<int32_t> {
	public:
		hxtest_integer(int32_t k) : hxhash_table_node_integer(k) { }
		hxtest_object value;
	};

	class hxtest_string : public hxhash_table_node_string<hxsystem_allocator_temporary_stack> {
	public:
		hxtest_string(const char* k) : hxhash_table_node_string(k) { }
		hxtest_object value;
	};

	class hxtest_string_literal : public hxhash_table_node_string_literal {
	public:
		hxtest_string_literal(const char* k) : hxhash_table_node_string_literal(k) { }
	};

	hxhash_table_test_f(void) {
		hxassert(s_hxtest_current == hxnull);
		m_constructed = 0;
		m_destructed = 0;
		m_next_id = 0;
		s_hxtest_current = this;
	}
	~hxhash_table_test_f(void) {
		s_hxtest_current = 0;
	}

	int32_t m_constructed;
	int32_t m_destructed;
	int32_t m_next_id;
};

TEST_F(hxhash_table_test_f, null) {
	{
		using table_t = hxhash_table<hxtest_integer, 4>;
		table_t table;
		EXPECT_EQ(table.size(), 0u);

		// "Returns an iterator pointing to the beginning of the hash table." Empty table => begin == end and load factor 0.
		EXPECT_TRUE(table.begin() == table.end());
		EXPECT_TRUE(table.cbegin() == table.cend());
		EXPECT_TRUE(((const table_t&)table).begin() == ((const table_t&)table).cend());
		EXPECT_FALSE(table.begin() != table.end());
		EXPECT_FALSE(table.cbegin() != table.cend());
		EXPECT_FALSE(((const table_t&)table).begin() != ((const table_t&)table).cend());

		// "Removes all nodes and calls deleter_t::operator() on every node." Clearing untouched table keeps load factor { 0.0 }.
		table.clear();
		EXPECT_EQ(table.load_factor(), 0.0f);

	}
	EXPECT_EQ(m_constructed, 0);
	EXPECT_EQ(m_destructed, 0);
}

TEST_F(hxhash_table_test_f, single) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const int k = 77;
	{
		using table_t = hxhash_table<hxtest_integer, 4>;
		table_t table;
		hxtest_integer* node = hxnew<hxtest_integer>(k);
		// "Inserts a node_t into the hash table, allowing duplicate keys." Seed table with manual node.
		table.insert_node(node);

		// "Returns a node containing key if any or allocates and returns a new one." Iterator + count checks confirm single-entry semantics.
		EXPECT_TRUE(table.begin() != table.end());
		EXPECT_TRUE(table.cbegin() != table.cend());
		EXPECT_TRUE(++table.begin() == table.end());
		EXPECT_TRUE(++table.cbegin() == table.cend());
		EXPECT_EQ(table.size(), 1u);
		EXPECT_EQ(table.count(k), 1u);
		EXPECT_TRUE(table[k].key() == k);
		EXPECT_TRUE(table[k].value.id == node->value.id);
		EXPECT_TRUE(table.insert_unique(k).value.id == node->value.id);
		// Lookup stack: find() returns { node }, subsequent cursor with previous skips duplicates.
		EXPECT_TRUE(table.find(k) == node);
		EXPECT_TRUE(table.find(k, node) == hxnull);
		EXPECT_TRUE(((const table_t&)table).find(k) == node);
		EXPECT_TRUE(((const table_t&)table).find(k, node) == hxnull);

		// "Removes and returns the first node_t with the given key." Ensure repeated calls -> { node, hxnull }.
		EXPECT_TRUE(table.extract(k) == node);
		EXPECT_TRUE(table.extract(k) == hxnull);

		table.insert_node(node);
		EXPECT_TRUE(table.find(k) == node);
		// "Clears the hash table without deleting any Nodes." After release_all(), find returns hxnull while node still alive.
		table.release_all();
		EXPECT_TRUE(table.find(k) == hxnull);
		EXPECT_EQ(table.size(), 0u);

		// Operations after the single node was removed.
		EXPECT_EQ(table.size(), 0u);
		EXPECT_EQ(table.count(k), 0u);
		EXPECT_TRUE(table.find(k) == hxnull);
		EXPECT_TRUE(((const table_t&)table).find(k) == hxnull);

		// MODIFIES TABLE
		EXPECT_TRUE(table[k].key() == k);

		// Operations after a different node was allocated.
		EXPECT_TRUE(table[k].value.id != node->value.id);
		EXPECT_EQ(table.size(), 1u);
		EXPECT_EQ(table.count(k), 1u);

		// MODIFIES TABLE: Destructor also frees allocated item.
		hxdelete(node);
	}
	EXPECT_EQ(m_constructed, 2);
	EXPECT_EQ(m_destructed, 2);
}

TEST_F(hxhash_table_test_f, map_node_usage) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	using map_node_t = hxhash_table_map_node<int32_t, hxtest_object>;
	using table_t = hxhash_table<map_node_t, 4>;
	{
		table_t table;
		// "value_t must default-construct when using subscripting." Confirm key 10 spawns default map entry { value.id = 0 } before assignment.
		map_node_t& via_subscript = table[10];
		EXPECT_EQ(via_subscript.key(), 10);
		via_subscript.value().id = 123;

		map_node_t* manual = hxnew<map_node_t>(20);
		manual->value().id = 321;
		// Link external allocation through insert_node to co-exist with subscript entry.
		table.insert_node(manual);

		EXPECT_EQ(table.size(), 2u);
		EXPECT_EQ(table.count(10), 1u);
		EXPECT_EQ(table.count(20), 1u);

		EXPECT_EQ(&table[10], &via_subscript);
		const table_t& const_table = table;
		const map_node_t* const_lookup = const_table.find(10);
		EXPECT_TRUE(const_lookup != hxnull);
		if(const_lookup != hxnull) {
			EXPECT_EQ(const_lookup->value().id, 123);
		}

		map_node_t* manual_lookup = table.find(20);
		EXPECT_TRUE(manual_lookup != hxnull);
		if(manual_lookup != hxnull) {
			EXPECT_EQ(manual_lookup->value().id, 321);
		}

		// Duplicate insert returns same storage -> verifies uniqueness guard.
		map_node_t& duplicate_lookup = table.insert_unique(10);
		EXPECT_EQ(&duplicate_lookup, &via_subscript);
	}

	EXPECT_EQ(m_constructed, 2);
	EXPECT_EQ(m_destructed, 2);
}

TEST_F(hxhash_table_test_f, multiple) {
	static const int N = 78;
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	{
		// Table will be overloaded.
		using table_t = hxhash_table<hxtest_integer>;
		table_t table;
		// "Use set_table_size_bits to configure hash bits dynamically." Force 2^5 buckets before load test.
		table.set_table_size_bits(5);

		// Subscript pipeline seeds keys { 0..N-1 } with value.id mirroring the key.
		for(int i = 0; i < N; ++i) {
			EXPECT_EQ(table[i].value.id, i);
			EXPECT_EQ(table[i].key(), i);
		}

		// Check properties of N unique keys.
		int id_histogram[N] = {};
		EXPECT_EQ(table.size(), N);
		table_t::iterator it = table.begin();
		table_t::iterator cit = table.begin();
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

		// "Returns the average number of Nodes per bucket." Ensure load_max stays within 2x mean occupancy.
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

TEST_F(hxhash_table_test_f, strings) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const char* colors[] = {
		"Red","Orange","Yellow",
		"Green","Cyan","Blue",
		"Indigo","Violet" };
	const int sz = hxsize(colors);

	{
		using table_t = hxhash_table<hxtest_string, 4>;
		table_t table;

		// "Allocates a copy, resulting in a string pool per hash table."
		for(int i = sz; i-- != 0;) {
			EXPECT_STREQ(table[colors[i]].key(), colors[i]);
		}
		EXPECT_TRUE(table.find("Cyan") != hxnull);
		EXPECT_TRUE(table.find("Pink") == hxnull);

	}
	EXPECT_EQ(m_constructed, sz);
	EXPECT_EQ(m_destructed, sz);
}

TEST_F(hxhash_table_test_f, string_literal_nodes) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);

	static const char* const literals[] = {
		"Crimson", "Teal", "Magenta", "Gold"
	};

	using table_t = hxhash_table<hxtest_string_literal, 4>;
	table_t table;

	for(unsigned int i = 0; i < hxsize(literals); ++i) {
		// "Specialization of hxhash_table_set_node for static C strings." Literal keys stay owned externally while lookups remain stable.
		hxtest_string_literal& entry = table[literals[i]];
		EXPECT_EQ(table.find(literals[i]), &entry);
		EXPECT_EQ(&table.insert_unique(literals[i]), &entry);

		EXPECT_TRUE(entry.key() == literals[i]);
		EXPECT_STREQ(entry.key(), literals[i]);
		EXPECT_EQ(entry.hash(), hxkey_hash(literals[i]));
		EXPECT_EQ(table.count(literals[i]), 1u);
	}

	EXPECT_EQ(table.size(), (size_t)hxsize(literals));
	EXPECT_FALSE(table.find("Crimson") == hxnull);
	EXPECT_TRUE(table.find("Cerulean") == hxnull);
}
