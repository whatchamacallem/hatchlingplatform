#pragma once
// Copyright 2017-2025 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

#if HX_CPLUSPLUS
#include <new>

extern "C" {
#endif

// ----------------------------------------------------------------------------
// Memory Manager C API
//
// Memory allocators are selected using an id.  These are the large system-wide
// allocators, not the per-object hxAllocator which allocates from here.
//
// Nota bene:  The current allocator is a thread local attribute.
//
// Alignment is specified using a mask of those LSB bits that must be 0.  Which
// is a value 1 less than the actual power of two alignment.

// The default alignment HX_ALIGNMENT_MASK allows for storing char pointers.
// This alignment should work for most types except SIMD vectors.
#define HX_ALIGNMENT_MASK ((uintptr_t)(sizeof(char*)-1u)) // masks bits that must be zero.

// hxMemoryManagerId. (See hxMemoryManager.cpp)
// hxMemoryManagerId_Scratch* are tightly coupled with hxMemoryAllocatorScratchpad.
// *** hxMemoryManagerId_Scratch* must be contiguously assigned.  ***

enum hxMemoryManagerId {
	hxMemoryManagerId_Heap = 0,
	hxMemoryManagerId_Permanent,
	hxMemoryManagerId_TemporaryStack, // Resets to previous depth at scope closure
#if HX_USE_MEMORY_SCRATCH
	hxMemoryManagerId_ScratchPage0,   // For triple buffered scratchpad algorithms
	hxMemoryManagerId_ScratchPage1,
	hxMemoryManagerId_ScratchPage2,
	hxMemoryManagerId_ScratchTemp,
	hxMemoryManagerId_ScratchAll, 	// Must be last Scratch id_
#endif
	hxMemoryManagerId_MAX,
	hxMemoryManagerId_Current = -1,
	hxMemoryManagerId_Console = hxMemoryManagerId_Heap
};

// Allocates memory of the specified size using the default memory manager.
void* hxMalloc(size_t size_);

// Allocates memory of the specified size with a specific memory manager and alignment.
// Parameters:
// - size_: The size of the memory to allocate.
// - id_: The memory manager ID to use for allocation.
// - alignmentMask_: The alignment mask for the allocation (default is HX_ALIGNMENT_MASK).
void* hxMallocExt(size_t size_, enum hxMemoryManagerId id_, uintptr_t alignmentMask_/*=HX_ALIGNMENT_MASK*/);

// Frees memory previously allocated with hxMalloc or hxMallocExt.
// Parameters:
// - ptr_: Pointer to the memory to free.
void hxFree(void* ptr_);

// Allocates a copy of a string using the specified memory manager.
// Parameters:
// - string_: The string to duplicate.
// - id_: The memory manager ID to use for allocation (default is hxMemoryManagerId_Heap).
// Returns: A pointer to the duplicated string.
char* hxStringDuplicate(const char* string_, enum hxMemoryManagerId id_ /*=hxMemoryManagerId_Current*/);

// Checks if the given pointer belongs to a scratchpad memory allocator.
// Parameters:
// - ptr_: The pointer to check.
// Returns: An integer (boolean) indicating whether the pointer is from a scratchpad.
int hxIsScratchpad(void* ptr_); // returns bool as int.

#if HX_CPLUSPLUS
} // extern "C"

// ----------------------------------------------------------------------------
// Memory Manager C++ API

// hxMemoryManagerScope
//
// RAII class to set the current memory manager allocator for a specific scope.
// Automatically restores the previous allocator when the scope ends.
class hxMemoryManagerScope
{
public:
	// Constructor: Sets the current memory allocator to the specified ID.
	// Parameters:
	// - id_: The memory manager ID to set for this scope.
	hxMemoryManagerScope(hxMemoryManagerId id_);

	// Destructor: Restores the previous memory manager allocator ID.
	~hxMemoryManagerScope();

	// Gets the total number of allocations made by the memory allocator.
	uintptr_t getTotalAllocationCount() const;

	// Gets the total number of bytes allocated by the memory allocator.
	uintptr_t getTotalBytesAllocated() const;

	// Gets the number of allocations made within this scope.
	uintptr_t getScopeAllocationCount() const;

	// Gets the number of bytes allocated within this scope.
	uintptr_t getScopeBytesAllocated() const;

	// Gets the number of allocations made before this scope was entered.
	uintptr_t getPreviousAllocationCount() const { return m_previousAllocationCount; }

	// Gets the number of bytes allocated before this scope was entered.
	uintptr_t getPreviousBytesAllocated() const { return m_previousBytesAllocated; }

private:
	// Deleted copy constructor to prevent copying.
	hxMemoryManagerScope(const hxMemoryManagerScope&); // = delete

	// Deleted assignment operator to prevent copying.
	void operator=(const hxMemoryManagerScope&); // = delete

	hxMemoryManagerId m_thisId; // The memory manager ID for this scope.
	hxMemoryManagerId m_previousId; // The previous memory manager ID.
	uintptr_t m_previousAllocationCount; // Previous allocation count.
	uintptr_t m_previousBytesAllocated; // Previous bytes allocated.
};

// Initializes the memory manager. Must be called before using any memory manager functions.
void hxMemoryManagerInit();

// Shuts down the memory manager. Frees any remaining resources.
void hxMemoryManagerShutDown();

// Gets the total number of allocations made by the memory manager.
// Returns: The total allocation count.
size_t hxMemoryManagerAllocationCount();

// hxNew.  An extended new().  hxMemoryManagerId_Current is the default.  C++11
// perfect argument forwarding would be less of an eyesore.  Use hxArray to manage
// a dynamically-allocated array of objects if you need automatic destruction.

// Allocates and constructs an object of type T_ using the default memory allocator.
// Returns: A pointer to the newly constructed object.
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

// Allocates and constructs an object of type T_ using a specific memory manager.
// Parameters:
// - id_: The memory manager ID to use for allocation.
// Returns: A pointer to the newly constructed object.
template <typename T_, hxMemoryManagerId id_>
HX_INLINE T_* hxNewExt() { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(); }

template <typename T_, hxMemoryManagerId id_, typename A1_>
HX_INLINE T_* hxNewExt(const A1_& a1_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_, typename A3_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_, const A3_& a3_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_, a3_); }

template <typename T_, hxMemoryManagerId id_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_INLINE T_* hxNewExt(const A1_& a1_, const A2_& a2_, const A3_& a3_, const A4_& a4_) { return ::new(hxMallocExt(sizeof(T_), id_, HX_ALIGNMENT_MASK))T_(a1_, a2_, a3_, a4_); }

// Deletes an object of type T_ and frees its memory using the memory manager.
// Parameters:
// - t_: Pointer to the object to delete.
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

// A functor that deletes objects of type T_ using hxDelete.
// Implements std::default_delete.
struct hxDeleter {
	// Deletes the object using hxDelete.
	template <typename T_>
	HX_INLINE void operator()(T_* t_) const { hxDelete(t_); }

	// Always returns true, indicating the deleter is valid.
	HX_INLINE operator bool() const { return true; }
};

// Add default args to C++ interface: alignmentMask=HX_ALIGNMENT_MASK.
// Allocates memory with a specific memory manager and default alignment.
HX_INLINE void* hxMallocExt(size_t size_, hxMemoryManagerId id_) {
	return hxMallocExt(size_, id_, HX_ALIGNMENT_MASK);
}

// Add default args to C++ interface: id_=hxMemoryManagerId_Current
// Duplicates a string using the default memory manager.
HX_INLINE char* hxStringDuplicate(const char* s_) {
	return hxStringDuplicate(s_, hxMemoryManagerId_Current);
}

#endif // HX_CPLUSPLUS
