#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxAllocator.hpp>

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
#include <initializer_list>
#endif

// hxArray - Implements some of std::vector. Requires T to have a default
// constructor.
template<typename T_, size_t Capacity_=hxAllocatorDynamicCapacity>
class hxArray : public hxAllocator<T_, Capacity_> {
public:
    typedef T_ T; // Contained type.
    typedef T_* iterator; // Random access iterator.
    typedef const T_* constIterator; // Const random access iterator.

    // Constructs an empty array with a capacity of Capacity. m_end_ will be 0
    // if Capacity is 0.
    HX_CONSTEXPR_FN explicit hxArray() : hxAllocator<T_, Capacity_>() { m_end_ = this->data(); }

    // Copy constructs an array. Non-explicit to allow assignment constructor.
    // rhs - A non-temporary Array<T>.
    HX_CONSTEXPR_FN hxArray(const hxArray& rhs_) : hxAllocator<T_, Capacity_>() {
        m_end_ = this->data();
        this->assign(rhs_.cBegin(), rhs_.cEnd());
    }

#if HX_CPLUSPLUS >= 201103L
    // Copy construct from temporary. Only works with Capacity_ == hxAllocatorDynamicCapacity
    // rhs - A temporary Array<T>.
    HX_CONSTEXPR_FN hxArray(hxArray&& rhs_) : hxArray() {
        this->swap(rhs_);
    }
#endif

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
    // Pass values of std::initializer_list as initializers to an array of T.
    // rhs - A std::initializer_list<Rhs>.
    template <typename Rhs_>
    HX_CONSTEXPR_FN hxArray(std::initializer_list<Rhs_> list_) : hxArray() {
        this->assign(list_.begin(), list_.end());
    }
#endif
    // Copy constructs an array from the elements of a container with random
    // access iterators.
    // rhs - Any container implementing begin and end.
    template <typename Rhs_>
    HX_CONSTEXPR_FN hxArray(const Rhs_& rhs_) : hxAllocator<T_, Capacity_>() {
        m_end_ = this->data();
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Destructs the array and destroys all elements.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxArray() {
        this->destruct_(this->data(), m_end_);
    }

    // Assigns the contents of another hxArray to this array.
    // Standard except reallocation is disallowed.
    // rhs - A non-temporary Array<T>.
    HX_CONSTEXPR_FN void operator=(const hxArray& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

#if HX_CPLUSPLUS >= 201103L
    // Swap contents with temporary. Only works with Capacity_ == hxAllocatorDynamicCapacity
    // rhs - A temporary Array<T>.
    HX_CONSTEXPR_FN void operator=(hxArray&& rhs_) {
        this->swap(rhs_);
    }
#endif

    // Copies the elements of a container with random access iterators.
    // rhs - Any container implementing begin and end.
    template <typename Rhs_>
    HX_CONSTEXPR_FN void operator=(const Rhs_& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Returns a constIterator to the beginning of the array.
    HX_CONSTEXPR_FN const T_* begin() const { return this->data(); }

    // Returns an iterator to the beginning of the array.
    HX_CONSTEXPR_FN T_* begin() { return this->data(); }

    // Returns a constIterator to the beginning of the array (alias for begin()).
    HX_CONSTEXPR_FN const T_* cBegin() const { return this->data(); }

    // Returns a constIterator to the beginning of the array (alias for begin()).
    HX_CONSTEXPR_FN const T_* cBegin() { return this->data(); }

    // Returns a constIterator to the end of the array.
    HX_CONSTEXPR_FN const T_* end() const { return m_end_; }

    // Returns an iterator to the end of the array.
    HX_CONSTEXPR_FN T_* end() { return m_end_; }

    // Returns a constIterator to the end of the array (alias for end()).
    HX_CONSTEXPR_FN const T_* cEnd() const { return m_end_; }

    // Returns a constIterator to the end of the array (alias for end()).
    HX_CONSTEXPR_FN const T_* cEnd() { return m_end_; }

    // Returns a const reference to the first element in the array.
    HX_CONSTEXPR_FN const T_& front() const { hxAssert(size()); return *this->data(); }

    // Returns a reference to the first element in the array.
    HX_CONSTEXPR_FN T_& front() { hxAssert(size()); return *this->data(); }

    // Returns a const reference to the last element in the array.
    HX_CONSTEXPR_FN const T_& back() const { hxAssert(size()); return *(m_end_ - 1); }

    // Returns a reference to the last element in the array.
    HX_CONSTEXPR_FN T_& back() { hxAssert(size()); return *(m_end_ - 1); }

    // Returns a const reference to the element at the specified index.
    // - index: The index of the element.
    HX_CONSTEXPR_FN const T_& operator[](size_t index_) const {
        hxAssert(index_ < this->size());
        return this->data()[index_];
    }

    // Returns a reference to the element at the specified index.
    // - index: The index of the element.
    HX_CONSTEXPR_FN T_& operator[](size_t index_) {
        hxAssert(index_ < this->size());
        return this->data()[index_];
    }

    // Returns the number of elements in the array.
    HX_CONSTEXPR_FN size_t size() const {
        hxAssert(!m_end_ == !this->data());
        return (size_t)(m_end_ - this->data());
    }

    // Reserves storage for at least the specified number of elements.
    // - size: The number of elements to reserve storage for.
    HX_CONSTEXPR_FN void reserve(size_t size_) {
        T_* prev = this->data();
        this->reserveStorage(size_);
        hxAssertMsg(!prev || prev == this->data(), "no reallocation"); (void)prev;
        if (m_end_ == hxnull) {
            m_end_ = this->data();
        }
    }

    // Clears the array, destroying all elements.
    HX_CONSTEXPR_FN void clear() {
        destruct_(this->data(), m_end_);
        m_end_ = this->data();
    }

    // Returns true if the array is empty.
    HX_CONSTEXPR_FN bool empty() const { return m_end_ == this->data(); }

    // Returns true if the array is full.
    HX_CONSTEXPR_FN bool full() const { return m_end_ == this->data() + this->capacity(); }

    // Resizes the array to the specified size, constructing or destroying elements as needed.
    // - size: The new size of the array.
    HX_CONSTEXPR_FN void resize(size_t size_) {
        this->reserve(size_);
        if (size_ >= this->size()) {
            this->construct_(m_end_, this->data() + size_);
        }
        else {
            this->destruct_(this->data() + size_, m_end_);
        }
        m_end_ = this->data() + size_;
    }

    // Adds a copy of the specified element to the end of the array.
    // - t: The element to add.
    HX_CONSTEXPR_FN void pushBack(const T_& t_) {
        hxAssert(this->size() < this->capacity());
        ::new (m_end_++) T_(t_);
    }

    // Removes the last element from the array.
    HX_CONSTEXPR_FN void popBack() {
        hxAssert(this->size());
        (--m_end_)->~T_();
    }

    // Assigns elements from a range defined by iterators to the array.
    // - first: The beginning iterator.
    // - last: The end iterator.
    template <typename Iter>
    HX_CONSTEXPR_FN void assign(Iter first_, Iter last_) {
        this->reserve((size_t)(last_ - first_));
        T_* it_ = this->data();
        this->destruct_(it_, m_end_);
        while (first_ != last_) { ::new (it_++) T_(*first_++); }
        m_end_ = it_;
    }

    // Swap. Only works with Capacity_ == hxAllocatorDynamicCapacity
    // - rhs: The array to swap with.
    HX_CONSTEXPR_FN void swap(hxArray& rhs) {
        hxAllocator<T_, Capacity_>::swap(rhs); // *** Only hxAllocatorDynamicCapacity works here. ***
        hxswap(rhs.m_end_, m_end_);
    }

    // --------------------------------------------------------------------------
    // Non-standard but useful

    // Constructs an array of T from an array of T2.
    // - a: The array.
    // - Sz: Its size.
    template<typename T2_, size_t Sz_>
    HX_CONSTEXPR_FN void assign(const T2_(&a_)[Sz_]) { this->assign(a_ + 0, a_ + Sz_); }

    // Variant of emplace_back() that returns a pointer for use with placement new.
    HX_CONSTEXPR_FN void* emplaceBackUnconstructed() {
        hxAssert(this->size() < this->capacity());
        return (void*)m_end_++;
    }

    // Variant of erase() that moves the end element down to replace erased element.
    // - index: The index of the element to erase.
    HX_CONSTEXPR_FN void eraseUnordered(size_t index_) {
        hxAssert(index_ < this->size());
        T_* it_ = this->data() + index_;
        if (it_ != --m_end_) {
            *it_ = *m_end_;
        }
        m_end_->~T_();
    }

    // Variant of erase() that moves the end element down to replace the erased element.
    // - it: Pointer to the element to erase.
    HX_CONSTEXPR_FN void eraseUnordered(T_* it_) {
        hxAssert((size_t)(it_ - this->data()) < size());
        if (it_ != --m_end_) {
            *it_ = *m_end_;
        }
        m_end_->~T_();
    }

    // Returns true when the array is full (size equals capacity).
    HX_CONSTEXPR_FN bool full() {
        return this->size() == this->capacity();
    }

private:
    // Constructs elements in the range [first, last].
    // - first: Pointer to the beginning of the range.
    // - last: Pointer to the end of the range.
    HX_CONSTEXPR_FN void construct_(T_* first_, T_* last_) {
        while (first_ != last_) {
            ::new (first_++) T_;
        }
    }

    // Destroys elements in the range [first, last].
    // - first: Pointer to the beginning of the range.
    // - last: Pointer to the end of the range.
    HX_CONSTEXPR_FN void destruct_(T_* first_, T_* last_) {
        while (first_ != last_) {
            first_++->~T_();
        }
    }
    T_* m_end_;
};

// hxswap(hxArray<T>&, hxArray<T>&) - Exchanges the contents of x and y using a
// temporary.
template<typename T_>
HX_CONSTEXPR_FN void hxswap(hxArray<T_>& x_, hxArray<T_>& y_) {
	x_.swap(y_);
}
