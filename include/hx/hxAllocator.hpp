#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxAllocator - Similar to std::allocator. Allows for static or dynamic
// allocation.

// A capacity value that allows for dynamic allocation.
#define hxAllocatorDynamicCapacity 0u

// hxAllocator<1+> - Provides static allocation when capacity is greater than
// zero.
template<typename T_, size_t Capacity_>
class hxAllocator {
public:
    typedef T_ T;

    // Template specialization below should have been selected.
    HX_STATIC_ASSERT(Capacity_ > 0u, "Capacity_ > 0");

	// Initializes memory to 0xcd when HX_RELEASE < 1.
    HX_CONSTEXPR_FN hxAllocator() {
        if ((HX_RELEASE) < 1) {
            ::memset(m_allocator_, 0xcd, sizeof m_allocator_);
        }
    }

	// Returns the number of elements of T allocated.
    HX_CONSTEXPR_FN size_t capacity() const { return Capacity_; }

	// Returns a const array of T.
    HX_CONSTEXPR_FN const T* data() const { return reinterpret_cast<const T*>(m_allocator_ + 0); }

	// Returns an array of T.
    HX_CONSTEXPR_FN T* data() { return reinterpret_cast<T*>(m_allocator_ + 0); }

protected:
    // Used to ensure initial capacity as reserveStorage() will not reallocate.
    // size_: The number of elements of type T to allocate.
	HX_CONSTEXPR_FN void reserveStorage(size_t size_) {
		hxAssertRelease(size_ <= Capacity_, "allocator overflowing fixed capacity."); (void)size_;
	}

private:
	// *** The static allocator does not support swapping allocations or
	// assignments from temporaries. ***
    HX_CONSTEXPR_FN void swap(hxAllocator& rhs) HX_DELETE_FN;
    static const size_t m_capacity_ = Capacity_;

	// Using union to implement alignas(char *).
    union {
        // Char arrays are the least likely to encounter undefined behavior.
        char m_allocator_[m_capacity_ * sizeof(T)];
        char* m_charPointerAlign_; // char pointers have the strictest alignment requirement.
    };
};

// hxAllocator<0> - Capacity is set by first call to reserveStorage() and may not
// be extended.
template<typename T_>
class hxAllocator<T_, hxAllocatorDynamicCapacity> {
public:
	typedef T_ T;

    // Does not allocate until reserveStorage() is called.
    HX_CONSTEXPR_FN hxAllocator() {
        m_allocator_ = hxnull;
        m_capacity_ = 0;
    }

    // Calls hxFree() with any allocated memory.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxAllocator() {
        if (m_allocator_) {
            m_capacity_ = 0;
            hxFree(m_allocator_);
            m_allocator_ = hxnull;
        }
    }

    // Returns the number of elements of T allocated.
    HX_CONSTEXPR_FN size_t capacity() const { return m_capacity_; }

    // Returns a const array of T.
    HX_CONSTEXPR_FN const T* data() const { return m_allocator_; }

    // Returns an array of T.
    HX_CONSTEXPR_FN T* data() { return m_allocator_; }

    // Swap. Only works with Capacity_ == hxAllocatorDynamicCapacity
    HX_CONSTEXPR_FN void swap(hxAllocator& rhs) {
        hxswap(m_capacity_, rhs.m_capacity_);
        hxswap(m_allocator_, rhs.m_allocator_);
    }

protected:
    // Capacity is set by first call to reserveStorage and may not be extended.
    // size_: The number of elements of type T to allocate space for.
    // allocator_: The memory manager ID to use for allocation (default: hxMemoryAllocator_Current)
    // alignment_: The alignment to for the allocation. (default: HX_ALIGNMENT)
    HX_CONSTEXPR_FN void reserveStorage(size_t size_,
            hxMemoryAllocator allocator_=hxMemoryAllocator_Current,
            uintptr_t alignment_=HX_ALIGNMENT) {
        if (size_ <= m_capacity_) { return; }
        hxAssertRelease(m_capacity_ == 0, "allocator reallocation disallowed.");
        m_allocator_ = (T*)hxMallocExt(sizeof(T) * size_, allocator_, alignment_);
        m_capacity_ = size_;
    }

private:
    hxAllocator(const hxAllocator&) HX_DELETE_FN;
    void operator=(const hxAllocator&) HX_DELETE_FN;

    size_t m_capacity_;
    T_* m_allocator_;
};
