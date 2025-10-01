#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxallocator.hpp Similar to `std::allocator`. Allows for static or
/// dynamic allocation.

#include "hatchling.h"

/// A capacity value that allows for dynamic allocation.
#define hxallocator_dynamic_capacity 0u

/// `hxallocator<1+>` - Provides static allocation when capacity is greater than
/// zero.
template<typename T_, size_t fixed_capacity_>
class hxallocator {
public:
	typedef T_ value_t;

	/// Template specialization below should have been selected.
	static_assert(fixed_capacity_ > 0u, "Fixed capacity must be > 0.");

	/// Initializes memory to `0xbc` when `HX_RELEASE < 1`.
	hxallocator() {
#if (HX_RELEASE) < 1
		::memset(m_data_, 0xbc, sizeof m_data_);
#endif
	}

	/// Returns the number of elements of `T` allocated.
	constexpr size_t capacity(void) const { return fixed_capacity_; }

	/// Returns a reference to a const and potentially uninitialized array of `T`.
	const T_ (&data() const)[fixed_capacity_] {
		return *reinterpret_cast<const T_(*)[fixed_capacity_]>(m_data_ + 0);
	}

	/// Returns a reference to a potentially uninitialized array of `T`.
	T_ (&data())[fixed_capacity_] {
		return *reinterpret_cast<T_(*)[fixed_capacity_]>(m_data_ + 0);
	}

protected:
	/// Used to ensure initial capacity as `reserve_storage` will not reallocate.
	/// Provided for interface compatibility with the dynamic allocator.
	/// - `size` : The number of elements of type `T` to ensure are available.
	/// - `allocator` : Ignored.
	/// - `alignment` : The alignment of the allocator is checked against this.
	constexpr void reserve_storage(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		(void)size_; (void)allocator_; (void)alignment_;
		hxassertmsg(size_ <= fixed_capacity_, "overflowing_fixed_capacity Buffer overflow.");
		hxassertmsg(((alignment_ - 1u) & (uintptr_t)this) == 0u,
			"alignment_error Static allocation misaligned.");
	}

private:
	hxallocator(const hxallocator&) = delete;
	void operator=(const hxallocator&) = delete;

	alignas(T_) char m_data_[fixed_capacity_ * sizeof(T_)];
};

/// `hxallocator<0>` - Capacity is set by first call to `reserve_storage` and may
/// not be extended. May be moved using `hxswap_memcpy`.
template<typename T_>
class hxallocator<T_, hxallocator_dynamic_capacity> {
public:
	typedef T_ value_t;

	/// Does not allocate until `reserve_storage` is called.
	hxallocator(void) {
		m_data_ = hxnull;
		m_capacity_ = 0u;
	}

	/// Calls `hxfree` with any allocated memory.
	~hxallocator(void) {
		if(m_data_) {
			m_capacity_ = 0u;
			hxfree(m_data_);
			m_data_ = hxnull;
		}
	}

	/// Returns the number of elements of `T` allocated.
	size_t capacity(void) const { return m_capacity_; }

	/// Returns a const array of `T`.
	const T_* data(void) const { return m_data_; }

	/// Returns an array of `T`.
	T_* data(void) { return m_data_; }

protected:
	/// Capacity is set by first call to reserve_storage and may not be extended.
	/// - `size` : The number of elements of type `T` to allocate space for.
	/// - `allocator` : The memory manager ID to use for allocation (default: `hxsystem_allocator_current`)
	/// - `alignment` : The alignment to for the allocation. (default: `HX_ALIGNMENT`)
	void reserve_storage(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		if(size_ <= m_capacity_) { return; }
		hxassertrelease(m_capacity_ == 0, "reallocation_disallowed");
		m_data_ = (T_*)hxmalloc_ext(sizeof(T_) * size_, allocator_, alignment_);
		m_capacity_ = size_;
	}

private:
	hxallocator(const hxallocator&) = delete;
	void operator=(const hxallocator&) = delete;

	size_t m_capacity_;
	T_* m_data_;
};
