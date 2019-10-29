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
			::memset(m_storage, 0xcd, sizeof m_storage);
		}
	}

	// Because reserveStorage() will not actually reallocate it is also used to ensure initial capacity.
	HX_INLINE void reserveStorage(uint32_t size) { hxAssertRelease(size <= Capacity, "allocator overflowing fixed capacity."); }
	HX_INLINE uint32_t getCapacity() const { return Capacity; }
	HX_INLINE const T* getStorage() const { return (T*)(uint64_t*)m_storage; }
	HX_INLINE T* getStorage() { return (T*)(uint64_t*)m_storage; }

private:
	// Force 64-bit alignment to avoid undefined behavior when storing 64-bit pointers.
	// This introduces type punning, which is also undefined behavior.  Doesn't seem
	// like there is an ideal solution without alignas.  However this is passing Clang's
	// undefined behavior sanitizer and runs correct with -O2 under both Clang and GCC.
	uint64_t m_storage[(Capacity * sizeof(T) + 7) / 8];
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
		m_storage = 0;
		m_capacity = 0;
	}

	HX_INLINE ~hxAllocator() {
		if (m_storage) {
			m_capacity = 0;
			hxFree(m_storage);
			m_storage = 0;
		}
	}

	// Capacity is set by first call to reserveStorage() and may not be extended.
	HX_INLINE void reserveStorage(uint32_t sz) {
		if (sz <= m_capacity) { return; }
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_storage = (T*)hxMalloc(sizeof(T) * sz); // Never fails.
		m_capacity = sz;
		if ((HX_RELEASE) < 1) {
			::memset(m_storage, 0xcd, sizeof(T) * sz);
		}
	}

	// Use hxArray::get_allocator() to access extended allocation semantics.
	HX_INLINE void reserveStorageExt(uint32_t sz, hxMemoryManagerId alId=hxMemoryManagerId_Current, uintptr_t alignmentMask=HX_ALIGNMENT_MASK) {
		hxAssertRelease(m_capacity == 0, "allocator reallocation disallowed.");
		m_storage = (T*)hxMallocExt(sizeof(T) * sz, alId, alignmentMask);
		m_capacity = sz;
		if ((HX_RELEASE) < 1) {
			::memset(m_storage, 0xcd, sizeof(T) * sz);
		}
	}

	HX_INLINE uint32_t getCapacity() const { return m_capacity; }
	HX_INLINE const T* getStorage() const { return m_storage; }
	HX_INLINE T* getStorage() { return m_storage; }

private:
	hxAllocator(const hxAllocator&); // = delete
	void operator=(const hxAllocator&); // = delete

	uint32_t m_capacity;
	T* m_storage;
};
