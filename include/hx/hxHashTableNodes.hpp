#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxHashTable.hpp>

// Usable implementations of the hxHashTable Node template parameter.
// These are the keys for a set.  Subclasses will give you associated
// values which is a mapping.

// ----------------------------------------------------------------------------
// hxHashTableNodeInteger. Specialization of hxHashTableNodeBase for integer types.
// See documentation of hxHashTable for interface documentation.
// This is a great example of a node that doesn't require a base class.
template<typename Key_>
class hxHashTableNodeInteger {
public:
	typedef Key_ Key;

	HX_CONSTEXPR_FN hxHashTableNodeInteger(const Key& key_) :
		m_hashNext(hxnull), m_key(key_) { }

	// Boilerplate for hxHashTable.
	void* hashNext(void) const { return m_hashNext; }
	void*& hashNext(void) { return m_hashNext; }

	// The key and hash identify the Node and should not change once added.
	HX_CONSTEXPR_FN const Key& key() const { return m_key; }
	HX_CONSTEXPR_FN uint32_t hash() const { return hxKeyHash(m_key); };

private:
	hxHashTableNodeInteger(void) HX_DELETE_FN;
	hxHashTableNodeInteger(const hxHashTableNodeInteger&) HX_DELETE_FN;
	void operator=(const hxHashTableNodeInteger&) HX_DELETE_FN;

	void* m_hashNext;
	Key m_key;
};

// ----------------------------------------------------------------------------
// hxHashTableNodeStringLiteral. Specialization of hxHashTableSetNode for
// static C strings.  This code expects the provided strings to outlive the
// container because it is intended for use with string literals.

class hxHashTableNodeStringLiteral : public hxHashTableSetNode<const char*> {
public:
	// Constructor initializes the node with a string key and computes its hash.
	// - k_: The string key to initialize the node with.
	HX_CONSTEXPR_FN hxHashTableNodeStringLiteral(const char* k_)
		: hxHashTableSetNode<const char*>(k_) { }
};

// ----------------------------------------------------------------------------
// hxHashTableNodeString. Specialization of hxHashTableSetNode for C strings.
// Allocates a copy, resulting in a string pool per-hash table.  The key is
// stored as a pointer to const to keep the hash table code const correct.

template <hxMemoryAllocator allocator_=hxMemoryAllocator_Heap>
class hxHashTableNodeString : public hxHashTableSetNode<const char*> {
public:
	// Constructor allocates and duplicates the string key, then initializes the node.
	// - k_: The string key to allocate, duplicate, and initialize the node with.
	HX_CONSTEXPR_FN hxHashTableNodeString(const char* k_)
		: hxHashTableSetNode(hxStringDuplicate(k_, allocator_)) { }

	// Destructor: Frees the allocated string key.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxHashTableNodeString() { hxFree(const_cast<char *>(this->key())); }
};
