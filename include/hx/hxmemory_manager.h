#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead
#endif

#if HX_CPLUSPLUS
#if HX_HOSTED
#include <new>
#endif

extern "C" {
#endif

// hxmemory_manager.h - Memory Manager C API. Memory allocators are selected using
// an id. These are the large system-wide allocators, not the per-object
// hxallocator which allocates from here.
//
// Nota bene: The current allocator is a thread local attribute.
//
// Alignment is specified using a mask of those LSB bits that must be 0. Which
// is a value 1 less than the actual power of two alignment.

/// `HX_ALIGNMENT` - The default alignment allows for storing char pointers. This
/// alignment should work for most types.
#define HX_ALIGNMENT sizeof(char*)

/// hxsystem_allocator_t. (See hxmemory_manager.cpp)
enum hxsystem_allocator_t {
	/// OS heap with alignment and stats.
	hxsystem_allocator_heap,
	/// Contigious allocations that must not be freed.
	hxsystem_allocator_permanent,
	/// Resets to previous depth at scope closure
	hxsystem_allocator_temporary_stack,
	// ** hxsystem_allocator_current must be last in enum. **
	/// Use current allocation scope.
	hxsystem_allocator_current
};

/// `hxmalloc` - Allocates memory of the specified size using the default memory
/// manager. A C++ overload optionally provides the same arguments as hxmalloc_ext.
/// Will not return on failure.
/// - `size` : The size of the memory to allocate.
/// - `allocator`(C++ only): The memory manager ID to use for allocation. (Default is
///   hxsystem_allocator_current.)
/// - `alignment`(C++ only): The alignment for the allocation. (Default
///   is HX_ALIGNMENT.)
void* hxmalloc(size_t size_);

/// `hxmalloc_ext` - Allocates memory of the specified size with a specific memory
/// manager and alignment. Will not return on failure.
/// - `size` : The size of the memory to allocate.
/// - `allocator` : The memory manager ID to use for allocation. (Default is hxsystem_allocator_current.)
/// - `alignment` : The alignment for the allocation. (Default is HX_ALIGNMENT.)
void* hxmalloc_ext(size_t size_, enum hxsystem_allocator_t allocator_, uintptr_t alignment_/*=HX_ALIGNMENT*/);

/// `hxfree` - Frees memory previously allocated with hxmalloc or hxmalloc_ext.
/// - `ptr` : Pointer to the memory to free.
void hxfree(void* ptr_);

/// `hxstring_duplicate` - Allocates a copy of a string using the specified memory
/// manager. Returns a pointer to the duplicated string.
/// - `string` : The string to duplicate.
/// - `allocator` : The memory manager ID to use for allocation. Defaults to
///   hxsystem_allocator_current in C++.
char* hxstring_duplicate(const char* string_, enum hxsystem_allocator_t allocator_ /*=hxsystem_allocator_current*/);

#if HX_CPLUSPLUS
} // extern "C"

// Memory Manager C++ API

#if !HX_HOSTED
// Declare placement new.
inline void* operator new(size_t, void* ptr_) noexcept { return ptr_; }
inline void* operator new[](size_t, void* ptr_) noexcept { return ptr_; }
inline void operator delete(void*, void*) noexcept { }
inline void operator delete[](void*, void*) noexcept { }
#endif

/// `hxsystem_allocator_scope` - RAII class to set the current memory manager allocator
/// for a specific scope. Automatically restores the previous allocator when the
/// scope ends.
class hxsystem_allocator_scope
{
public:
	/// `hxsystem_allocator_scope` - Constructor: Sets the current memory allocator to the specified ID.
	/// - `allocator` : The memory manager ID to set for this scope.
	hxsystem_allocator_scope(hxsystem_allocator_t allocator_);

	/// Destructor restores the previous memory manager allocator ID.
	~hxsystem_allocator_scope(void);

	/// Gets the total number of allocations made by the memory allocator.
	size_t get_total_allocation_count(void) const;

	/// Gets the total number of bytes allocated by the memory allocator.
	size_t get_total_bytes_allocated(void) const;

	/// Gets the number of allocations made within this scope.
	size_t get_scope_allocation_count(void) const;

	/// Gets the number of bytes allocated within this scope.
	size_t get_scope_bytes_allocated(void) const;

	/// Gets the number of allocations made before this scope was entered.
	size_t get_previous_allocation_count(void) const { return m_previous_allocation_count_; }

	/// Gets the number of bytes allocated before this scope was entered.
	size_t get_previous_bytes_allocated(void) const { return m_previous_bytes_allocated_; }

private:
	// Deleted copy constructor to prevent copying.
	hxsystem_allocator_scope(const hxsystem_allocator_scope&) = delete;

	// Deleted assignment operator to prevent copying.
	void operator=(const hxsystem_allocator_scope&) = delete;

	hxsystem_allocator_t m_this_allocator_; // The memory manager ID for this scope.
	hxsystem_allocator_t m_previous_allocator_; // The previous memory manager ID.
	size_t m_previous_allocation_count_; // Previous allocation count.
	size_t m_previous_bytes_allocated_; // Previous bytes allocated.
};

/// `hxmemory_manager_init` - Initializes the memory manager. Must be called before
/// using any memory manager functions.
void hxmemory_manager_init(void);

/// `hxmemory_manager_shut_down` - Shuts down the memory manager. Frees any remaining
/// resources.
void hxmemory_manager_shut_down(void);

/// `hxmemory_manager_leak_count` - Returns the total number of allocations outstanding
/// made by the memory manager.
size_t hxmemory_manager_leak_count(void);

/// `hxnew<T, allocator, align>(...)` - Allocates and constructs an object of type
/// T using an optional memory allocator and alignment. Returns a pointer to the
/// newly constructed object. Will not return on failure.
/// - `allocator` : The memory manager ID to use for allocation. Defaults to hxsystem_allocator_current.
/// - `align` : A mask of low bits to be zero'd out when allocating new pointers. Defaults to HX_ALIGNMENT.
template <typename T_, hxsystem_allocator_t allocator_=hxsystem_allocator_current, uintptr_t align_=HX_ALIGNMENT, typename... Args_>
constexpr T_* hxnew(Args_&&... args_) noexcept {
	return ::new(hxmalloc_ext(sizeof(T_), allocator_, align_)) T_(args_...);
}

/// `hxdelete` - Deletes an object of type T and frees its memory using the memory
/// manager.
/// - `t` : Pointer to the object to delete.
template <typename T_>
constexpr void hxdelete(T_* t_) {
	if (t_) {
		t_->~T_();
		if ((HX_RELEASE) < 1) {
			// Mark as released to memory manager.
			::memset((void*)t_, 0xdd, sizeof *t_);
		}
		hxfree(t_);
	}
}

/// `hxdeleter` - A functor that deletes objects of type T using hxdelete.
/// Implements std::default_delete.
class hxdeleter {
public:
	/// Deletes the object using hxdelete.
	template <typename T_>
	constexpr void operator()(T_* t_) const { hxdelete(t_); }

	/// Always returns true, indicating the deleter is valid.
	constexpr operator bool(void) const { return true; }
};

/// Implement hxdeleter with NOPs. Allows the compiler to remove the destructors
/// from containers that handle static allocations or don't own their contents for
/// another reason.
class hxdo_not_delete {
public:
	/// Deletes the object using hxdelete.
	template <typename T_>
	constexpr void operator()(T_*) const { }

	/// Always returns false, indicating the deleter should not be called.
	constexpr operator bool(void) const { return false; }
};

/// `hxmalloc` - Add hxmalloc_ext args to hxmalloc C interface. Allocates memory with a
/// specific memory manager and alignment.
inline void* hxmalloc( size_t size_, enum hxsystem_allocator_t allocator_, uintptr_t alignment_=HX_ALIGNMENT) {
	return hxmalloc_ext(size_, allocator_, alignment_);
}

/// `hxstring_duplicate` - Add default args to C interface:
///   allocator=hxsystem_allocator_current
/// Duplicates a string using the default memory manager.
inline char* hxstring_duplicate(const char* s_) {
	return hxstring_duplicate(s_, hxsystem_allocator_current);
}

#endif // HX_CPLUSPLUS
