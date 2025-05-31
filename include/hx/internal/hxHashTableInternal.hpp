#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxAllocator.hpp>

// ----------------------------------------------------------------------------
// hxHashTable internals.  See hxHashTable.h instead

// This is a hxHashTable specific subclass of hxAllocator.  C++98 requires this to be
// declared outside hxHashTable.

template<typename Node_, uint32_t HashBits_>
class hxHashTableInternalAllocator_ : public hxAllocator<Node_*, 1u << HashBits_> {
public:
	HX_INLINE hxHashTableInternalAllocator_() {
		::memset(this->getStorage(), 0x00, sizeof(Node_*) * this->getCapacity());
	}
	HX_INLINE uint32_t getHashBits() const { return HashBits_; }
	HX_INLINE void setHashBits(uint32_t bits) {
		hxAssertMsg(bits == HashBits_, "resizing static hash table"); (void)bits;
	}
};

template<typename Node_>
class hxHashTableInternalAllocator_<Node_, hxAllocatorDynamicCapacity>
	: public hxAllocator<Node_*, hxAllocatorDynamicCapacity> {
public:
	HX_INLINE hxHashTableInternalAllocator_() : m_hashBits_(0u) { }

	HX_INLINE uint32_t getHashBits() const {
		hxAssertMsg(m_hashBits_ != 0u, "hash table unallocated");
		return m_hashBits_;
	}

	HX_INLINE void setHashBits(uint32_t bits_) {
		hxAssertMsg(m_hashBits_ == 0u || bits_ == m_hashBits_, "resizing dynamic hash table");
		if (m_hashBits_ == 0u) {
			hxAssertMsg(bits_ > 0u && bits_ <= 31u, "hash bits must be [1..31]");
			m_hashBits_ = bits_;
			this->reserveStorage(1u << bits_);
			::memset(this->getStorage(), 0x00, sizeof(Node_*) * this->getCapacity());
		}
	}

private:
	uint32_t m_hashBits_;
};
