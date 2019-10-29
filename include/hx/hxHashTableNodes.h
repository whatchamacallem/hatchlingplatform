#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hxHashTable.h>

// Usable implementations of the hxHashTable Node template parameter.

// ----------------------------------------------------------------------------
// hxHashTableNodeInteger. Specialization of hxHashTableNodeBase for integer types.
// See documentation of hxHashTableNodeBase for interface documentation.  Uses the
// well studied hash multiplier taken from Linux's hash.h

template<typename Key>
class hxHashTableNodeInteger : public hxHashTableNodeBase<Key> {
public:
	typedef hxHashTableNodeBase<Key> Base;
	HX_INLINE hxHashTableNodeInteger(const Key& k, uint32_t hash=0u) : Base(k) { (void)hash; }
	HX_INLINE uint32_t hash() const { return hash(this->key); }
	HX_INLINE static uint32_t hash(const Key& key) { return (uint32_t)key * (uint32_t)0x61C88647u; }
	HX_INLINE static bool keyEqual(const hxHashTableNodeInteger& lhs, const Key& rhs, uint32_t rhsHash) {
		return lhs.key == rhs; (void)rhsHash;
	}
};

// ----------------------------------------------------------------------------
// hxHashTableNodeStringLiteral. Specialization of hxHashTableNodeBase for
// static C strings.  Intended for use with string literals. See documentation of
// hxHashTableNodeBase for interface documentation.

class hxHashTableNodeStringLiteral : public hxHashTableNodeBase<const char*> {
public:
	typedef hxHashTableNodeBase<const char*> Base;
	HX_INLINE hxHashTableNodeStringLiteral(const char* k) : Base(k), m_hash(hash(k)) { }
	HX_INLINE hxHashTableNodeStringLiteral(const char* k, uint32_t hash) : Base(k), m_hash(hash) { }
	HX_INLINE uint32_t hash() const {
		return m_hash;
	}
	HX_INLINE static uint32_t hash(const char*const& key) {
		const char* k = key;
		uint32_t x = (uint32_t)0x811c9dc5; // FNV-1a string hashing.
		while (*k != '\0') {
			x ^= (uint32_t)*k++;
			x *= (uint32_t)0x01000193;
		}
		return x;
	}
	HX_INLINE static bool keyEqual(const hxHashTableNodeStringLiteral& lhs, const Key& rhs, uint32_t rhsHash) {
		return lhs.m_hash == rhsHash && ::strcmp(lhs.key, rhs) == 0;
	}
private:
	uint32_t m_hash;
};

// ----------------------------------------------------------------------------
// hxHashTableNodeString. Specialization of hxHashTableNodeBase for C strings.
// Allocates a copy, resulting in a string pool per-hash table.  See documentation
// of hxHashTableNodeBase for interface documentation.

template <hxMemoryManagerId id=hxMemoryManagerId_Heap>
class hxHashTableNodeString : public hxHashTableNodeStringLiteral {
public:
	HX_INLINE hxHashTableNodeString(const char* k) : hxHashTableNodeStringLiteral(hxStringDuplicate(k, id)) { }
	HX_INLINE hxHashTableNodeString(const char* k, uint32_t hash) : hxHashTableNodeStringLiteral(hxStringDuplicate(k, id), hash) { }
	HX_INLINE ~hxHashTableNodeString() { hxFree((void*)key); }
};
