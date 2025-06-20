#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxallocator.hpp>

// hxhash_table internals. See hxhash_table.h instead

// This is a hxhash_table specific subclass of hxallocator. C++98 requires this to be
// declared outside hxhash_table. The table has a size of 2^Table_size_bits_.

template<typename Node_, uint32_t Table_size_bits_>
class hxhash_table_internal_allocator_ : public hxallocator<Node_*, 1u << Table_size_bits_> {
public:
	HX_CONSTEXPR_FN hxhash_table_internal_allocator_() {
		::memset(this->data(), 0x00, sizeof(Node_*) * this->capacity());
	}
	HX_CONSTEXPR_FN uint32_t get_table_size_bits() const { return Table_size_bits_; }
	HX_CONSTEXPR_FN void set_table_size_bits(uint32_t bits) {
		hxassertmsg(bits == Table_size_bits_, "resizing static hash table"); (void)bits;
	}
};

template<typename Node_>
class hxhash_table_internal_allocator_<Node_, hxallocator_dynamic_capacity>
	: public hxallocator<Node_*, hxallocator_dynamic_capacity> {
public:
	HX_CONSTEXPR_FN hxhash_table_internal_allocator_() : m_table_size_bits_(0u) { }

	HX_CONSTEXPR_FN uint32_t get_table_size_bits() const {
		hxassertmsg(m_table_size_bits_ != 0u, "hash table unallocated");
		return m_table_size_bits_;
	}

	HX_CONSTEXPR_FN void set_table_size_bits(uint32_t bits_) {
		hxassertmsg(m_table_size_bits_ == 0u || bits_ == m_table_size_bits_, "resizing dynamic hash table");
		if (m_table_size_bits_ == 0u) {
			hxassertmsg(bits_ > 0u && bits_ <= 31u, "hash bits must be [1..31]");
			m_table_size_bits_ = bits_;
			this->reserve_storage(1u << bits_);
			::memset(this->data(), 0x00, sizeof(Node_*) * this->capacity());
		}
	}

private:
	uint32_t m_table_size_bits_;
};
