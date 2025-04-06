#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxHashTable.hpp>

// Usable implementations of the hxHashTable Node template parameter.

// ----------------------------------------------------------------------------
// hxHashTableNodeInteger. Specialization of hxHashTableNodeBase for integer types.
// See documentation of hxHashTableNodeBase for interface documentation.  Uses the
// well studied hash multiplier taken from Linux's hash.h

template<typename Key_>
class hxHashTableNodeInteger : public hxHashTableNodeBase<Key_> {
public:
	typedef hxHashTableNodeBase<Key_> Base;

	// Constructor: Initializes the node with a key and optional hash value.
	HX_INLINE hxHashTableNodeInteger(const Key_& k_, uint32_t h_=0u)
		: Base(k_) { (void)h_; }

	// Returns the hash value of the key.
	HX_INLINE uint32_t hash() const { return hash(this->key); }

	// Static method to compute the hash value for a given key.
	HX_INLINE static uint32_t hash(const Key_& key_) {
		return (uint32_t)key_ * (uint32_t)0x61C88647u;
	}

	// Static method to compare two keys for equality.
	HX_INLINE static bool keyEqual(const hxHashTableNodeInteger& lhs_, const Key_& rhs_, uint32_t rhsHash_) {
		(void)rhsHash_; return lhs_.key == rhs_;
	}
};

// ----------------------------------------------------------------------------
// hxHashTableNodeStringLiteral. Specialization of hxHashTableNodeBase for
// static C strings.  Intended for use with string literals. See documentation of
// hxHashTableNodeBase for interface documentation.

class hxHashTableNodeStringLiteral : public hxHashTableNodeBase<const char*> {
public:
	typedef hxHashTableNodeBase<const char*> Base;

	// Constructor: Initializes the node with a string key and computes its hash.
	HX_INLINE hxHashTableNodeStringLiteral(const char* k_)
		: Base(k_), m_hash(hash(k_)) { }

	// Constructor: Initializes the node with a string key and a precomputed hash.
	HX_INLINE hxHashTableNodeStringLiteral(const char* k_, uint32_t hash_)
		: Base(k_), m_hash(hash_) { }

	// Returns the precomputed hash value of the key.
	HX_INLINE uint32_t hash() const {
		return m_hash;
	}

	// Static method to compute the hash value for a given string key using FNV-1a.
	HX_INLINE static uint32_t hash(const char*const& key_) {
		const char* k_ = key_;
		uint32_t x_ = (uint32_t)0x811c9dc5; // FNV-1a string hashing.
		while (*k_ != '\0') {
			x_ ^= (uint32_t)*k_++;
			x_ *= (uint32_t)0x01000193;
		}
		return x_;
	}

	// Static method to compare two string keys for equality, including hash comparison.
	HX_INLINE static bool keyEqual(const hxHashTableNodeStringLiteral& lhs_, const Key& rhs_, uint32_t rhsHash_) {
		return lhs_.hash() == rhsHash_ && ::strcmp(lhs_.key, rhs_) == 0;
	}

private:
	uint32_t m_hash; // Stores the precomputed hash value of the key.
};

// ----------------------------------------------------------------------------
// hxHashTableNodeString. Specialization of hxHashTableNodeBase for C strings.
// Allocates a copy, resulting in a string pool per-hash table.  See documentation
// of hxHashTableNodeBase for interface documentation.

template <hxMemoryManagerId id_=hxMemoryManagerId_Heap>
class hxHashTableNodeString : public hxHashTableNodeStringLiteral {
public:
	// Constructor: Allocates and duplicates the string key, then initializes the node.
	HX_INLINE hxHashTableNodeString(const char* k_)
		: hxHashTableNodeStringLiteral(hxStringDuplicate(k_, id_)) { }

	// Constructor: Allocates and duplicates the string key with a precomputed hash.
	HX_INLINE hxHashTableNodeString(const char* k_, uint32_t hash_)
		: hxHashTableNodeStringLiteral(hxStringDuplicate(k_, id_), hash_) { }

	// Destructor: Frees the allocated string key.
	HX_INLINE ~hxHashTableNodeString() { hxFree(const_cast<char*>(key)); }
};
