#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxhash_table.hpp>

// These are usable implementations of the hxhash_table Node template parameter.
// These are the keys for a set. Subclasses will give you associated
// values which is a mapping.

// hxhash_table_node_integer - Specialization of hxhash_table_node_base for integer
// types. See documentation of hxhash_table for interface documentation.
// This is a great example of a node that doesn't require a base class.
template<typename key_t_>
class hxhash_table_node_integer {
public:
	typedef key_t_ key_t;

	hxconstexpr_fn hxhash_table_node_integer(const key_t_& key_) :
		m_hash_next_(hxnull), m_key_(key_) { }

	// Boilerplate for hxhash_table.
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }

	// The key and hash identify the Node and should not change once added.
	hxconstexpr_fn const key_t_& key(void) const { return m_key_; }
	hxconstexpr_fn uint32_t hash(void) const { return hxkey_hash(m_key_); };

private:
	hxhash_table_node_integer(void) hxdelete_fn;
	hxhash_table_node_integer(const hxhash_table_node_integer&) hxdelete_fn;
	void operator=(const hxhash_table_node_integer&) hxdelete_fn;

    void* m_hash_next_;
    key_t_ m_key_;
};

// hxhash_table_node_string_literal - Specialization of hxhash_table_set_node for
// static C strings. This code expects the provided strings to outlive the
// container because it is intended for use with string literals.
class hxhash_table_node_string_literal : public hxhash_table_set_node<const char*> {
public:
	// Constructor initializes the node with a string key and computes its hash.
	// - k: The string key to initialize the node with.
	hxconstexpr_fn hxhash_table_node_string_literal(const char* k_)
		: hxhash_table_set_node<const char*>(k_) { }
};

// hxhash_table_node_string - Specialization of hxhash_table_set_node for C strings.
// Allocates a copy, resulting in a string pool per-hash table. The key is
// stored as a pointer to const to keep the hash table code const correct.
template <hxsystem_allocator_t allocator_=hxsystem_allocator_heap>
class hxhash_table_node_string : public hxhash_table_set_node<const char*> {
public:
	// Constructor allocates and duplicates the string key, then initializes the
	// node.
	// - k: The string key to allocate, duplicate, and initialize the node with.
	hxconstexpr_fn hxhash_table_node_string(const char* k_)
		: hxhash_table_set_node(hxstring_duplicate(k_, allocator_)) { }

	// Destructor frees the allocated string key.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxhash_table_node_string(void) { hxfree(const_cast<char *>(this->key())); }
};
