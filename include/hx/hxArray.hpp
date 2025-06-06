#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxAllocator.hpp>

#if HX_CPLUSPLUS >= 201103L
#include <initializer_list>
#endif

// ----------------------------------------------------------------------------
// hxArray
//
// Implements some of std::vector.  Requires T to have a default constructor.

template<typename T_, size_t Capacity_=hxAllocatorDynamicCapacity>
class hxArray : private hxAllocator<T_, Capacity_> {
public:
    typedef T_ T; // value type
    typedef T* iterator; // Random access iterator.
    typedef const T* constIterator; // Const random access iterator.
    typedef hxAllocator<T_, Capacity_> Allocator;

    // Constructs an empty array with a capacity of Capacity. m_end will be 0
    // if Capacity is 0.
    HX_CONSTEXPR_FN explicit hxArray() : Allocator() { m_end = this->getStorage(); }

    // Copy constructs an array. Non-explicit to allow assignment constructor.
    HX_CONSTEXPR_FN hxArray(const hxArray& rhs_) : Allocator() {
        m_end = this->getStorage();
        this->assign(rhs_.cBegin(), rhs_.cEnd());
    }

    // C++11 constructors
#if HX_CPLUSPLUS >= 201103L
    // Copy construct from temporary.  Only works with Capacity_ == hxAllocatorDynamicCapacity
    HX_CONSTEXPR_FN hxArray(hxArray&& rhs_) : hxArray() {
        this->swap(rhs_);
    }

    // Pass values of std::initializer_list as initializers to an array of T.
    template <typename Rhs>
    HX_CONSTEXPR_FN hxArray(std::initializer_list<Rhs> list_) : hxArray() {
        this->assign(list_.begin(), list_.end());
    }
#endif

    // Copy constructs an array from a container with begin() and end() methods and
    // a random access iterator. Expects `rhs_` to be a reference to a container.
    template <typename Rhs>
    HX_CONSTEXPR_FN hxArray(const Rhs& rhs_) : Allocator() {
        m_end = this->getStorage();
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Destructs the array and destroys all elements.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxArray() {
        this->destruct_(this->getStorage(), m_end);
    }

    // Assigns the contents of another hxArray to this array.
    // Standard except reallocation is disallowed. Expects `rhs_` to be a reference
    // to another hxArray.
    HX_CONSTEXPR_FN void operator=(const hxArray& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

#if HX_CPLUSPLUS >= 201103L
    // Swap contents with temporary.  Only works with Capacity_ == hxAllocatorDynamicCapacity
    HX_CONSTEXPR_FN void operator=(hxArray&& rhs_) {
        this->swap(rhs_);
    }
#endif

    // Copies the elements of a container with begin() and end() methods and a random
    // access iterator. Expects `rhs_` to be a reference to a container.
    template <typename Rhs>
    HX_CONSTEXPR_FN void operator=(const Rhs& rhs_) {
        this->assign(rhs_.begin(), rhs_.end());
    }

    // Returns a const iterator to the beginning of the array.
    HX_CONSTEXPR_FN const T* begin() const { return this->getStorage(); }

    // Returns an iterator to the beginning of the array.
    HX_CONSTEXPR_FN T* begin() { return this->getStorage(); }

    // Returns a const iterator to the beginning of the array (alias for begin()).
    HX_CONSTEXPR_FN const T* cBegin() const { return this->getStorage(); }

    // Returns a const iterator to the beginning of the array (alias for begin()).
    HX_CONSTEXPR_FN const T* cBegin() { return this->getStorage(); }

    // Returns a const iterator to the end of the array.
    HX_CONSTEXPR_FN const T* end() const { return m_end; }

    // Returns an iterator to the end of the array.
    HX_CONSTEXPR_FN T* end() { return m_end; }

    // Returns a const iterator to the end of the array (alias for end()).
    HX_CONSTEXPR_FN const T* cEnd() const { return m_end; }

    // Returns a const iterator to the end of the array (alias for end()).
    HX_CONSTEXPR_FN const T* cEnd() { return m_end; }

    // Returns a const reference to the first element in the array.
    HX_CONSTEXPR_FN const T& front() const { hxAssert(size()); return *this->getStorage(); }

    // Returns a reference to the first element in the array.
    HX_CONSTEXPR_FN T& front() { hxAssert(size()); return *this->getStorage(); }

    // Returns a const reference to the last element in the array.
    HX_CONSTEXPR_FN const T& back() const { hxAssert(size()); return *(m_end - 1); }

    // Returns a reference to the last element in the array.
    HX_CONSTEXPR_FN T& back() { hxAssert(size()); return *(m_end - 1); }

    // Returns a const reference to the element at the specified index.
    // Expects `index_` to be the index of the element.
    HX_CONSTEXPR_FN const T& operator[](size_t index_) const {
        hxAssert(index_ < this->size());
        return this->getStorage()[index_];
    }

    // Returns a reference to the element at the specified index.
    // Expects `index_` to be the index of the element.
    HX_CONSTEXPR_FN T& operator[](size_t index_) {
        hxAssert(index_ < this->size());
        return this->getStorage()[index_];
    }

    // Returns the number of elements in the array.
    HX_CONSTEXPR_FN size_t size() const {
        hxAssert(!m_end == !this->getStorage());
        return (size_t)(m_end - this->getStorage());
    }

    // Reserves storage for at least the specified number of elements.
    // Expects `size_` to be the number of elements to reserve storage for.
    HX_CONSTEXPR_FN void reserve(size_t size_) {
        T* prev = this->getStorage();
        this->reserveStorage(size_);
        hxAssertMsg(!prev || prev == this->getStorage(), "no reallocation"); (void)prev;
        if (m_end == hxnull) {
            m_end = this->getStorage();
        }
    }

    // Returns the capacity of the array.
    HX_CONSTEXPR_FN size_t capacity() const { return this->getCapacity(); }

    // Clears the array, destroying all elements.
    HX_CONSTEXPR_FN void clear() {
        destruct_(this->getStorage(), m_end);
        m_end = this->getStorage();
    }

    // Returns true if the array is empty.
    HX_CONSTEXPR_FN bool empty() const { return m_end == this->getStorage(); }

    // Resizes the array to the specified size, constructing or destroying elements as needed.
    // Expects `size_` to be the new size of the array.
    HX_CONSTEXPR_FN void resize(size_t size_) {
        this->reserve(size_);
        if (size_ >= this->size()) {
            this->construct_(m_end, this->getStorage() + size_);
        }
        else {
            this->destruct_(this->getStorage() + size_, m_end);
        }
        m_end = this->getStorage() + size_;
    }

    // Adds a copy of the specified element to the end of the array.
    // Expects `t_` to be the element to add.
    HX_CONSTEXPR_FN void pushBack(const T& t_) {
        hxAssert(this->size() < this->capacity());
        ::new (m_end++) T(t_);
    }

    // Removes the last element from the array.
    HX_CONSTEXPR_FN void popBack() {
        hxAssert(this->size());
        (--m_end)->~T();
    }

    // Returns a const pointer to the array's data.
    HX_CONSTEXPR_FN const T* data() const { return this->getStorage(); }

    // Returns a pointer to the array's data.
    HX_CONSTEXPR_FN T* data() { return this->getStorage(); }

    // Assigns elements from a range defined by iterators to the array.
    // Expects `first_` to be the beginning iterator and `last_` to be the end iterator.
    template <typename Iter>
    HX_CONSTEXPR_FN void assign(Iter first_, Iter last_) {
        this->reserve((size_t)(last_ - first_));
        T* it_ = this->getStorage();
        this->destruct_(it_, m_end);
        while (first_ != last_) { ::new (it_++) T(*first_++); }
        m_end = it_;
    }

    // Swap.  Only works with Capacity_ == hxAllocatorDynamicCapacity
    HX_CONSTEXPR_FN void swap(hxArray& rhs) {
        Allocator::swap(rhs); // *** Only hxAllocatorDynamicCapacity works here. ***
        hxswap(rhs.m_end, m_end);
    }

    // --------------------------------------------------------------------------
    // Non-standard but useful

    // Constructs an array of T from an array of T2.
    // Expects `a_` to be the array and `Sz_` to be its size.
    template<typename T2_, size_t Sz_>
    HX_CONSTEXPR_FN void assign(const T2_(&a_)[Sz_]) { this->assign(a_ + 0, a_ + Sz_); }

    // Variant of emplace_back() that returns a pointer for use with placement new.
    HX_CONSTEXPR_FN void* emplaceBackUnconstructed() {
        hxAssert(this->size() < this->capacity());
        return (void*)m_end++;
    }

    // Variant of erase() that moves the end element down to replace erased element.
    // Expects `index_` to be the index of the element to erase.
    HX_CONSTEXPR_FN void eraseUnordered(size_t index_) {
        hxAssert(index_ < this->size());
        T* it_ = this->getStorage() + index_;
        if (it_ != --m_end) {
            *it_ = *m_end;
        }
        m_end->~T();
    }

    // Variant of erase() that moves the end element down to replace the erased element.
    // Expects `it_` to be a pointer to the element to erase.
    HX_CONSTEXPR_FN void eraseUnordered(T* it_) {
        hxAssert((size_t)(it_ - this->getStorage()) < size());
        if (it_ != --m_end) {
            *it_ = *m_end;
        }
        m_end->~T();
    }

    // Returns true when the array is full (size equals capacity).
    HX_CONSTEXPR_FN bool full() {
        return this->size() == this->capacity();
    }

private:
    // Constructs elements in the range [first_, last_]. Expects `first_` and `last_`
    // to be pointers defining the range.
    HX_CONSTEXPR_FN void construct_(T* first_, T* last_) {
        while (first_ != last_) {
            ::new (first_++) T;
        }
    }

    // Destroys elements in the range [first_, last_]. Expects `first_` and `last_`
    // to be pointers defining the range.
    HX_CONSTEXPR_FN void destruct_(T* first_, T* last_) {
        while (first_ != last_) {
            first_++->~T();
        }
    }
    T* m_end;
};
