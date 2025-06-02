#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxHashTable.hpp>

// Usable implementations of the hxHashTable Node template parameter.
// These are the keys for a set.  Subclasses will give you associated
// values which is a mapping.

// ----------------------------------------------------------------------------
// hxHashTableNodeInteger. Specialization of hxHashTableNodeBase for integer types.
// See documentation of hxHashTable for interface documentation.
template<typename Key_>
class hxHashTableNodeInteger {
public:
	typedef Key_ Key;

	HX_CONSTEXPR_FN hxHashTableNodeInteger(const Key& key_) :
		m_hashNext(hxnull), m_key(key_) { }

	// The key and hash identify the Node and should not change once added.
	const Key& key() const { return m_key; }
	uint32_t hash() const { return hxKeyHash(m_key); };

private:
	hxHashTableNodeInteger(void) HX_DELETE_FN;
	hxHashTableNodeInteger(const hxHashTableNodeInteger&) HX_DELETE_FN;
	void operator=(const hxHashTableNodeInteger&) HX_DELETE_FN;

	// The hash table uses m_hashNext to implement an embedded linked list.
	template<typename, uint32_t> friend class hxHashTable;
	hxHashTableNodeInteger* m_hashNext;
	Key m_key;
};

// ----------------------------------------------------------------------------
// hxHashTableNodeStringLiteral. Specialization of hxHashTableSetNode for
// static C strings.  This code expects the provided strings to outlive the
// container because it is intended for use with string literals.

class hxHashTableNodeStringLiteral : public hxHashTableSetNode<const char*> {
public:
	// Constructor: Initializes the node with a string key and computes its hash.
	// Parameters:
	// - k_: The string key to initialize the node with.
	HX_CONSTEXPR_FN hxHashTableNodeStringLiteral(const char* k_)
		: hxHashTableSetNode<const char*>(k_) { }
};

// ----------------------------------------------------------------------------
// hxHashTableNodeString. Specialization of hxHashTableSetNode for C strings.
// Allocates a copy, resulting in a string pool per-hash table.  The key is
// stored as a pointer to const to keep the hash table code const correct.

template <hxMemoryManagerId allocator_=hxMemoryManagerId_Heap>
class hxHashTableNodeString : public hxHashTableSetNode<const char*> {
public:
	// Constructor: Allocates and duplicates the string key, then initializes the node.
	// Parameters:
	// - k_: The string key to allocate, duplicate, and initialize the node with.
	HX_CONSTEXPR_FN hxHashTableNodeString(const char* k_)
		: hxHashTableSetNode(hxStringDuplicate(k_, allocator_)) { }

	// Destructor: Frees the allocated string key.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxHashTableNodeString() { hxFree(const_cast<char *>(this->key())); }
};
