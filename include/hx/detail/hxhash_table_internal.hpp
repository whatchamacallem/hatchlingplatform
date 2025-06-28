#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxallocator.hpp>

// hxhash_table internals. See hxhash_table.h instead

// This is a hxhash_table specific subclass of hxallocator. C++98 requires this to be
// declared outside hxhash_table. The table has a size of 2^table_size_bits_.

namespace hxdetail_ {

template<typename node_t_, uint32_t table_size_bits_>
class hxhash_table_internal_allocator_ : public hxallocator<node_t_*, 1u << table_size_bits_> {
public:
	hxconstexpr_fn hxhash_table_internal_allocator_(void) {
		::memset(this->data(), 0x00, sizeof(node_t_*) * this->capacity());
	}
	hxconstexpr_fn uint32_t get_table_size_bits(void) const { return table_size_bits_; }
	hxconstexpr_fn void set_table_size_bits(uint32_t bits) {
		hxassertmsg(bits == table_size_bits_, "fixed_capacity"); (void)bits;
	}
};

template<typename node_t_>
class hxhash_table_internal_allocator_<node_t_, hxallocator_dynamic_capacity>
	: public hxallocator<node_t_*, hxallocator_dynamic_capacity> {
public:
	hxconstexpr_fn hxhash_table_internal_allocator_() : m_table_size_bits_(0u) { }

	hxconstexpr_fn uint32_t get_table_size_bits(void) const {
		hxassertmsg(m_table_size_bits_ != 0u, "container_unallocated");
		return m_table_size_bits_;
	}

	hxconstexpr_fn void set_table_size_bits(uint32_t bits_) {
		hxassertmsg(m_table_size_bits_ == 0u || bits_ == m_table_size_bits_, "reallocation_disallowed");
		if (m_table_size_bits_ == 0u) {
			hxassertmsg(bits_ > 0u && bits_ <= 31u, "bad_hash_bits %d", (int)bits_);
			m_table_size_bits_ = bits_;
			this->reserve_storage(1u << bits_);
			::memset(this->data(), 0x00, sizeof(node_t_*) * this->capacity());
		}
	}

private:
	uint32_t m_table_size_bits_;
};

} // hxdetail_
using namespace hxdetail_;
