#pragma once
// Copyright 2017-2025 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

#if HX_CPLUSPLUS
#include <new>

extern "C" {
#endif

// hxMemoryAllocator - Memory Manager C API. Memory allocators are selected using
// an id. These are the large system-wide allocators, not the per-object
// hxAllocator which allocates from here.
//
// Nota bene: The current allocator is a thread local attribute.
//
// Alignment is specified using a mask of those LSB bits that must be 0. Which
// is a value 1 less than the actual power of two alignment.

// HX_ALIGNMENT - The default alignment allows for storing char pointers. This
// alignment should work for most types.
#define HX_ALIGNMENT sizeof(char*)

// hxMemoryAllocator. (See hxMemoryManager.cpp)

enum hxMemoryAllocator {
	hxMemoryAllocator_Heap,           // OS heap with alignment and stats.
	hxMemoryAllocator_Permanent,      // Contigious allocations that must not be freed.
	hxMemoryAllocator_TemporaryStack, // Resets to previous depth at scope closure
	// ** hxMemoryAllocator_Current must be last in enum. **
	hxMemoryAllocator_Current         // Use current allocation scope.
};

// hxMalloc - Allocates memory of the specified size using the default memory
// manager. A C++ overload optionally provides the same arguments as hxMallocExt.
// Will not return on failure.
// - size_: The size of the memory to allocate.
// - allocator_(C++ only): The memory manager ID to use for allocation. (Default is
//   hxMemoryAllocator_Current.)
// - alignment_(C++ only): The alignment for the allocation. (Default
//   is HX_ALIGNMENT.)
void* hxMalloc(size_t size_);

// hxMallocExt - Allocates memory of the specified size with a specific memory
// manager and alignment. Will not return on failure.
// - size_: The size of the memory to allocate.
// - allocator_: The memory manager ID to use for allocation. (Default is hxMemoryAllocator_Current.)
// - alignment_: The alignment for the allocation. (Default is HX_ALIGNMENT.)
void* hxMallocExt(size_t size_, enum hxMemoryAllocator allocator_, uintptr_t alignment_/*=HX_ALIGNMENT*/);

// hxFree - Frees memory previously allocated with hxMalloc or hxMallocExt.
// - ptr_: Pointer to the memory to free.
void hxFree(void* ptr_);

// hxStringDuplicate - Allocates a copy of a string using the specified memory
// manager.
// - string_: The string to duplicate.
// - allocator_: The memory manager ID to use for allocation. Defaults to
//   hxMemoryAllocator_Current in C++.
// Returns: A pointer to the duplicated string.
char* hxStringDuplicate(const char* string_, enum hxMemoryAllocator allocator_ /*=hxMemoryAllocator_Current*/);

#if HX_CPLUSPLUS
} // extern "C"

// Memory Manager C++ API

// hxMemoryAllocatorScope - RAII class to set the current memory manager allocator
// for a specific scope. Automatically restores the previous allocator when the
// scope ends.
class hxMemoryAllocatorScope
{
public:
    // hxMemoryAllocatorScope - Constructor: Sets the current memory allocator to the specified ID.
    // - allocator_: The memory manager ID to set for this scope.
    hxMemoryAllocatorScope(hxMemoryAllocator allocator_);

	// Destructor restores the previous memory manager allocator ID.
	~hxMemoryAllocatorScope();

	// Gets the total number of allocations made by the memory allocator.
	uintptr_t getTotalAllocationCount() const;

	// Gets the total number of bytes allocated by the memory allocator.
	uintptr_t getTotalBytesAllocated() const;

	// Gets the number of allocations made within this scope.
	uintptr_t getScopeAllocationCount() const;

	// Gets the number of bytes allocated within this scope.
	uintptr_t getScopeBytesAllocated() const;

	// Gets the number of allocations made before this scope was entered.
	uintptr_t getPreviousAllocationCount() const { return m_previousAllocationCount_; }

	// Gets the number of bytes allocated before this scope was entered.
	uintptr_t getPreviousBytesAllocated() const { return m_previousBytesAllocated_; }

private:
	// Deleted copy constructor to prevent copying.
	hxMemoryAllocatorScope(const hxMemoryAllocatorScope&) HX_DELETE_FN;

	// Deleted assignment operator to prevent copying.
	void operator=(const hxMemoryAllocatorScope&) HX_DELETE_FN;

    hxMemoryAllocator m_thisAllocator_; // The memory manager ID for this scope.
    hxMemoryAllocator m_previousAllocator_; // The previous memory manager ID.
    uintptr_t m_previousAllocationCount_; // Previous allocation count.
    uintptr_t m_previousBytesAllocated_; // Previous bytes allocated.
};

// hxMemoryManagerInit - Initializes the memory manager. Must be called before
// using any memory manager functions.
void hxMemoryManagerInit();

// hxMemoryManagerShutDown - Shuts down the memory manager. Frees any remaining
// resources.
void hxMemoryManagerShutDown();

// hxMemoryManagerAllocationCount - Gets the total number of allocations made by
// the memory manager.
// Returns: The total allocation count.
size_t hxMemoryManagerAllocationCount();

// hxNew - An extended new(). hxMemoryAllocator_Current is the default.
// Allocates and constructs an object of type T_ using a specific memory manager.
// Template Parameters:
// - allocator_: The memory manager ID to use for allocation.
// - align_: A mask of low bits to be zero'd out when allocating new pointers.
// Returns: A pointer to the newly constructed object. Will not return on failure.
#if HX_CPLUSPLUS >= 201103L // Argument forwarding requires c++11.
template <typename T_, hxMemoryAllocator allocator_=hxMemoryAllocator_Current, uintptr_t align_=HX_ALIGNMENT, typename... Args_>
HX_CONSTEXPR_FN T_* hxNew(Args_&&... args_) noexcept {
	return ::new(hxMallocExt(sizeof(T_), allocator_, align_)) T_(args_...);
}
#else
// hxNew - C++98 functor polyfill. All template args default except the first.
// Passing more than one arg requires C++11.
template <typename T_, hxMemoryAllocator allocator_=hxMemoryAllocator_Current, uintptr_t align_=HX_ALIGNMENT>
struct hxNew {
	inline explicit hxNew(void ) {
		m_tmp_ = ::new(hxMalloc(sizeof(T_)))T_();
	}

	template<typename Arg_>
	inline explicit hxNew(const Arg_& arg_) {
		m_tmp_ = ::new(hxMalloc(sizeof(T_)))T_(arg_);
	}
	operator T_*() { return m_tmp_; }
	T_* m_tmp_;
};
#endif

// hxDelete - Deletes an object of type T_ and frees its memory using the memory
// manager.
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

// hxDeleter - A functor that deletes objects of type T_ using hxDelete.
// Implements std::default_delete.
struct hxDeleter {
	// Deletes the object using hxDelete.
	template <typename T_>
	HX_CONSTEXPR_FN void operator()(T_* t_) const { hxDelete(t_); }

	// Always returns true, indicating the deleter is valid.
	HX_CONSTEXPR_FN operator bool() const { return true; }
};

// hxMalloc - Add hxMallocExt args to hxMalloc C interface. Allocates memory with a
// specific memory manager and alignment.
inline void* hxMalloc( size_t size_, enum hxMemoryAllocator allocator_, uintptr_t alignment_=HX_ALIGNMENT) {
	return hxMallocExt(size_, allocator_, alignment_);
}

// hxStringDuplicate - Add default args to C interface:
//   allocator_=hxMemoryAllocator_Current
// Duplicates a string using the default memory manager.
inline char* hxStringDuplicate(const char* s_) {
	return hxStringDuplicate(s_, hxMemoryAllocator_Current);
}

#endif // HX_CPLUSPLUS
