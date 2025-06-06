#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxAllocator.  Similar to std::allocator.  Allows for static or dynamic allocation.

// ----------------------------------------------------------------------------
// hxAllocator<1+>
//
// Provides static allocation when capacity is greater than zero.

template<typename T_, size_t Capacity_>
class hxAllocator {
public:
	typedef T_ T;

	// Template specialization below should have been selected.
	HX_STATIC_ASSERT(Capacity_ > 0u, "Capacity_ > 0");

	// Initializes memory to 0xcd when HX_RELEASE < 1.
	HX_CONSTEXPR_FN hxAllocator() {
		if ((HX_RELEASE) < 1) {
			::memset(m_allocator, 0xcd, sizeof m_allocator);
		}
	}

	// Used to ensure initial capacity as reserveStorage() will not reallocate.
	// size_: The number of elements of type T to allocate.
	HX_CONSTEXPR_FN void reserveStorage(size_t size_) {
		hxAssertRelease(size_ <= Capacity_, "allocator overflowing fixed capacity."); (void)size_;
	}

	// Returns the number of elements of T allocated.
	HX_CONSTEXPR_FN size_t getCapacity() const { return Capacity_; }

	// Returns const array of T.
	HX_CONSTEXPR_FN const T* getStorage() const { return reinterpret_cast<const T*>(m_allocator + 0); }

	// Returns array of T.
	HX_CONSTEXPR_FN T* getStorage() { return reinterpret_cast<T*>(m_allocator + 0); }

private:
	// *** The static allocator does not support swapping allocations or
	// assignments from temporaries. ***
	HX_CONSTEXPR_FN void swap(hxAllocator& rhs) HX_DELETE_FN;

	// Consistently show m_capacity in debugger.
	static const size_t m_capacity = Capacity_;

	// Using union to implement alignas(char *).
	union {
		// Char arrays are the least likely to encounter undefined behavior.
		char m_allocator[m_capacity * sizeof(T)];
		char* m_charPointerAlign; // char pointers have the strictest alignment requirement.
	};
};

// ----------------------------------------------------------------------------
// hxAllocator<0>
//
// Capacity is set by first call to reserveStorage() and may not be extended.

#define hxAllocatorDynamicCapacity 0u

template<typename T_>
class hxAllocator<T_, hxAllocatorDynamicCapacity> {
public:
	typedef T_ T;

	// Does not allocate until reserveStorage() is called.
	HX_CONSTEXPR_FN hxAllocator() {
		m_allocator = hxnull;
		m_capacity = 0;
	}

	// Calls hxFree() with any allocated memory.
#if HX_CPLUSPLUS >= 202002L
	constexpr
#endif
	~hxAllocator() {
		if (m_allocator) {
			m_capacity = 0;
			hxFree(m_allocator);
			m_allocator = hxnull;
		}
	}

	// Capacity is set by first call to reserveStorage and may not be extended.
	// size_: The number of elements of type T to allocate space for.
	// allocator_: The memory manager ID to use for allocation (default: hxMemoryAllocator_Current)
	// alignment_: The alignment to for the allocation. (default: HX_ALIGNMENT)
	HX_CONSTEXPR_FN void reserveStorage(size_t size_,
			hxMemoryAllocator allocator_=hxMemoryAllocator_Current,
			uintptr_t alignment_=HX_ALIGNMENT) {
		if (size_ <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_allocator = (T*)hxMallocExt(sizeof(T) * size_, allocator_, alignment_);
		m_capacity = size_;
	}

	// Returns the number of elements of T allocated.
	HX_CONSTEXPR_FN size_t getCapacity() const { return m_capacity; }

	// Returns const array of T.
	HX_CONSTEXPR_FN const T* getStorage() const { return m_allocator; }

	// Returns array of T.
	HX_CONSTEXPR_FN T* getStorage() { return m_allocator; }

	// Swap.  Only works with Capacity_ == hxAllocatorDynamicCapacity
	HX_CONSTEXPR_FN void swap(hxAllocator& rhs) {
		hxswap(m_capacity, rhs.m_capacity);
		hxswap(m_allocator, rhs.m_allocator);
	}

private:
	hxAllocator(const hxAllocator&) HX_DELETE_FN;
	void operator=(const hxAllocator&) HX_DELETE_FN;

	size_t m_capacity;
	T* m_allocator;
};
