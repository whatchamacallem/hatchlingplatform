#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"

// hxAllocator.  Similar to std::allocator.  Allows for static or dynamic allocation.

// ----------------------------------------------------------------------------
// hxAllocator<1+>
//
// Static instantiation for known capacities.

template<typename T, uint32_t Capacity>
class hxAllocator {
public:
	static_assert(Capacity > 0u, "Capacity > 0");
	HX_INLINE hxAllocator() {
		if ((HX_RELEASE) < 1) {
			::memset(m_allocator, 0xcd, sizeof m_allocator);
		}
	}

	// Because reserveStorage() will not actually reallocate it is also used to ensure initial capacity.
	HX_INLINE void reserveStorage(uint32_t size) { hxAssertRelease(size <= Capacity, "allocator overflowing fixed capacity."); }
	HX_INLINE uint32_t getCapacity() const { return Capacity; }
	HX_INLINE const T* getStorage() const { return reinterpret_cast<const T*>(m_allocator + 0); }
	HX_INLINE T* getStorage() { return reinterpret_cast<T*>(m_allocator + 0); }

private:
	enum { m_capacity = Capacity };
	union {
		char m_allocator[Capacity * sizeof(T)];
		char* m_align;
	};
};

// ----------------------------------------------------------------------------
// hxAllocator<0>
//
// Capacity is set by first call to reserveStorage() and may not be extended.
#define hxAllocatorDynamicCapacity 0u

template<typename T>
class hxAllocator<T, hxAllocatorDynamicCapacity> {
public:
	HX_INLINE hxAllocator() {
		m_allocator = 0;
		m_capacity = 0;
	}

	HX_INLINE ~hxAllocator() {
		if (m_allocator) {
			m_capacity = 0;
			hxFree(m_allocator);
			m_allocator = 0;
		}
	}

	// Capacity is set by first call to reserveStorage() and may not be extended.
	HX_INLINE void reserveStorage(uint32_t sz) {
		if (sz <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_allocator = (T*)hxMalloc(sizeof(T) * sz); // Never fails.
		m_capacity = sz;
		if ((HX_RELEASE) < 1) {
			::memset(m_allocator, 0xcd, sizeof(T) * sz);
		}
	}

	// Use hxArray::get_allocator() to access extended allocation semantics.
	HX_INLINE void reserveStorageExt(uint32_t sz, hxMemoryManagerId alId=hxMemoryManagerId_Current, uintptr_t alignmentMask=HX_ALIGNMENT_MASK) {
		if (sz <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_allocator = (T*)hxMallocExt(sizeof(T) * sz, alId, alignmentMask);
		m_capacity = sz;
		if ((HX_RELEASE) < 1) {
			::memset(m_allocator, 0xcd, sizeof(T) * sz);
		}
	}

	HX_INLINE uint32_t getCapacity() const { return m_capacity; }
	HX_INLINE const T* getStorage() const { return m_allocator; }
	HX_INLINE T* getStorage() { return m_allocator; }

private:
	hxAllocator(const hxAllocator&); // = delete
	void operator=(const hxAllocator&); // = delete

	uint32_t m_capacity;
	T* m_allocator;
};
