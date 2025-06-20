#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxallocator - Similar to std::allocator. Allows for static or dynamic
// allocation.

// A capacity value that allows for dynamic allocation.
#define hxallocator_dynamic_capacity 0u

// hxallocator<1+> - Provides static allocation when capacity is greater than
// zero.
template<typename T_, size_t Capacity_>
class hxallocator {
public:
    typedef T_ T;

    // Template specialization below should have been selected.
    HX_STATIC_ASSERT(Capacity_ > 0u, "Capacity_ > 0");

	// Initializes memory to 0xcd when HX_RELEASE < 1.
#if (HX_RELEASE) < 1
    hxallocator() : m_data_(data()) {
        ::memset(m_allocator_, 0xcd, sizeof m_allocator_);
    }
#else
    HX_CONSTEXPR_FN hxallocator() { }
#endif

	// Returns the number of elements of T allocated.
    HX_CONSTEXPR_FN size_t capacity() const { return Capacity_; }

	// Returns a reference to a const and potentially uninitialized array of T.
    const T_ (&data() const)[Capacity_] { return *reinterpret_cast<const T_(*)[Capacity_]>(m_allocator_ + 0); }

	// Returns a reference to a potentially uninitialized array of T.
    T_ (&data())[Capacity_] { return *reinterpret_cast<T_(*)[Capacity_]>(m_allocator_ + 0); }

protected:
    // Used to ensure initial capacity as reserve_storage() will not reallocate.
    // size: The number of elements of type T to allocate.

    HX_CONSTEXPR_FN void reserve_storage(size_t size_) {
		hxassert_release(size_ <= Capacity_, "allocator overflowing fixed capacity."); (void)size_;
	}

private:
	// *** The static allocator does not support swapping allocations or
	// assignments from temporaries. ***
    void swap(hxallocator& rhs) HX_DELETE_FN;
    static const size_t m_capacity_ = Capacity_;

    // Put x.m_data_ in your watch window for x or have your IDE do it for you.
#if HX_CPLUSPLUS >= 201703L
    alignas(T_) char m_allocator_[Capacity_ * sizeof(T_)];
#else
    union {
        char m_allocator_[Capacity_ * sizeof(T_)];
        double m_alignas_double;
    };
#endif
#if (HX_RELEASE) < 1
    // debug only reference to the allocator as a T[Capacity];
    T_ (&m_data_)[Capacity_];
#endif
};

// hxallocator<0> - Capacity is set by first call to reserve_storage() and may
// not be extended.
template<typename T_>
class hxallocator<T_, hxallocator_dynamic_capacity> {
public:
	typedef T_ T;

    // Does not allocate until reserve_storage() is called.
    HX_CONSTEXPR_FN hxallocator() {
        m_data_ = hxnull;
        m_capacity_ = 0u;
    }

    // Calls hxfree() with any allocated memory.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxallocator() {
        if (m_data_) {
            m_capacity_ = 0u;
            hxfree(m_data_);
            m_data_ = hxnull;
        }
    }

    // Returns the number of elements of T allocated.
    HX_CONSTEXPR_FN size_t capacity() const { return m_capacity_; }

    // Returns a const array of T.
    HX_CONSTEXPR_FN const T_* data() const { return m_data_; }

    // Returns an array of T.
    HX_CONSTEXPR_FN T_* data() { return m_data_; }

    // Swap. Only works with Capacity_ == hxallocator_dynamic_capacity
    HX_CONSTEXPR_FN void swap(hxallocator& rhs) {
        hxswap(m_capacity_, rhs.m_capacity_);
        hxswap(m_data_, rhs.m_data_);
    }

protected:
    // Capacity is set by first call to reserve_storage and may not be extended.
    // size: The number of elements of type T to allocate space for.
    // allocator: The memory manager ID to use for allocation (default: hxmemory_allocator_Current)
    // alignment: The alignment to for the allocation. (default: HX_ALIGNMENT)
    HX_CONSTEXPR_FN void reserve_storage(size_t size_,
            hxmemory_allocator allocator_=hxmemory_allocator_Current,
            uintptr_t alignment_=HX_ALIGNMENT) {
        if (size_ <= m_capacity_) { return; }
        hxassert_release(m_capacity_ == 0, "allocator reallocation disallowed.");
        m_data_ = (T_*)hxmalloc_ext(sizeof(T_) * size_, allocator_, alignment_);
        m_capacity_ = size_;
    }

private:
    hxallocator(const hxallocator&) HX_DELETE_FN;
    void operator=(const hxallocator&) HX_DELETE_FN;

    size_t m_capacity_;
    T_* m_data_;
};
