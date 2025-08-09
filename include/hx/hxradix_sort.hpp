#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//
// <hx/hxradix_sort.hpp> - hxradix_sort is recommended as an `Θ(n)` sorting
// strategy for any primitive type that is 4-bytes or less. This implementation
// does not cause code bloat and is the fastest sorting algorithm available for
// scalar keys. Radix sort is best when you need real-time guarantees and have a
// massive workload. This is not a toy. It was actually how IBM sorted punch
// cards.

#include <hx/hatchling.h>
#include <hx/hxarray.hpp>

/// `hxradix_sort_base`. Operations that are independent of `hxradix_sort` type.
/// See `hxradix_sort<K, V>` below.
class hxradix_sort_base {
public:
	/// Reserves memory for the internal array to hold at least `size` elements.
	/// - `size` : The number of elements to reserve memory for.
	explicit hxradix_sort_base(size_t size_=0u) : m_array_() { m_array_.reserve(size_); }

	/// Reserves memory for the internal array to hold at least `size` elements.
	/// - `size` : The number of elements to reserve memory for.
	void reserve(size_t size_) { m_array_.reserve(size_); }

	/// Clears the internal array, removing all elements.
	void clear(void) { m_array_.clear(); }

	/// Sorts the internal array using the provided temporary memory allocator
	/// to store histograms.
	/// - `temp_memory` : A hxsystem_allocator_t id.
	void sort(hxsystem_allocator_t temp_memory);

protected:
	// Represents a key-value pair used in radix sorting.
	class hxkey_value_pair_ {
	public:
		// Constructor for an 8-bit key and associated value.
		hxkey_value_pair_(uint8_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

		// Constructor for a 16-bit key and associated value.
		hxkey_value_pair_(uint16_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

		// Constructor for a 32-bit key and associated value.
		hxkey_value_pair_(uint32_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

		// Constructor for a signed 32-bit key and associated value. Adjusts the
		// key to handle signed integers correctly.
		hxkey_value_pair_(int32_t key_, void* val_)
			: m_key_((uint32_t)(key_ ^ 0x80000000)), m_val_(val_) { }

		// Constructor for a floating-point key and associated value. Adjusts
		// the key to handle floating-point sorting correctly.
		hxkey_value_pair_(float key_, void* val_) : m_val_(val_) {
			uint32_t t_;
			::memcpy(&t_, &key_, sizeof t_);
			m_key_ = t_ ^ (uint32_t)(((int32_t)t_ >> 31) | 0x80000000);
		}

		/// Comparison operator for sorting hxkey_value_pair_ objects by key.
		bool operator<(const hxkey_value_pair_& rhs_) const { return m_key_ < rhs_.m_key_; }

		uint32_t m_key_; // The key used for sorting.
		void* m_val_;	 // The associated value.
	};

	hxarray<hxkey_value_pair_> m_array_; // Internal array of key-value pairs.

	hxradix_sort_base(const hxradix_sort_base&) = delete;
	void operator=(const hxradix_sort_base&) = delete;
};

/// hxradix_sort. Sorts an array of value* by keys. K is the key and V the
/// value. Keys of double, int64_t and uint64_t are not supported. To sort an
/// array of doubles with the radix sort it would make sense to sort first by a
/// float key and then run an `hxinsertion_sort` over the nearly sorted data.
/// `hxradix_sort` scales linearly with the byte length of the key whereas
/// `hxinsertion_sort` is Θ(n) on mostly sorted data.
template<typename key_t_, class value_t_>
class hxradix_sort : public hxradix_sort_base {
public:
	/// Type of the key used for sorting.
	typedef key_t_ key_t;
	/// Type of the value associated with the key.
	typedef value_t_ value_t;

	/// `forward_iterator` over Values. Not currently bound to
	/// `std::iterator_traits` or `std::forward_iterator_tag`.
	class const_iterator {
	public:
		// Internal. Constructs a `const_iterator` from an `const
		// hxkey_value_pair_*`.
		const_iterator(const hxkey_value_pair_* it_) : m_ptr_(it_) { }

		/// Constructs an invalid `const_iterator`.
		const_iterator() : m_ptr_(hxnull) { }

		/// Pre-increment operator. Moves the `iterator` to the next element.
		const_iterator& operator++(void) { ++m_ptr_; return *this; }

		/// Post-increment operator. Moves the `iterator` to the next element
		/// and returns the previous state.
		const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }

		/// Equality comparison operator.
		bool operator==(const const_iterator& rhs_) const { return m_ptr_ == rhs_.m_ptr_; }

		/// Inequality comparison operator.
		bool operator!=(const const_iterator& rhs_) const { return m_ptr_ != rhs_.m_ptr_; }

		/// Dereference operator. Returns a reference to the value pointed to by
		/// the iterator.
		const value_t_& operator*(void) const { return *(const value_t_*)m_ptr_->m_val_; }

		/// Arrow operator. Returns a pointer to the value pointed to by the
		/// iterator.
		const value_t_* operator->(void) const { return (const value_t_*)m_ptr_->m_val_; }

	protected:
		const hxkey_value_pair_* m_ptr_; // Internal pointer to the current element.
	};

	/// Iterator that can be cast to a `const_iterator`.
	class iterator : public const_iterator {
	public:
		// Internal. Constructs an iterator from an `hxkey_value_pair_*`.
		iterator(hxkey_value_pair_* it_) : const_iterator(it_) { }

		/// Constructs an invalid iterator.
		iterator(void) { }

		/// Pre-increment operator. Moves the iterator to the next element.
		iterator& operator++(void) { const_iterator::operator++(); return *this; }

		/// Post-increment operator. Moves the iterator to the next element and
		/// returns the previous state.
		iterator operator++(int) { iterator t_(*this); const_iterator::operator++(); return t_; }

		/// Dereference operator. Returns a reference to the value pointed to by
		/// the iterator.
		value_t_& operator*(void) const { return *(value_t_*)this->m_ptr_->m_val_; }

		/// Arrow operator. Returns a pointer to the value pointed to by the
		/// iterator.
		value_t_* operator->(void) const { return (value_t_*)this->m_ptr_->m_val_; }
	};

	explicit hxradix_sort(size_t size_=0u) : hxradix_sort_base(size_) { }

	/// Accesses the value at the specified index (const version).
	const value_t_& operator[](size_t index_) const { return *(value_t_*)m_array_[index_].m_val_; }

	/// Accesses the value at the specified index (non-const version).
	value_t_& operator[](size_t index_) { return *(value_t_*)m_array_[index_].m_val_; }

	/// Adds a key and value pair to the array. Ownership is not taken.
	/// - `key` : The key used for sorting.
	/// - `value` : Pointer to the value associated with the key.
	void insert(key_t_ key_, value_t_* val_) {
		hxassertrelease(!this->full(), "reallocation_disallowed");

		// This radix sort uses void* to avoid template bloat. The casts are not
		// required by the standard, but fix -Wcast-qual for a const value_t_.
		::new(m_array_.emplace_back_unconstructed())
			hxkey_value_pair_(key_, const_cast<void*>((const void*)val_));
	}

	/// Returns a pointer to the value at the specified index (const version).
	const value_t_* get(size_t index_) const { return (value_t_*)m_array_[index_].m_val_; }

	/// Returns a pointer to the value at the specified index (non-const
	/// version).
	value_t_* get(size_t index_) { return (value_t_*)m_array_[index_].m_val_; }

	/// Returns a `const_iterator` to the beginning of the array.
	const_iterator begin(void) const { return const_iterator(m_array_.cbegin()); }

	/// Returns an `iterator` to the beginning of the array.
	iterator begin(void) { return iterator(m_array_.begin()); }

	/// Returns a `const_iterator` to the beginning of the array (const
	/// version).
	const_iterator cbegin(void) const { return const_iterator(m_array_.cbegin()); }

	/// Returns a `const_iterator` to the beginning of the array (non-const
	/// version).
	const_iterator cbegin(void) { return const_iterator(m_array_.cbegin()); }

	/// Returns a `const_iterator` to the end of the array.
	const_iterator end(void) const { return const_iterator(m_array_.cend()); }

	/// Returns an `iterator` to the end of the array.
	iterator end(void) { return iterator(m_array_.end()); }

	/// Returns a `const_iterator` to the end of the array (const version).
	const_iterator cend(void) const { return const_iterator(m_array_.cend()); }

	/// Returns a `const_iterator` to the end of the array (non-const version).
	const_iterator cend(void) { return const_iterator(m_array_.cend()); }

	/// Returns the number of elements in the array.
	size_t size(void) const { return m_array_.size(); }

	/// Returns true if the array is empty, false otherwise.
	bool empty(void) const { return m_array_.empty(); }

	/// Returns true if the array is full, false otherwise.
	bool full(void) const { return m_array_.full(); }
};
