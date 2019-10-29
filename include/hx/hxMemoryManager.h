#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#ifndef HATCHLING_H
#error #include <hx/hatchling.h>
#endif

#if __cplusplus
#include <new>

extern "C" {
#endif

// ----------------------------------------------------------------------------
// Memory Manager C API
//
// Memory allocators are selected using an id.
//
// Alignment is specified using a mask of those LSB bits that must be 0.  Which
// is a value 1 less than the actual power of two alignment.  

// The default alignment HX_ALIGNMENT_MASK allows for storing char pointers.
// This alignment should work for most types except SIMD vectors.
#define HX_ALIGNMENT_MASK ((uintptr_t)(sizeof(char*)-1u)) // HX_ALIGNMENT-1

// hxMemoryManagerId. (See hxMemoryManager.cpp)
// hxMemoryManagerId_Scratch* are tightly coupled with hxMemoryAllocatorScratchpad.
// *** hxMemoryManagerId_Scratch* must be continuously assigned.  ***

enum hxMemoryManagerId {
	hxMemoryManagerId_Heap = 0,
	hxMemoryManagerId_Permanent,
	hxMemoryManagerId_TemporaryStack, // Resets to previous depth at scope closure
	hxMemoryManagerId_ScratchPage0,   // For triple buffered scratchpad algorithms
	hxMemoryManagerId_ScratchPage1,
	hxMemoryManagerId_ScratchPage2,
	hxMemoryManagerId_ScratchTemp,
	hxMemoryManagerId_ScratchAll,     // Must be last Scratch id
	hxMemoryManagerId_MAX,
	hxMemoryManagerId_Current = -1,
	hxMemoryManagerId_Console = hxMemoryManagerId_Heap
};

void* hxMalloc(size_t size);

void* hxMallocExt(size_t size, enum hxMemoryManagerId memoryAllocatorId, uintptr_t alignmentMask/*=HX_ALIGNMENT_MASK*/);

void hxFree(void* ptr);

// Allocates a copy of a string using the provided allocator.
char* hxStringDuplicate(const char* string, enum hxMemoryManagerId allocatorId /*=hxMemoryManagerId_Heap*/);

uint32_t hxIsScratchpad(void* ptr); // returns bool as int.

#if __cplusplus
} // extern "C"

// ----------------------------------------------------------------------------
// Memory Manager C++ API

// hxMemoryManagerScope
//
// Set current allocator using RAII interface.
class hxMemoryManagerScope
{
public:
	hxMemoryManagerScope(hxMemoryManagerId id);
	~hxMemoryManagerScope();

	uintptr_t getTotalAllocationCount() const;
	uintptr_t getTotalBytesAllocated() const;
	uintptr_t getScopeAllocationCount() const;
	uintptr_t getScopeBytesAllocated() const;
	uintptr_t getPreviousAllocationCount() const { return m_previousAllocationCount; }
	uintptr_t getPreviousBytesAllocated() const { return m_previousBytesAllocated; }

private:
	hxMemoryManagerScope(const hxMemoryManagerScope&); // = delete
	void operator=(const hxMemoryManagerScope&); // = delete

	hxMemoryManagerId m_thisId;
	hxMemoryManagerId m_previousId;
	uintptr_t m_previousAllocationCount;
	uintptr_t m_previousBytesAllocated;
};

// ----------------------------------------------------------------------------

void hxMemoryManagerInit();
void hxMemoryManagerShutDown();
uint32_t hxMemoryManagerAllocationCount();

// hxNew.  An extended new().  hxMemoryManagerId_Current is the default.  C++11
// perfect argument forwarding would be less of an eyesore.  Use hxArray to manage
// a dynamically-allocated array of objects if you need automatic destruction.

// hxNew<T>(...)
template <typename T>
HX_INLINE T* hxNew() { return ::new(hxMalloc(sizeof(T)))T(); }

template <typename T, typename A1>
HX_INLINE T* hxNew(const A1& a1) { return ::new(hxMalloc(sizeof(T)))T(a1); }

template <typename T, typename A1, typename A2>
HX_INLINE T* hxNew(const A1& a1, const A2& a2) { return ::new(hxMalloc(sizeof(T)))T(a1, a2); }

template <typename T, typename A1, typename A2, typename A3>
HX_INLINE T* hxNew(const A1& a1, const A2& a2, const A3& a3) { return ::new(hxMalloc(sizeof(T)))T(a1, a2, a3); }

template <typename T, typename A1, typename A2, typename A3, typename A4>
HX_INLINE T* hxNew(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return ::new(hxMalloc(sizeof(T)))T(a1, a2, a3, a4); }

// hxNewExt<T, hxMemoryManagerId>(...)
template <typename T, hxMemoryManagerId id>
HX_INLINE T* hxNewExt() { return ::new(hxMallocExt(sizeof(T), id, HX_ALIGNMENT_MASK))T(); }

template <typename T, hxMemoryManagerId id, typename A1>
HX_INLINE T* hxNewExt(const A1& a1) { return ::new(hxMallocExt(sizeof(T), id, HX_ALIGNMENT_MASK))T(a1); }

template <typename T, hxMemoryManagerId id, typename A1, typename A2>
HX_INLINE T* hxNewExt(const A1& a1, const A2& a2) { return ::new(hxMallocExt(sizeof(T), id, HX_ALIGNMENT_MASK))T(a1, a2); }

template <typename T, hxMemoryManagerId id, typename A1, typename A2, typename A3>
HX_INLINE T* hxNewExt(const A1& a1, const A2& a2, const A3& a3) { return ::new(hxMallocExt(sizeof(T), id, HX_ALIGNMENT_MASK))T(a1, a2, a3); }

template <typename T, hxMemoryManagerId id, typename A1, typename A2, typename A3, typename A4>
HX_INLINE T* hxNewExt(const A1& a1, const A2& a2, const A3& a3, const A3& a4) { return ::new(hxMallocExt(sizeof(T), id, HX_ALIGNMENT_MASK))T(a1, a2, a3, a4); }

template <typename T>
HX_INLINE void hxDelete(T* t) {
	if (t) {
		t->~T();
		if ((HX_RELEASE) < 1) {
			// Mark as released to memory manager.
			::memset((void*)t, 0xdd, sizeof *t);
		}
		hxFree(t);
	}
}

// A functor that deletes T*.  Implements std::default_delete.
struct hxDeleter {
	template <typename T>
	HX_INLINE void operator()(T* t) const { hxDelete(t); }
	HX_INLINE operator bool() const { return true; }
};

// Add default args to C++ interface: alignmentMask=HX_ALIGNMENT_MASK.
HX_INLINE void* hxMallocExt(size_t size, hxMemoryManagerId memoryAllocatorId) {
	return hxMallocExt(size, memoryAllocatorId, HX_ALIGNMENT_MASK);
}

// Add default args to C++ interface: allocatorId=hxMemoryManagerId_Heap
HX_INLINE char* hxStringDuplicate(const char* s) {
	return hxStringDuplicate(s, hxMemoryManagerId_Heap);
}

#endif // __cplusplus
