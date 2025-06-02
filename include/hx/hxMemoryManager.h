#pragma once
// Copyright 2017-2025 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

#if HX_CPLUSPLUS
#include <new>
#if HX_CPLUSPLUS >= 201103L // std::forward requires c++11 and <utility>
#include <utility>
#endif

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

// The default alignment HX_ALIGNMENT allows for storing char pointers.
// This alignment should work for most types except vectors.
#define HX_ALIGNMENT ((uintptr_t)sizeof(char*))

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
// A C++ overload optionally provides the same arguments as hxMallocExt.
// - size_: The size of the memory to allocate.
// - id_(C++ only): The memory manager ID to use for allocation. (Default is
//   hxMemoryManagerId_Current.)
// - alignment_(C++ only): The alignment for the allocation. (Default
//   is HX_ALIGNMENT.)
void* hxMalloc(size_t size_);

// Allocates memory of the specified size with a specific memory manager and alignment.
// Parameters:
// - size_: The size of the memory to allocate.
// - id_: The memory manager ID to use for allocation. (Default is hxMemoryManagerId_Current.)
// - alignment_: The alignment for the allocation. (Default is HX_ALIGNMENT.)
void* hxMallocExt(size_t size_, enum hxMemoryManagerId id_, uintptr_t alignment_/*=HX_ALIGNMENT*/);

// Frees memory previously allocated with hxMalloc or hxMallocExt.
// Parameters:
// - ptr_: Pointer to the memory to free.
void hxFree(void* ptr_);

// Allocates a copy of a string using the specified memory manager.
// Parameters:
// - string_: The string to duplicate.
// - id_: The memory manager ID to use for allocation. Defaults to
//   hxMemoryManagerId_Current in C++.
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
	hxMemoryManagerScope(const hxMemoryManagerScope&) HX_DELETE_FN;

	// Deleted assignment operator to prevent copying.
	void operator=(const hxMemoryManagerScope&) HX_DELETE_FN;

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

// hxNew.  An extended new().  hxMemoryManagerId_Current is the default.
// Allocates and constructs an object of type T_ using a specific memory manager.
// Template Parameters:
// - id_: The memory manager ID to use for allocation.
// - align_: A mask of low bits to be zero'd out when allocating new pointers.
// Returns: A pointer to the newly constructed object.
#if HX_CPLUSPLUS >= 201103L // Argument forwarding requires c++11.
template <typename T_, hxMemoryManagerId id_=hxMemoryManagerId_Current, uintptr_t align_=HX_ALIGNMENT, typename... Args_>
HX_CONSTEXPR_FN T_* hxNew(Args_&&... args_) noexcept {
	return ::new(hxMallocExt(sizeof(T_), id_, align_)) T_(args_...);
}
#else
template <typename T_, hxMemoryManagerId id_=hxMemoryManagerId_Current, uintptr_t align_=HX_ALIGNMENT>
HX_CONSTEXPR_FN T_* hxNew() {
	return ::new(hxMallocExt(sizeof(T_), id_, align_))T_();
}
template <typename T_, hxMemoryManagerId id_=hxMemoryManagerId_Current, uintptr_t align_=HX_ALIGNMENT, typename Arg>
HX_CONSTEXPR_FN T_* hxNew(const Arg& arg) {
	return ::new(hxMallocExt(sizeof(T_), id_, align_))T_(arg);
}
#endif

// Deletes an object of type T_ and frees its memory using the memory manager.
// Parameters:
// - t_: Pointer to the object to delete.
template <typename T_>
HX_CONSTEXPR_FN void hxDelete(T_* t_) {
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
	HX_CONSTEXPR_FN void operator()(T_* t_) const { hxDelete(t_); }

	// Always returns true, indicating the deleter is valid.
	HX_CONSTEXPR_FN operator bool() const { return true; }
};

// Add hxMallocExt args to hxMalloc C interface. Allocates memory with a
// specific memory manager and alignment.
inline void* hxMalloc( size_t size_,
				enum hxMemoryManagerId id_,
				uintptr_t alignment_=HX_ALIGNMENT) {
	return hxMallocExt(size_, id_, alignment_);
}

// Add default args to C interface: id_=hxMemoryManagerId_Current
// Duplicates a string using the default memory manager.
inline char* hxStringDuplicate(const char* s_) {
	return hxStringDuplicate(s_, hxMemoryManagerId_Current);
}

#endif // HX_CPLUSPLUS
