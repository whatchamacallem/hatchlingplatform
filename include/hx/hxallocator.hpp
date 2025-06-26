#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxallocator - Similar to std::allocator. Allows for static or dynamic
// allocation.

/// A capacity value that allows for dynamic allocation.
#define hxallocator_dynamic_capacity 0u

/// hxallocator<1+> - Provides static allocation when capacity is greater than
/// zero.
template<typename T_, size_t fixed_capacity_>
class hxallocator {
public:
    typedef T_ value_t;

    /// Template specialization below should have been selected.
    hxstatic_assert(fixed_capacity_ > 0u, "Fixed capacity must be > 0");

	/// Initializes memory to 0xcd when HX_RELEASE < 1.
#if (HX_RELEASE) < 1
    hxallocator() : m_data_(data()) {
        ::memset(m_allocator_, 0xcd, sizeof m_allocator_);
    }
#else
    hxconstexpr_fn hxallocator(void) { }
#endif

	/// Returns the number of elements of T allocated.
    hxconstexpr_fn size_t capacity(void) const { return fixed_capacity_; }

	/// Returns a reference to a const and potentially uninitialized array of T.
    const T_ (&data() const)[fixed_capacity_] {
        return *reinterpret_cast<const T_(*)[fixed_capacity_]>(m_allocator_ + 0);
    }

	/// Returns a reference to a potentially uninitialized array of T.
    T_ (&data())[fixed_capacity_] {
        return *reinterpret_cast<T_(*)[fixed_capacity_]>(m_allocator_ + 0);
    }

protected:
    /// Used to ensure initial capacity as reserve_storage() will not reallocate.
    /// Provided for interface compatibility with the dynamic allocator.
    /// - size: The number of elements of type T to ensure are available.
    /// - allocator: Ignored.
    /// - alignment: The alignment of the allocator is checked against this.
    hxconstexpr_fn void reserve_storage(size_t size_,
            hxsystem_allocator_t allocator_=hxsystem_allocator_current,
            uintptr_t alignment_=HX_ALIGNMENT) {
                (void)size_; (void)allocator_; (void)alignment_;
                hxassertmsg(size_ <= fixed_capacity_, "overflowing_fixed_capacity");
                hxassertmsg(((alignment_ - 1u) & (uintptr_t)this) == 0u,
                    "alignment_error static allocation");
	}

private:
	// *** The static allocator does not support swapping allocations or
	// assignments from temporaries. ***
    void swap(hxallocator& rhs) hxdelete_fn;
    static const size_t m_capacity_ = fixed_capacity_;

    /// Put x.m_data_ in your watch window for x or have your IDE do it for you.
#if HX_CPLUSPLUS >= 201703L
    alignas(T_) char m_allocator_[fixed_capacity_ * sizeof(T_)];
#else
    union {
        char m_allocator_[fixed_capacity_ * sizeof(T_)];
        double m_alignas_double;
    };
#endif
#if (HX_RELEASE) < 1
    /// debug only reference to the allocator as a T[fixed_capacity];
    T_ (&m_data_)[fixed_capacity_];
#endif
};

/// hxallocator<0> - Capacity is set by first call to reserve_storage() and may
/// not be extended.
template<typename T_>
class hxallocator<T_, hxallocator_dynamic_capacity> {
public:
    typedef T_ value_t;

    /// Does not allocate until reserve_storage() is called.
    hxconstexpr_fn hxallocator(void) {
        m_data_ = hxnull;
        m_capacity_ = 0u;
    }

    /// Calls hxfree() with any allocated memory.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxallocator(void) {
        if (m_data_) {
            m_capacity_ = 0u;
            hxfree(m_data_);
            m_data_ = hxnull;
        }
    }

    /// Returns the number of elements of T allocated.
    hxconstexpr_fn size_t capacity(void) const { return m_capacity_; }

    /// Returns a const array of T.
    hxconstexpr_fn const T_* data(void) const { return m_data_; }

    /// Returns an array of T.
    hxconstexpr_fn T_* data(void) { return m_data_; }

    /// Swap. Only works with fixed_capacity_ == hxallocator_dynamic_capacity
    hxconstexpr_fn void swap(hxallocator& rhs) {
        hxswap(m_capacity_, rhs.m_capacity_);
        hxswap(m_data_, rhs.m_data_);
    }

protected:
    /// Capacity is set by first call to reserve_storage and may not be extended.
    /// - size: The number of elements of type T to allocate space for.
    /// - allocator: The memory manager ID to use for allocation (default: hxsystem_allocator_current)
    /// - alignment: The alignment to for the allocation. (default: HX_ALIGNMENT)
    hxconstexpr_fn void reserve_storage(size_t size_,
            hxsystem_allocator_t allocator_=hxsystem_allocator_current,
            uintptr_t alignment_=HX_ALIGNMENT) {
        if (size_ <= m_capacity_) { return; }
        hxassertrelease(m_capacity_ == 0, "reallocation_disallowed");
        m_data_ = (T_*)hxmalloc_ext(sizeof(T_) * size_, allocator_, alignment_);
        m_capacity_ = size_;
    }

private:
    hxallocator(const hxallocator&) hxdelete_fn;
    void operator=(const hxallocator&) hxdelete_fn;

    size_t m_capacity_;
    T_* m_data_;
};
