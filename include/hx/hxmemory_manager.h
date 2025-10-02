#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxmemory_manager.h Memory Manager C API. Memory allocators are
/// selected using an id. These are the large system-wide allocators, not the
/// per-container `hxallocator` which allocates from here.
///
/// General purpose memory allocators are inefficient and unsafe to use. The
/// problem is that long running code requires a lot of extra space to make sure
/// it doesn't fragment and unexpectedly fail to make a large allocation.
/// (Hardware support for virtual memory can be used to defrag the program heap,
/// but that requires processor support and system call overhead.) For code that
/// uses a lot of small intermediate allocations 1/3 of your memory and 1/3 or
/// your processor time could get eaten by the program heap allocator. The
/// `hxsystem_allocator_temporary_stack` is provided as a replacement in that
/// case.
///
/// There are also a category of allocations that are expected to last for the
/// lifetime of the application. They can be allocated with 0 overhead using
/// `hxsystem_allocator_permanent`.
///
/// NOTA BENE: The current allocator id is a thread local attribute that is
/// managed by the `hxsystem_allocator_scope` RAII class. This provides a non-
/// intrusive way to move swaths of code to different allocators.
///
/// Alignment must be a power of two. (It always is.)
///
/// `HX_RELEASE == 0` default memory markings:
///
/// - `0xab` - Allocated to client code.
/// - `0xbc` - Allocated to `hxallocator` dynamic allocation.
/// - `0xcd` - Belongs to system allocator.
/// - `0xde` - Returned to heap allocator.
/// - `0xef` - Reserved for client poisoned data.
///
/// Global new and delete are provided when `HX_NO_LIBCXX==1`. This is a
/// requirement for running as a stand alone C++ runtime. Otherwise they are not
/// interfered with. Those default versions do not use the memory manager's
/// current allocator because it may not be safe to do so. `hxnew` and
/// `hxdelete` are available as recommended substitutes.
///
/// It should be possible to implement a triple buffered streaming strategy
/// for DMA by adding a two more temp stacks.

#if !HATCHLING_VER
#error #include <hx/hatchling.h> instead.
#endif

#if HX_CPLUSPLUS
#if !HX_NO_LIBCXX
#include <new>
#endif

extern "C" {
#endif

/// `hxalignment_t` - A positive integer power of 2 for aligning allocations.
typedef unsigned int hxalignment_t;

/// `HX_ALIGNMENT` - The default alignment allows for storing things like
/// pointers. This alignment should work for most types.
#define HX_ALIGNMENT (hxalignment_t)alignof(size_t)

/// `hxsystem_allocator_t` - This is intended to be extendable by the application.
/// See `hxmemory_manager.cpp`.
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

/// `hxfree` - Frees memory previously allocated with `hxmalloc` or `hxmalloc_ext`.
/// Freeing null pointers is allowed.
/// - `ptr` : Pointer to the memory to free.
void hxfree(void* ptr_) hxattr_noexcept hxattr_hot;

/// `hxmalloc` - Allocates memory of the specified size using the default memory
/// manager. A C++ overload optionally provides the same arguments as `hxmalloc_ext`.
/// Will not return on failure.
/// - `size` : The size of the memory to allocate.
/// - `allocator`(C++ only): The memory manager ID to use for allocation. (Default is
///   `hxsystem_allocator_current`.)
/// - `alignment`(C++ only): The alignment for the allocation. (Default
///   is `HX_ALIGNMENT`.)
void* hxmalloc(size_t size_) hxattr_allocator(hxfree) hxattr_noexcept hxattr_hot;

/// `hxmalloc_ext` - Allocates memory of the specified size with a specific memory
/// manager and alignment. Will not return on failure.
/// - `size` : The size of the memory to allocate.
/// - `allocator` : The memory manager ID to use for allocation. (Default is `hxsystem_allocator_current`.)
/// - `alignment` : The alignment for the allocation. (Default is `HX_ALIGNMENT`.)
void* hxmalloc_ext(size_t size_, enum hxsystem_allocator_t allocator_,
	hxalignment_t alignment_/*=HX_ALIGNMENT*/) hxattr_noexcept hxattr_allocator(hxfree) hxattr_hot;

/// `hxstring_duplicate` - Allocates a copy of a string using the specified memory
/// manager. Returns a pointer to the duplicated string.
/// - `string` : The string to duplicate.
/// - `allocator` : The memory manager ID to use for allocation. Defaults to
///   `hxsystem_allocator_current` in C++.
char* hxstring_duplicate(const char* string_,
	enum hxsystem_allocator_t allocator_ /*=hxsystem_allocator_current*/)
		 hxattr_noexcept hxattr_allocator(hxfree) hxattr_nonnull(1) hxattr_hot;

#if HX_CPLUSPLUS
} // extern "C"

// Memory Manager C++ API

#if HX_NO_LIBCXX
// Declare placement new. These are not built into the compiler.
inline void* operator new(size_t, void* ptr_) noexcept { return ptr_; }
inline void* operator new[](size_t, void* ptr_) noexcept { return ptr_; }
#endif

// Internal.
/// \cond HIDDEN
namespace hxdetail_ {
	class hxsystem_allocator_os_heap;
	class hxsystem_allocator_stack;
}
/// \endcond

/// `hxsystem_allocator_scope` - An RAII class to set the current memory manager
/// allocator for the current scope. It automatically restores the previous
/// allocator when the scope ends. It also resets stack allocators to their
/// initial offsets thereby freeing any allocations made during the lifetime of
/// this object. NOTA BENE: Two threads cannot share a stack allocator using
/// this mechanism without due caution. Wait for worker tasks to complete before
/// freeing their temporary allocations.
class hxsystem_allocator_scope
{
public:
	/// `hxsystem_allocator_scope` - Constructor: Sets the current memory allocator to the specified ID.
	/// - `allocator` : The memory manager ID to set for this scope.
	hxsystem_allocator_scope(hxsystem_allocator_t allocator_) hxattr_noexcept;

	/// Destructor restores the previous memory manager allocator ID.
	~hxsystem_allocator_scope(void) hxattr_noexcept;

	/// Gets the total number of allocations made by the memory allocator.
	size_t get_current_allocation_count(void) const;

	/// Gets the total number of bytes allocated by the memory allocator.
	size_t get_current_bytes_allocated(void) const;

	/// Gets the number of allocations made when this scope was entered.
	size_t get_initial_allocation_count(void) const { return m_initial_allocation_count_; }

	/// Gets the number of bytes allocated when this scope was entered.
	size_t get_initial_bytes_allocated(void) const { return m_initial_bytes_allocated_; }

private:
	// The hxsystem_allocator_* classes are responsible for setting
	// m_initial_allocation_count_ and m_initial_bytes_allocated_.
	// This avoids a number of virtual calls.
	friend class hxdetail_::hxsystem_allocator_os_heap;
	friend class hxdetail_::hxsystem_allocator_stack;

	// Deleted copy constructor to prevent copying.
	hxsystem_allocator_scope(const hxsystem_allocator_scope&) = delete;

	// Deleted assignment operator to prevent copying.
	void operator=(const hxsystem_allocator_scope&) = delete;

	hxsystem_allocator_t m_this_allocator_;
	hxsystem_allocator_t m_initial_allocator_;
	size_t m_initial_allocation_count_;
	size_t m_initial_bytes_allocated_;
};

/// `hxmemory_manager_init` - Initializes the memory manager. Must be called before
/// using any memory manager functions.
void hxmemory_manager_init(void) hxattr_cold;

/// `hxmemory_manager_shut_down` - Shuts down the memory manager. Frees any remaining
/// resources.
void hxmemory_manager_shut_down(void) hxattr_cold;

/// `hxmemory_manager_leak_count` - Returns the total number of allocations outstanding
/// made by the memory manager.
size_t hxmemory_manager_leak_count(void) hxattr_cold;

/// `hxdelete` - Deletes an object of type T and frees its memory using the memory
/// manager.
/// - `t` : Pointer to the object to delete.
template <typename T_>
void hxdelete(T_* t_) {
	if(t_) {
		t_->~T_();
		if((HX_RELEASE) < 1) {
			// Mark as released to memory manager.
			::memset((void*)t_, 0xcd, sizeof *t_);
		}
		hxfree(t_);
	}
}

/// `hxnew<T, allocator, align>(...)` - Allocates and constructs an object of type
/// T using an optional memory allocator and alignment. Returns a pointer to the
/// newly constructed object. Will not return on failure.
/// - `allocator` : The memory manager ID to use for allocation. Defaults to `hxsystem_allocator_current`.
/// - `align` : A mask of low bits to be zeroed out when allocating new pointers. Defaults to `HX_ALIGNMENT`.
template <typename T_, hxsystem_allocator_t allocator_=hxsystem_allocator_current, hxalignment_t align_=HX_ALIGNMENT, typename... Args_>
T_* hxnew(Args_&&... args_) {
	return ::new(hxmalloc_ext(sizeof(T_), allocator_, align_)) T_(args_...);
}

/// A functor that deletes objects of type `T` using `hxdelete`. Used by
/// containers to implement the destruction of their contents according to a
/// template parameter. Implements `std::default_delete`.
class hxdefault_delete {
public:
	/// Deletes the object using `hxdelete`.
	template <typename T_>
	void operator()(T_* t_) const { hxdelete(t_); }

	/// Always returns true, indicating the deleter is valid.
	operator bool(void) const { return true; }
};

/// A version of `hxdefault_delete` that does not delete the object. Allows
/// removing object destruction from container destructors that handle static
/// allocations or don't own their contents for another reason.
class hxdo_not_delete {
public:
	/// Does not delete the object.
	template <typename T_>
	void operator()(T_*) const { }

	/// Always returns false, indicating the deleter should not be called.
	operator bool(void) const { return false; }
};

/// `hxmalloc` - Add `hxmalloc_ext args` to `hxmalloc` C interface. Allocates memory with a
/// specific memory manager and alignment.
inline void* hxmalloc( size_t size_, enum hxsystem_allocator_t allocator_, hxalignment_t alignment_=HX_ALIGNMENT) {
	return hxmalloc_ext(size_, allocator_, alignment_);
}

/// `hxstring_duplicate` - Add default args to C interface:
///   allocator=hxsystem_allocator_current
/// Duplicates a string using the default memory manager.
inline char* hxstring_duplicate(const char* s_) {
	return hxstring_duplicate(s_, hxsystem_allocator_current);
}

#endif // HX_CPLUSPLUS
