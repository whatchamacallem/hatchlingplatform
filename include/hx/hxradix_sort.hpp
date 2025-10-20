#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxradix_sort.hpp Radix sort is recommended as an Θ(n) sorting
/// strategy for any fundamental type that is 32 bits or less. This
/// implementation does not cause code bloat and is the fastest sorting
/// algorithm available for scalar keys. Radix sort is best when you need
/// real-time guarantees and have a massive workload. This is not a toy; IBM
/// actually used it to sort punch cards.
///
/// For example:
/// ```cpp
///   hxarray<hxradix_sort_key<key_t, example_t>> rs; rs.reserve(size);
///   for(uint32_t i = size; i--;) {
///     rs.push_back(hxradix_sort_key<key_t, example_t>(x[i].id, &x[i]));
///   }
///   hxradix_sort(rs.begin(), rs.end());
/// ```

#include "hatchling.h"

/// A key-value pair used with `hxradix_sort`. Only 32-bit or smaller fixed size
/// types from `<stdint.h>` are supported for `key_t`. Stores a pointer to
/// `value_t` only.
template<typename key_t_, typename value_t_>
class hxradix_sort_key {
public:
	/// Constructs from the required `key_t` type and `value_t*` type.
	hxradix_sort_key(key_t_ key_, value_t_ value_) { this->set_(key_, value_); }

	/// Sets from the required `key_t` type and `value_t*` type.
	void set(key_t_ key_, value_t_ value_) { this->set_(key_, value_); }

	/// Returns the stored `value_t`.
	value_t_ get_value(void) const { return m_value_; }

	/// Returns the stored `value_t*`.
	value_t_ get_value(void) { return m_value_; }

	/// Comparison operator for comparison sorting `hxradix_sort_key` objects
	/// by key as a fallback for short arrays.
	bool operator<(hxradix_sort_key x_) const { return m_key_ < x_.m_key_; }

	/// A version of the key that may have been modified to work as a `uint32_t`.
	uint32_t get_modified_key(void) const { return m_key_; }

private:
	// Required by the implementation of hxradix_sort_void/hxradix_sort_void11.
	// Use a pointer or small struct for value_t. Otherwise the implementation
	// does too much copying.
	static_assert(sizeof(void*) >= sizeof(value_t_), "value_t size too big.");

	// Internal. Possible conversion routines.
	void set_(int8_t key_, value_t_ value_) { this->set_(static_cast<int32_t>(key_), value_); }
	void set_(uint8_t key_, value_t_ value_) { m_key_=key_; m_value_=value_; }
	void set_(int16_t key_, value_t_ value_) { this->set_(static_cast<int32_t>(key_), value_); }
	void set_(uint16_t key_, value_t_ value_) { m_key_=key_; m_value_=value_; }
	void set_(int32_t key_, value_t_ value_) {
		m_key_ = static_cast<uint32_t>(key_) ^ 0x80000000u;
		m_value_ = value_;
	}
	void set_(int64_t key_, value_t_ value_) = delete; // Not supported.
	void set_(uint64_t key_, value_t_ value_) = delete; // Not supported.
	void set_(uint32_t key_, value_t_ value_) { m_key_=key_; m_value_=value_; }
	void set_(float key_, value_t_ value_) {
		// Reinterpret a float as a signed int in order to use a sign extending
		// right shift before switching to well-defined unsigned bit ops.
		int32_t t_ = 0; ::memcpy(&t_, &key_, sizeof t_);
		m_key_ = static_cast<uint32_t>(t_) ^ (static_cast<uint32_t>(t_ >> 31) | 0x80000000u);
		m_value_ = value_;
	}
	void set_(double key_, value_t_ value_) = delete; // Not supported.

	// The key used for sorting. May not be preserved in a usable form.
	uint32_t m_key_;
	union {
		// The value associated with the key. Reinterpreted as a void* during
		// sorting.
		value_t_ m_value_;

		// Forces m_value_ to occupy the same space as a void*.
		void* m_void_ptr_;
	};
};

/// Internal. Used to share the implementation with all pointer types.
using hxradix_sort_key_void = hxradix_sort_key<uint32_t, void*>;

/// Internal. The shared implementation used with all pointer types when sorting
/// using 8-bit digits.
hxattr_nonnull(1,2) hxattr_hot
void hxradix_sort_void(hxradix_sort_key_void* begin_, hxradix_sort_key_void* end_);

/// Internal. The shared implementation used with all pointer types when sorting
/// using 11-bit digits.
hxattr_nonnull(1,2) hxattr_hot
void hxradix_sort_void11(hxradix_sort_key_void* begin_, hxradix_sort_key_void* end_);

/// Sorts an array of `value_t*` by `key_t` using 8-bit digits. `key_t` is the
/// sort key and `value_t` the value being sorted. Keys of `double`, `int64_t`,
/// and `uint64_t` are not supported. `hxradix_sort` scales linearly with the
/// byte-length of the key, whereas `hxinsertion_sort` is Θ(n) on mostly sorted
/// data.
template<typename key_t_, typename value_t_> hxattr_nonnull(1,2) hxattr_hot
void hxradix_sort(hxradix_sort_key<key_t_, value_t_>* begin_, hxradix_sort_key<key_t_, value_t_>* end_) {
	hxradix_sort_void(reinterpret_cast<hxradix_sort_key_void*>(begin_), reinterpret_cast<hxradix_sort_key_void*>(end_));
}

/// Sorts an array of `value_t*` by `key_t` using 11-bit digits. `key_t` is the
/// sort key and `value_t` the value being sorted. Keys of `double`, `int64_t`,
/// and `uint64_t` are not supported. `hxradix_sort` scales linearly with the
/// byte-length of the key, whereas `hxinsertion_sort` is Θ(n) on mostly sorted
/// data.
template<typename key_t_, typename value_t_> hxattr_nonnull(1,2) hxattr_hot
void hxradix_sort11(hxradix_sort_key<key_t_, value_t_>* begin_, hxradix_sort_key<key_t_, value_t_>* end_) {
	hxradix_sort_void11(reinterpret_cast<hxradix_sort_key_void*>(begin_), reinterpret_cast<hxradix_sort_key_void*>(end_));
}
