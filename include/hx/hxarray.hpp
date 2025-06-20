#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxallocator.hpp>

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
#include <initializer_list>
#endif

// hxarray - Implements some of std::vector. Requires T to have a default
// constructor.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxarray : public hxallocator<T_, capacity_> {
public:
    typedef T_ T; // Contained type.
    typedef T_* iterator; // Random access iterator.
    typedef const T_* const_iterator; // Const random access iterator.

    // Constructs an empty array with a capacity of Capacity. m_end_ will be 0
    // if Capacity is 0.
    hxconstexpr_fn explicit hxarray()
        : hxallocator<T_, capacity_>(), m_end_(this->data()) { }

    // Constructs an array of a given size using T_'s default constructor.
    // - size: Sets array size as if resize(size) were called.
    hxconstexpr_fn explicit hxarray(size_t size_)
            : hxallocator<T_, capacity_>(), m_end_(this->data()) {
        this->resize(size_);
    }

    // Constructs an array of a given size by making copies of t.
    // - size: Sets array size as if resize(size, t) were called.
    // - t: The const T& to be duplicated.
    hxconstexpr_fn explicit hxarray(size_t size_, const T_& t_)
            : hxallocator<T_, capacity_>(), m_end_(this->data()) {
        this->resize(size_, t_);
    }

    // Copy constructs an array. Non-explicit to allow assignment constructor.
    // rhs - A non-temporary Array<T>.
    hxconstexpr_fn hxarray(const hxarray& rhs_) : hxallocator<T_, capacity_>() {
        m_end_ = this->data();
        this->assign(rhs_.cbegin(), rhs_.cend());
    }

#if HX_CPLUSPLUS >= 201103L
    // Copy construct from temporary. Only works with capacity_ == hxallocator_dynamic_capacity
    // rhs - A temporary Array<T>.
    hxconstexpr_fn hxarray(hxarray&& rhs_) : hxarray() {
        this->swap(rhs_);
    }
#endif

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
    // Pass values of std::initializer_list as initializers to an array of T.
    // rhs - A std::initializer_list<rhs_t>.
    template <typename rhs_t_>
    hxconstexpr_fn hxarray(std::initializer_list<rhs_t_> list_) : hxarray() {
        this->assign(list_.begin(), list_.end());
    }
#endif
    // Copy constructs an array from the elements of a container with random
    // access iterators.
    // rhs - Any container implementing begin and end.
    template <typename rhs_t_>
    hxconstexpr_fn hxarray(const rhs_t_& rhs_) : hxallocator<T_, capacity_>() {
        m_end_ = this->data();
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Destructs the array and destroys all elements.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxarray() {
        this->destruct_(this->data(), m_end_);
    }

    // Assigns the contents of another hxarray to this array.
    // Standard except reallocation is disallowed.
    // rhs - A non-temporary Array<T>.
    hxconstexpr_fn void operator=(const hxarray& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

#if HX_CPLUSPLUS >= 201103L
    // Swap contents with temporary. Only works with capacity_ == hxallocator_dynamic_capacity
    // rhs - A temporary Array<T>.
    hxconstexpr_fn void operator=(hxarray&& rhs_) {
        this->swap(rhs_);
    }
#endif

    // Copies the elements of a container with random access iterators.
    // rhs - Any container implementing begin and end.
    template <typename rhs_t_>
    hxconstexpr_fn void operator=(const rhs_t_& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Returns a const_iterator to the beginning of the array.
    hxconstexpr_fn const T_* begin() const { return this->data(); }

    // Returns an iterator to the beginning of the array.
    hxconstexpr_fn T_* begin() { return this->data(); }

    // Returns a const_iterator to the beginning of the array (alias for begin()).
    hxconstexpr_fn const T_* cbegin() const { return this->data(); }

    // Returns a const_iterator to the beginning of the array (alias for begin()).
    hxconstexpr_fn const T_* cbegin() { return this->data(); }

    // Returns a const_iterator to the end of the array.
    hxconstexpr_fn const T_* end() const { return m_end_; }

    // Returns an iterator to the end of the array.
    hxconstexpr_fn T_* end() { return m_end_; }

    // Returns a const_iterator to the end of the array (alias for end()).
    hxconstexpr_fn const T_* cend() const { return m_end_; }

    // Returns a const_iterator to the end of the array (alias for end()).
    hxconstexpr_fn const T_* cend() { return m_end_; }

    // Returns a const reference to the first element in the array.
    hxconstexpr_fn const T_& front() const { hxassert(size()); return *this->data(); }

    // Returns a reference to the first element in the array.
    hxconstexpr_fn T_& front() { hxassert(size()); return *this->data(); }

    // Returns a const reference to the last element in the array.
    hxconstexpr_fn const T_& back() const { hxassert(size()); return *(m_end_ - 1); }

    // Returns a reference to the last element in the array.
    hxconstexpr_fn T_& back() { hxassert(size()); return *(m_end_ - 1); }

    // Returns a const reference to the element at the specified index.
    // - index: The index of the element.
    hxconstexpr_fn const T_& operator[](size_t index_) const {
        hxassert(index_ < this->size());
        return this->data()[index_];
    }

    // Returns a reference to the element at the specified index.
    // - index: The index of the element.
    hxconstexpr_fn T_& operator[](size_t index_) {
        hxassert(index_ < this->size());
        return this->data()[index_];
    }

    // Returns the number of elements in the array.
    hxconstexpr_fn size_t size() const {
        hxassert(!m_end_ == !this->data());
        return (size_t)(m_end_ - this->data());
    }

    // Reserves storage for at least the specified number of elements.
    // - size: The number of elements to reserve storage for.
    hxconstexpr_fn void reserve(size_t size_) {
        T_* prev = this->data();
        this->reserve_storage(size_);
        hxassertmsg(!prev || prev == this->data(), "no reallocation"); (void)prev;
        if (m_end_ == hxnull) {
            m_end_ = this->data();
        }
    }

    // Clears the array, destroying all elements.
    hxconstexpr_fn void clear() {
        destruct_(this->data(), m_end_);
        m_end_ = this->data();
    }

    // Returns true if the array is empty.
    hxconstexpr_fn bool empty() const { return m_end_ == this->data(); }

    // Returns true if the array is full.
    hxconstexpr_fn bool full() const { return m_end_ == this->data() + this->capacity(); }

    // Resizes the array to the specified size, constructing or destroying
    // elements as needed. Requires a default constructor. Integers and floats
    // will be value-initialized to zero as per the standard.
    // - size: The new size of the array.
    hxconstexpr_fn void resize(size_t size_) {
        this->reserve(size_);
        T_* end_ = this->data() + size_;
        if (size_ >= this->size()) {
            while (m_end_ != end_) {
                ::new (m_end_++) T_();
            }
        }
        else {
            this->destruct_(end_, m_end_);
        }
        m_end_ = end_;
    }

    // An overload with an initial value for new elements. Resizes the array to
    // the specified size, copy constructing or destroying elements as needed.
    // - size: The new size of the array.
    // - t: Initial value for new elements.
    hxconstexpr_fn void resize(size_t size_, const T_& t_) {
        this->reserve(size_);
        T_* end_ = this->data() + size_;
        if (size_ >= this->size()) {
            while (m_end_ != end_) {
                ::new (m_end_++) T_(t_);
            }
        }
        else {
            this->destruct_(end_, m_end_);
        }
        m_end_ = end_;
    }

    // Adds a copy of the specified element to the end of the array.
    // - t: The element to add.
    hxconstexpr_fn void push_back(const T_& t_) {
        hxassert(this->size() < this->capacity());
        ::new (m_end_++) T_(t_);
    }

    // Removes the last element from the array.
    hxconstexpr_fn void pop_back() {
        hxassert(this->size());
        (--m_end_)->~T_();
    }

    // Assigns elements from a range defined by iterators to the array.
    // - first: The beginning iterator.
    // - last: The end iterator.
    template <typename iter_t_>
    hxconstexpr_fn void assign(iter_t_ first_, iter_t_ last_) {
        this->reserve((size_t)(last_ - first_));
        T_* it_ = this->data();
        this->destruct_(it_, m_end_);
        while (first_ != last_) { ::new (it_++) T_(*first_++); }
        m_end_ = it_;
    }

    // Swap. Only works with capacity_ == hxallocator_dynamic_capacity
    // - rhs: The array to swap with.
    hxconstexpr_fn void swap(hxarray& rhs) {
        hxallocator<T_, capacity_>::swap(rhs); // *** Only hxallocator_dynamic_capacity works here. ***
        hxswap(rhs.m_end_, m_end_);
    }

    // --------------------------------------------------------------------------
    // Non-standard but useful

    // Constructs an array of T from an array of U.
    // - a: The array.
    // - Sz: Its size.
    template<typename U_, size_t size_>
    hxconstexpr_fn void assign(const U_(&a_)[size_]) { this->assign(a_ + 0, a_ + size_); }

    // Variant of emplace_back() that returns a pointer for use with placement new.
    hxconstexpr_fn void* emplace_back_unconstructed() {
        hxassert(this->size() < this->capacity());
        return (void*)m_end_++;
    }

    // Variant of erase() that moves the end element down to replace erased element.
    // - index: The index of the element to erase.
    hxconstexpr_fn void erase_unordered(size_t index_) {
        hxassert(index_ < this->size());
        T_* it_ = this->data() + index_;
        if (it_ != --m_end_) {
            *it_ = *m_end_;
        }
        m_end_->~T_();
    }

    // Variant of erase() that moves the end element down to replace the erased element.
    // - it: Pointer to the element to erase.
    hxconstexpr_fn void erase_unordered(T_* it_) {
        hxassert((size_t)(it_ - this->data()) < size());
        if (it_ != --m_end_) {
            *it_ = *m_end_;
        }
        m_end_->~T_();
    }

    // Returns true when the array is full (size equals capacity).
    hxconstexpr_fn bool full() {
        return this->size() == this->capacity();
    }

private:
    // Destroys elements in the range [first, last].
    // - first: Pointer to the beginning of the range.
    // - last: Pointer to the end of the range.
    hxconstexpr_fn void destruct_(T_* first_, T_* last_) {
        while (first_ != last_) {
            first_++->~T_();
        }
    }
    T_* m_end_;
};

// hxswap(hxarray<T>&, hxarray<T>&) - Exchanges the contents of x and y.
template<typename T_>
hxconstexpr_fn void hxswap(hxarray<T_>& x_, hxarray<T_>& y_) {
	x_.swap(y_);
}
