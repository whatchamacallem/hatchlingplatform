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
	HX_INLINE hxAllocator() {
		if ((HX_RELEASE) < 1) {
			::memset(m_allocator, 0xcd, sizeof m_allocator);
		}
	}

	// Used to ensure initial capacity as reserveStorage() will not reallocate.
	// sz_: The number of elements of type T to allocate.
	HX_INLINE void reserveStorage(size_t size_) {
		hxAssertRelease(size_ <= Capacity_, "allocator overflowing fixed capacity."); (void)size_;
	}

	// Returns the number of elements of T allocated.
	HX_CONSTEXPR_FN size_t getCapacity() const { return Capacity_; }

	// Returns const array of T.
	HX_INLINE const T* getStorage() const { return reinterpret_cast<const T*>(m_allocator + 0); }

	// Returns array of T.
	HX_INLINE T* getStorage() { return reinterpret_cast<T*>(m_allocator + 0); }

private:
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
	HX_INLINE hxAllocator() {
		m_allocator = hxnull;
		m_capacity = 0;
	}

	// Calls hxFree() with any allocated memory.
		HX_INLINE ~hxAllocator() {
		if (m_allocator) {
			m_capacity = 0;
			hxFree(m_allocator);
			m_allocator = hxnull;
		}
	}

	// Capacity is set by first call to reserveStorage() and may not be extended.
	// sz_: The number of elements of type T to allocate.
	HX_INLINE void reserveStorage(size_t sz_) {
		if (sz_ <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_allocator = (T*)hxMalloc(sizeof(T) * sz_); // Never fails.
		m_capacity = sz_;
	}

	// Use hxArray::getAllocator() to access extended allocation semantics.
	// sz_: The number of elements of type T to allocate.
	// alId_: The memory manager ID to use for allocation (default: hxMemoryManagerId_Current).
	// alignmentMask_: The alignment mask to apply to the allocation (default: HX_ALIGNMENT_MASK).
	HX_INLINE void reserveStorageExt(size_t sz_,
			hxMemoryManagerId alId_=hxMemoryManagerId_Current,
			uintptr_t alignmentMask_=HX_ALIGNMENT_MASK) {
		if (sz_ <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_allocator = (T*)hxMallocExt(sizeof(T) * sz_, alId_, alignmentMask_);
		m_capacity = sz_;
	}

	// Returns the number of elements of T allocated.
	HX_INLINE size_t getCapacity() const { return m_capacity; }

	// Returns const array of T.
	HX_INLINE const T* getStorage() const { return m_allocator; }

	// Returns array of T.
	HX_INLINE T* getStorage() { return m_allocator; }

private:
	hxAllocator(const hxAllocator&); // = delete
	void operator=(const hxAllocator&); // = delete

	size_t m_capacity;
	T* m_allocator;
};
