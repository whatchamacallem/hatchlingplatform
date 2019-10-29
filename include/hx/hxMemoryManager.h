#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#if !HATCHLING_VER
#error #include <hx/hatchling.h>
#endif

#if defined(__cplusplus)
#include <new>

extern "C" {
#endif

// ----------------------------------------------------------------------------
// Memory Manager C API
//
// Memory allocators are selected using an id.  These are the large system-wide
// allocators, not the per-object hxAllocator which allocates from here.
//
// Nota bene:  While the current allocator is a thread local attribute, the
// memory manager does not support concurrent access to the same allocator.
// Either preallocate working buffers or arrange for locking around shared
// allocators.  
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
#if HX_USE_MEMORY_SCRATCH
	hxMemoryManagerId_ScratchPage0,   // For triple buffered scratchpad algorithms
	hxMemoryManagerId_ScratchPage1,
	hxMemoryManagerId_ScratchPage2,
	hxMemoryManagerId_ScratchTemp,
	hxMemoryManagerId_ScratchAll,     // Must be last Scratch id_
#endif
	hxMemoryManagerId_MAX,
	hxMemoryManagerId_Current = -1,
	hxMemoryManagerId_Console = hxMemoryManagerId_Heap
};

void* hxMalloc(size_t size_);

void* hxMallocExt(size_t size_, enum hxMemoryManagerId id_, uintptr_t alignmentMask_/*=HX_ALIGNMENT_MASK*/);

void hxFree(void* ptr_);

// Allocates a copy of a string using the provided allocator.
char* hxStringDuplicate(const char* string_, enum hxMemoryManagerId id_ /*=hxMemoryManagerId_Heap*/);

uint32_t hxIsScratchpad(void* ptr_); // returns bool as int.

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
	hxMemoryManagerScope(hxMemoryManagerId id_);
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

// hxNew<T_>(...)
template <typename T_>
HX_INLINE T_* hxNew() { return ::new(hxMalloc(sizeof(T_)))T_(); }

template <typename T_, typename A1_>
HX_INLINE T_* hxNew(const A1_& a1_) { return ::new(hxMalloc(sizeof(T_)))T_(a1_); }

template <typename T_, typename A1_, typename A2_>
HX_INLINE T_* hxNew(const A1_& a1_, const A2_& a2_) { return ::new(hxMalloc(sizeof(T_)))T_(a1_, a2_); }

template <typename T_, typename A1_, typename A2_, typename A3_>
HX_INLINE T_* hxNew(const A1_& a1_, const A2_& a2_, const A3_& a3_) { return ::new(hxMalloc(sizeof(T_)))T_(a1_, a2_, a3_); }

template <typename T_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_INLINE T_* hxNew(const A1_& a1_, const A2_& a2_, const A3_& a3_, const A4_& a4_) { return ::new(hxMalloc(sizeof(T_)))T_(a1_, a2_, a3_, a4_); }

// hxNewExt<T_, hxMemoryManagerId>(...)
template <typename T_, hxMemoryManagerId id_>
HX_INLINE T_* hxNewExt() { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(); }

template <typename T_, hxMemoryManagerId id_, typename A1_>
HX_INLINE T_* hxNewExt(const A1_& a1_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_, typename A3_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_, const A3_& a3_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_, a3_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_, const A3_& a3_, const A3_& a4_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_, a3_, a4_); }

template <typename T_>
HX_INLINE void hxDelete(T_* t_) {
	if (t_) {
		t_->~T_();
		if ((HX_RELEASE) < 1) {
			// Mark as released to memory manager.
			::memset((void*)t_, 0xdd, sizeof *t_);
		}
		hxFree(t_);
	}
}

// A functor that deletes T*.  Implements std::default_delete.
struct hxDeleter {
	template <typename T_>
	HX_INLINE void operator()(T_* t_) const { hxDelete(t_); }
	HX_INLINE operator bool() const { return true; }
};

// Add default args to C++ interface: alignmentMask=HX_ALIGNMENT_MASK.
HX_INLINE void* hxMallocExt(size_t size_, hxMemoryManagerId id_) {
	return hxMallocExt(size_, id_, HX_ALIGNMENT_MASK);
}

// Add default args to C++ interface: id_=hxMemoryManagerId_Heap
HX_INLINE char* hxStringDuplicate(const char* s_) {
	return hxStringDuplicate(s_, hxMemoryManagerId_Heap);
}

#endif // __cplusplus
