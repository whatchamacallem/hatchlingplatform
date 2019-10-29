#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#ifndef HATCHLING_H
#error "include hatchling.h"
#endif

#ifdef __cplusplus
#include <new>
extern "C" {
#endif

#define HX_ALIGNMENT_MASK ((uintptr_t)(sizeof(void*) - 1u)) // HX_ALIGNMENT-1
#define hxIsAligned(x) (((uintptr_t)(x) & HX_ALIGNMENT_MASK) == 0)
#define hxAssertAligned(x) hxAssert(hxIsAligned(x))

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

// C API for hxMemoryManager:
void* hxMalloc(size_t size);
void* hxMallocExt(size_t size, enum hxMemoryManagerId memoryAllocatorId, uintptr_t alignmentMask/*=HX_ALIGNMENT_MASK*/);
void hxFree(void* ptr);
uint32_t hxIsScratchpad(void* ptr); // returns bool as int.

// ----------------------------------------------------------------------------
#ifdef __cplusplus
} // extern "C"

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

// Add default parameters to C calls.
void* hxMallocExt(size_t size, hxMemoryManagerId memoryAllocatorId);
char* hxStringDuplicate(const char* s);

// hxNew.  An extended new().  hxMemoryManagerId_Current is the default.
// C++11 perfect argument forwarding would be less of an eyesore.

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
			::memset((void*)t, 0xdd, sizeof *t);
		}
		hxFree(t);
	}
}

// Functor deletes T*.  implements std::default_delete.
struct hxDeleter {
	template <typename T>
	HX_INLINE void operator()(T* t) const { hxDelete(t); }
	HX_INLINE operator bool() const { return true; }
};

#endif // __cplusplus
