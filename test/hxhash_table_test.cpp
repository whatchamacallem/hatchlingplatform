// Copyright 2017-2025 Adrian Johnston

#include <hx/hxhash_table_nodes.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

static class hxhash_table_test* s_hxtest_current = 0;

class hxhash_table_test :
	public testing::Test
{
public:
    struct test_object {
        test_object() {
            ++s_hxtest_current->m_constructed;
            id = s_hxtest_current->m_next_id++;
        }
        ~test_object() {
            ++s_hxtest_current->m_destructed;
            id = ~0u;
        }

		void operator=(const test_object& rhs) { id = rhs.id; }
		void operator=(int32_t x) { id = x; }
		bool operator==(const test_object& rhs) const { return id == rhs.id; }
		bool operator==(int32_t x) const { return id == x; }

		operator float() const { return (float)id; }

		int32_t id;
	};

	class test_integer : public hxhash_table_node_integer<int32_t> {
	public:
		test_integer(int32_t k) : hxhash_table_node_integer(k) { }
		test_object value;
	};

	class test_string : public hxhash_table_node_string<hxmemory_allocator_temporary_stack> {
	public:
		test_string(const char* k) : hxhash_table_node_string(k) { }
		test_object value;
	};

    hxhash_table_test() {
        hxassert(s_hxtest_current == hxnull);
        m_constructed = 0;
        m_destructed = 0;
        m_next_id = 0;
        s_hxtest_current = this;
    }
    ~hxhash_table_test() {
        s_hxtest_current = 0;
    }

    bool Check_totals(int32_t total) const {
        return m_constructed == total && m_destructed == total;
    }

    int32_t m_constructed;
    int32_t m_destructed;
    int32_t m_next_id;
};

TEST_F(hxhash_table_test, Null) {
	{
		typedef hxhash_table<test_integer, 4> Table;
		Table table;
		ASSERT_EQ(table.size(), 0u);

		ASSERT_TRUE(table.begin() == table.end());
		ASSERT_TRUE(table.c_begin() == table.c_end());
		ASSERT_TRUE(((const Table&)table).begin() == ((const Table&)table).c_end());
		ASSERT_FALSE(table.begin() != table.end());
		ASSERT_FALSE(table.c_begin() != table.c_end());
		ASSERT_FALSE(((const Table&)table).begin() != ((const Table&)table).c_end());

		table.clear();
		ASSERT_EQ(table.load_factor(), 0.0f);

	}
	ASSERT_EQ(m_constructed, 0);
	ASSERT_EQ(m_destructed, 0);
}

TEST_F(hxhash_table_test, Single) {
	static const int k = 77;
	{
		typedef hxhash_table<test_integer, 4> Table;
		Table table;
		test_integer* node = hxnew<test_integer>(k);
		table.insert_node(node);

		// Operations on a single node
		ASSERT_TRUE(table.begin() != table.end());
		ASSERT_TRUE(table.c_begin() != table.c_end());
		ASSERT_TRUE(++table.begin() == table.end());
		ASSERT_TRUE(++table.c_begin() == table.c_end());
		ASSERT_EQ(table.size(), 1u);
		ASSERT_EQ(table.count(k), 1u);
		ASSERT_TRUE(table[k].key() == k);
		ASSERT_TRUE(table[k].value.id == node->value.id);
		ASSERT_TRUE(table.insert_unique(k).value.id == node->value.id);
		ASSERT_TRUE(table.find(k) == node);
		ASSERT_TRUE(table.find(k, node) == hxnull);
		ASSERT_TRUE(((const Table&)table).find(k) == node);
		ASSERT_TRUE(((const Table&)table).find(k, node) == hxnull);

		// MODIFIES TABLE
		ASSERT_TRUE(table.extract(k) == node);
		ASSERT_TRUE(table.extract(k) == hxnull);

		table.insert_node(node);
		ASSERT_TRUE(table.find(k) == node);
		table.release_all();
		ASSERT_TRUE(table.find(k) == hxnull);
		ASSERT_EQ(table.size(), 0u);

		// Operations after single node was removed
		ASSERT_EQ(table.size(), 0u);
		ASSERT_EQ(table.count(k), 0u);
		ASSERT_TRUE(table.find(k) == hxnull);
		ASSERT_TRUE(((const Table&)table).find(k) == hxnull);

		// MODIFIES TABLE
		ASSERT_TRUE(table[k].key() == k);

		// Operations after a different node was allocated
		ASSERT_TRUE(table[k].value.id != node->value.id);
		ASSERT_EQ(table.size(), 1u);
		ASSERT_EQ(table.count(k), 1u);

		// MODIFIES TABLE: destructor also frees allocated item.
		hxdelete(node);
	}
	ASSERT_EQ(m_constructed, 2);
	ASSERT_EQ(m_destructed, 2);
}

TEST_F(hxhash_table_test, Multiple) {
	static const int N = 78;
	{
		// Table will be overloaded.
		typedef hxhash_table<test_integer> Table;
		Table table;
		table.set_table_size_bits(5);

		// Insert N elements
		for (int i = 0; i < N; ++i) {
			ASSERT_EQ(table[i].value.id, i);
			ASSERT_EQ(table[i].key(), i);
		}

		// Check properties of N unique keys.
		int id_histogram[N] = {};
		ASSERT_EQ(table.size(), N);
		Table::iterator it = table.begin();
		Table::iterator cit = table.begin();
		for (int i = 0; i < N; ++i) {
			test_integer* ti = table.find(i);
			ASSERT_EQ(ti->value, i);
			ASSERT_TRUE(table.find(i, ti) == hxnull);

			// Iteration over.
			ASSERT_TRUE(it != table.end());
			ASSERT_TRUE(cit != table.c_end());
			ASSERT_TRUE(it == cit);
			ASSERT_TRUE((unsigned int)it->value.id < (unsigned int)N);
			id_histogram[it->value.id]++;
			ASSERT_TRUE((unsigned int)cit->value.id < (unsigned int)N);
			id_histogram[cit->value.id]++;
			++cit;
			it++;
		}
		ASSERT_TRUE(table.end() == it);
		ASSERT_TRUE(table.c_end() == cit);
		for (int i = 0; i < N; ++i) {
			ASSERT_EQ(id_histogram[i], 2);
		}

		// insert second N elements
		for (int i = 0; i < N; ++i) {
			test_integer* ti = hxnew<test_integer>(i);
			ASSERT_EQ(ti->value.id, i+N);
			table.insert_node(ti);
		}

		// Check properties of 2*N duplicate keys.
		int key_histogram[N] = {};
		ASSERT_EQ(table.size(), N * 2);
		it = table.begin();
		cit = table.begin();
		for (int i = 0; i < N; ++i) {
			test_integer* ti = table.find(i);
			ASSERT_EQ(ti->key(), i);
			const test_integer* ti2 = ((const hxhash_table<test_integer>&)table).find(i, ti); // test const version
			ASSERT_EQ(ti2->key(), i);
			ASSERT_TRUE(table.find(i, ti2) == hxnull);

			ASSERT_EQ(table.count(i), 2u);

			ASSERT_TRUE((unsigned int)it->key() < (unsigned int)N);
			key_histogram[it->key()]++;
			++it;
			ASSERT_TRUE((unsigned int)it->key() < (unsigned int)N);
			key_histogram[it->key()]++;
			it++;
			ASSERT_TRUE((unsigned int)cit->key() < (unsigned int)N);
			key_histogram[cit->key()]++;
			++cit;
			ASSERT_TRUE((unsigned int)cit->key() < (unsigned int)N);
			key_histogram[cit->key()]++;
			cit++;
		}
		ASSERT_TRUE(table.end() == it);
		ASSERT_TRUE(table.c_end() == cit);
		for (int i = 0; i < N; ++i) {
			ASSERT_EQ(key_histogram[i], 4);
		}

		// Check that keys are distributed such that no bucket has more than 2x average.
		ASSERT_TRUE((table.load_factor() * 2.0f) > (float)table.load_max());

		// Erase keys [0..N/2), remove 1 of 2 of keys [N/2..N)
		for (int i = 0; i < (N/2); ++i) {
			ASSERT_EQ(table.erase(i), 2);
		}
		for (int i = (N/2); i < N; ++i) {
			test_integer* ti = table.extract(i);
			ASSERT_TRUE(ti->key() == i);
			hxdelete(ti);
		}

		// Check properties of N/2 remaining keys.
		for (int i = 0; i < (N/2); ++i) {
			ASSERT_EQ(table.release_key(i), 0);
			ASSERT_TRUE(table.find(i) == hxnull);
		}
		for (int i = (N/2); i < N; ++i) {
			test_integer* ti = table.find(i);
			ASSERT_EQ(ti->key(), i);
			ASSERT_TRUE(table.find(i, ti) == hxnull);
			ASSERT_EQ(table.count(i), 1u);
		}

		it = table.begin();
		cit = table.begin();
		for (int i = 0; i < (N/2); ++i) {
			++it;
			cit++;
		}
		ASSERT_TRUE(table.end() == it);
		ASSERT_TRUE(table.c_end() == cit);
	}
	ASSERT_EQ(m_constructed, 2*N);
	ASSERT_EQ(m_destructed, 2*N);
}

TEST_F(hxhash_table_test, Strings) {
	static const char* colors[] = {
		"Red","Orange","Yellow",
		"Green","Cyan","Blue",
		"Indigo","Violet" };
	const int sz = sizeof colors / sizeof *colors;

	{
		typedef hxhash_table<test_string, 4> Table;
		Table table;

		for (int i = sz; i--;) {
			ASSERT_TRUE(::strcmp(table[colors[i]].key(), colors[i]) == 0);
		}
		ASSERT_TRUE(table.find("Cyan") != hxnull);
		ASSERT_TRUE(table.find("Pink") == hxnull);

	}
	ASSERT_EQ(m_constructed, sz);
	ASSERT_EQ(m_destructed, sz);
}
