#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxAllocator.hpp>

// hxHashTable internals. See hxHashTable.h instead

// This is a hxHashTable specific subclass of hxAllocator. C++98 requires this to be
// declared outside hxHashTable. The table has a size of 2^TableSizeBits_.

template<typename Node_, uint32_t TableSizeBits_>
class hxHashTableInternalAllocator_ : public hxAllocator<Node_*, 1u << TableSizeBits_> {
public:
	HX_CONSTEXPR_FN hxHashTableInternalAllocator_() {
		::memset(this->data(), 0x00, sizeof(Node_*) * this->capacity());
	}
	HX_CONSTEXPR_FN uint32_t getTableSizeBits() const { return TableSizeBits_; }
	HX_CONSTEXPR_FN void setTableSizeBits(uint32_t bits) {
		hxAssertMsg(bits == TableSizeBits_, "resizing static hash table"); (void)bits;
	}
};

template<typename Node_>
class hxHashTableInternalAllocator_<Node_, hxAllocatorDynamicCapacity>
	: public hxAllocator<Node_*, hxAllocatorDynamicCapacity> {
public:
	HX_CONSTEXPR_FN hxHashTableInternalAllocator_() : m_tableSizeBits_(0u) { }

	HX_CONSTEXPR_FN uint32_t getTableSizeBits() const {
		hxAssertMsg(m_tableSizeBits_ != 0u, "hash table unallocated");
		return m_tableSizeBits_;
	}

	HX_CONSTEXPR_FN void setTableSizeBits(uint32_t bits_) {
		hxAssertMsg(m_tableSizeBits_ == 0u || bits_ == m_tableSizeBits_, "resizing dynamic hash table");
		if (m_tableSizeBits_ == 0u) {
			hxAssertMsg(bits_ > 0u && bits_ <= 31u, "hash bits must be [1..31]");
			m_tableSizeBits_ = bits_;
			this->reserveStorage(1u << bits_);
			::memset(this->data(), 0x00, sizeof(Node_*) * this->capacity());
		}
	}

private:
	uint32_t m_tableSizeBits_;
};
