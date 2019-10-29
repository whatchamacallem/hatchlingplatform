
#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hxAllocator.h>

// ----------------------------------------------------------------------------
// hxHashTable internals.  See hxHashTable.h instead

// This is a hxHashTable specific subclass of hxAllocator.  C++98 requires this to be
// declared outside hxHashTable.

template<typename Node, uint32_t HashBits>
class hxHashTableInternalAllocator : public hxAllocator<Node*, 1u << HashBits> {
public:
	HX_INLINE hxHashTableInternalAllocator() { ::memset(this->getStorage(), 0x00, sizeof(Node*) * this->getCapacity()); }
	HX_INLINE HX_CONSTEXPR uint32_t getHashBits() const { return HashBits; }
	HX_INLINE void setHashBits(uint32_t bits) { hxAssertMsg(bits == HashBits, "resizing static hash table"); }
};

template<typename Node>
class hxHashTableInternalAllocator<Node, hxAllocatorDynamicCapacity>
	: public hxAllocator<Node*, hxAllocatorDynamicCapacity> {
public:
	HX_INLINE hxHashTableInternalAllocator() : m_hashBits(0u) { }

	HX_INLINE uint32_t getHashBits() const {
		hxAssertMsg(m_hashBits != 0u, "hash table unallocated");
		return m_hashBits;
	}

	HX_INLINE void setHashBits(uint32_t bits) {
		hxAssertMsg(m_hashBits == 0u || bits == m_hashBits, "resizing dynamic hash table");
		if (m_hashBits == 0u) {
			hxAssertMsg(bits > 0u && bits <= 31u, "hash bits must be [1..31]");
			m_hashBits = bits;
			this->reserveStorage(1u << bits);
			::memset(this->getStorage(), 0x00, sizeof(Node*) * this->getCapacity());
		}
	}

private:
	uint32_t m_hashBits;
};
