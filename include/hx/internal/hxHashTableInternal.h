#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hxAllocator.h>

// ----------------------------------------------------------------------------
// hxHashTable internals.  See hxHashTable.h instead

// This is a hxHashTable specific subclass of hxAllocator.  C++98 requires this to be
// declared outside hxHashTable.

template<typename Node_, uint32_t HashBits_>
class hxHashTableInternalAllocator : public hxAllocator<Node_*, 1u << HashBits_> {
public:
	typedef Node_ Node;
	enum { HashBits = HashBits_ };

	HX_INLINE hxHashTableInternalAllocator() {
		::memset(this->getStorage(), 0x00, sizeof(Node*) * this->getCapacity());
	}
	HX_CONSTEXPR_FN uint32_t getHashBits() const { return HashBits; }
	HX_INLINE void setHashBits(uint32_t bits) {
		hxAssertMsg(bits == HashBits, "resizing static hash table"); (void)bits;
	}
};

template<typename Node_>
class hxHashTableInternalAllocator<Node_, hxAllocatorDynamicCapacity>
	: public hxAllocator<Node_*, hxAllocatorDynamicCapacity> {
public:
	typedef Node_ Node;
	enum { HashBits = hxAllocatorDynamicCapacity };

	HX_INLINE hxHashTableInternalAllocator() : m_hashBits(0u) { }

	HX_INLINE uint32_t getHashBits() const {
		hxAssertMsg(m_hashBits != 0u, "hash table unallocated");
		return m_hashBits;
	}

	HX_INLINE void setHashBits(uint32_t bits_) {
		hxAssertMsg(m_hashBits == 0u || bits_ == m_hashBits, "resizing dynamic hash table");
		if (m_hashBits == 0u) {
			hxAssertMsg(bits_ > 0u && bits_ <= 31u, "hash bits must be [1..31]");
			m_hashBits = bits_;
			this->reserveStorage(1u << bits_);
			::memset(this->getStorage(), 0x00, sizeof(Node*) * this->getCapacity());
		}
	}

private:
	uint32_t m_hashBits;
};
