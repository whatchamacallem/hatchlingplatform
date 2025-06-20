#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxkey.hpp>
#include <hx/hxarray.hpp>

// hxinsertion_sort - Sorts the elements in the range [begin_, end_) in comparison
// order using the insertion sort algorithm. The end_ parameter points past the
// end of the array. Exceptions during operation are not supported. Declare your
// copy constructor and assignment operator noexcept or turn off exceptions.
// The compare parameter is a function object that returns true if the first
// argument is ordered before (i.e. is less than) the second. See hxkey_less.
// - begin: Pointer to the beginning of the range to sort.
// - end: Pointer to one past the last element in the range to sort.
// - less: Comparison function object that takes two const T_& parameters and
// returns a bool.
template<typename T_, typename Less_>
void hxinsertion_sort(T_* begin_, T_* end_, const Less_& less_) {
    if(begin_ == end_) { return; } // don't add +1 to null.

    // i points to insertion location. j points to next unsorted value.
    for (T_ *i_ = begin_, *j_ = begin_ + 1; j_ < end_; i_ = j_++) {
        if (!less_(*i_, *j_)) {
            T_ t_ = *j_;
            *j_ = *i_;
            while (begin_ < i_ && !less_(i_[-1], t_)) {
                i_[0] = i_[-1];
                --i_;
            }
            *i_ = t_;
        }
    }
}

// hxinsertion_sort (specialization) - A specialization of hxinsertion_sort using
// hxkey_less. c++98 junk.
// - begin: Pointer to the beginning of the range to sort.
// - end: Pointer to one past the last element in the range to sort.
template<typename T_>
void hxinsertion_sort(T_* begin_, T_* end_) {
    hxinsertion_sort(begin_, end_, hxkey_less<T_>);
}

// hxbinary_search - Performs a binary search in the range [first, last). Returns
// hxnull if the value is not found. Unsorted data will lead to errors.
// Non-unique values will be selected between arbitrarily.
//
// The compare parameter is a function object that returns true if the first
// argument is ordered before (i.e. is less than) the second. See hxkey_less.
// - begin: Pointer to the beginning of the range to search.
// - end: Pointer to one past the last element in the range to search.
// - val: The value to search for.
// - less: Comparison function object.
template<typename T_, typename Less_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_, const Less_& less_) {
    // don't operate on null pointer args. unallocated containers have this.
    if(begin_ == end_) { return hxnull; }

    ptrdiff_t a_ = 0;
    ptrdiff_t b_ = end_ - begin_ - 1;

    while(a_ <= b_) {
        ptrdiff_t mid_ = a_ + (b_ - a_) / 2;
        if(!less_(val_, begin_[mid_])) {
            if(!less_(begin_[mid_], val_)) {
                return begin_ + mid_;
            }
            a_ = mid_ + 1; // val is > mid.
        }
        else {
            b_ = mid_ - 1; // val is < mid.
        }
    }
    return hxnull;
}

// c++98 junk.
template<typename T_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_) {
    return hxbinary_search(begin_, end_, val_, hxkey_less<T_>);
}

// const correct wrapper
template<typename T_, typename Less_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_, const Less_& less_) {
    return hxbinary_search(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_, less_);
}

// c++98 junk.
template<typename T_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_) {
    return hxbinary_search(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_, hxkey_less<T_>);
}

// hxradix_sort_base. Operations that are independent of hxradix_sort type.
//
// See hxradix_sort<K, V> below.

class hxradix_sort_base {
public:
    HX_STATIC_ASSERT((int32_t)0x80000000u >> 31 == ~(int32_t)0, "arithmetic left shift expected");

    explicit hxradix_sort_base(uint32_t size_=0u) : m_array_() { m_array_.reserve(size_); }

    // Reserves memory for the internal array to hold at least `size_` elements.
    // - size: The number of elements to reserve memory for.
    void reserve(uint32_t size_) { m_array_.reserve(size_); }

    // Clears the internal array, removing all elements.
    void clear() { m_array_.clear(); }

    // Sorts the internal array using the provided temporary memory allocator to
    // store histograms.
    // - temp_memory: A hxmemory_allocator id.
    void sort(hxmemory_allocator temp_memory);

protected:
    // Represents a key-value pair used in radix sorting.
    class hxkey_value_pair {
    public:
        // Constructor for an 8-bit key and associated value.
        hxkey_value_pair(uint8_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

        // Constructor for a 16-bit key and associated value.
        hxkey_value_pair(uint16_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

        // Constructor for a 32-bit key and associated value.
        hxkey_value_pair(uint32_t key_, void* val_) : m_key_(key_), m_val_(val_) { }

        // Constructor for a signed 32-bit key and associated value.
        // Adjusts the key to handle signed integers correctly.
        hxkey_value_pair(int32_t key_, void* val_) : m_key_((uint32_t)(key_ ^ 0x80000000)), m_val_(val_) { }

        // Constructor for a floating-point key and associated value.
        // Adjusts the key to handle floating-point sorting correctly.
        hxkey_value_pair(float key_, void* val_)
            : m_val_(val_)
        {
            uint32_t t_;
            ::memcpy(&t_, &key_, sizeof t_);
            m_key_ = t_ ^ (uint32_t)(((int32_t)t_ >> 31) | 0x80000000);
        }

        // Comparison operator for sorting hxkey_value_pair objects by key.
        bool operator<(const hxkey_value_pair& rhs_) const { return m_key_ < rhs_.m_key_; }

        uint32_t m_key_; // The key used for sorting.
        void* m_val_;	 // The associated value.
    };

    hxarray<hxkey_value_pair> m_array_; // Internal array of key-value pairs.

private:
    hxradix_sort_base(const hxradix_sort_base&) HX_DELETE_FN;
    void operator=(const hxradix_sort_base&) HX_DELETE_FN;
};

// hxradix_sort. Sorts an array of value* by keys. K is the key and V the value.
//
// Nota bene: Keys of double, int64_t and uint64_t are not supported. Keys
// are stored as uint32_t to reduce generated code.

template<typename key_t_, class value_t_>
class hxradix_sort : public hxradix_sort_base {
public:
    typedef key_t_ key_t;   // Type of the key used for sorting.
    typedef value_t_ value_t; // Type of the value associated with the key.

    // Forward_iterator over Values. Not currently bound to std::iterator_traits
    // or std::forward_iterator_tag.
    class const_iterator {
    public:
        // Constructs a const_iterator from an hxarray<hxkey_value_pair>::const_iterator.
        const_iterator(hxarray<hxkey_value_pair>::const_iterator it_) : m_ptr_(it_) { }

        // Constructs an invalid const_iterator.
        const_iterator() : m_ptr_(hxnull) { }

        // Pre-increment operator. Moves the iterator to the next element.
        const_iterator& operator++() { ++m_ptr_; return *this; }

        // Post-increment operator. Moves the iterator to the next element and returns the previous state.
        const_iterator operator++(int) { const_iterator t_(*this); operator++(); return t_; }

        // Equality comparison operator.
        bool operator==(const const_iterator& rhs_) const { return m_ptr_ == rhs_.m_ptr_; }

        // Inequality comparison operator.
        bool operator!=(const const_iterator& rhs_) const { return m_ptr_ != rhs_.m_ptr_; }

        // Dereference operator. Returns a reference to the value pointed to by the iterator.
        const value_t_& operator*() const { return *(const value_t_*)m_ptr_->m_val_; }

        // Arrow operator. Returns a pointer to the value pointed to by the iterator.
        const value_t_* operator->() const { return (const value_t_*)m_ptr_->m_val_; }

    protected:
        hxarray<hxkey_value_pair>::const_iterator m_ptr_; // Internal pointer to the current element.
    };

    // Iterator that can be cast to a const_iterator.
    class iterator : public const_iterator {
    public:
        // Constructs an iterator from an hxarray<hxkey_value_pair>::iterator.
        iterator(hxarray<hxkey_value_pair>::iterator it_) : const_iterator(it_) { }

        // Constructs an invalid iterator.
        iterator() { }

        // Pre-increment operator. Moves the iterator to the next element.
        iterator& operator++() { const_iterator::operator++(); return *this; }

        // Post-increment operator. Moves the iterator to the next element and returns the previous state.
        iterator operator++(int) { iterator t_(*this); const_iterator::operator++(); return t_; }

        // Dereference operator. Returns a reference to the value pointed to by the iterator.
        value_t_& operator*() const { return *(value_t_*)this->m_ptr_->m_val_; }

        // Arrow operator. Returns a pointer to the value pointed to by the iterator.
        value_t_* operator->() const { return (value_t_*)this->m_ptr_->m_val_; }
    };

    explicit hxradix_sort(uint32_t size_=0u) : hxradix_sort_base(size_) { }

    // Accesses the value at the specified index (const version).
    const value_t_& operator[](uint32_t index_) const { return *(value_t_*)m_array_[index_].m_val_; }

    // Accesses the value at the specified index (non-const version).
    value_t_& operator[](uint32_t index_) { return *(value_t_*)m_array_[index_].m_val_; }

    // Returns a pointer to the value at the specified index (const version).
    const value_t_* get(uint32_t index_) const { return (value_t_*)m_array_[index_].m_val_; }

    // Returns a pointer to the value at the specified index (non-const version).
    value_t_* get(uint32_t index_) { return (value_t_*)m_array_[index_].m_val_; }

    // Returns a const_iterator to the beginning of the array.
    const_iterator begin() const { return const_iterator(m_array_.cbegin()); }

    // Returns an iterator to the beginning of the array.
    iterator begin() { return iterator(m_array_.begin()); }

    // Returns a const_iterator to the beginning of the array (const version).
    const_iterator cbegin() const { return const_iterator(m_array_.cbegin()); }

    // Returns a const_iterator to the beginning of the array (non-const version).
    const_iterator cbegin() { return const_iterator(m_array_.cbegin()); }

    // Returns a const_iterator to the end of the array.
    const_iterator end() const { return const_iterator(m_array_.cend()); }

    // Returns an iterator to the end of the array.
    iterator end() { return iterator(m_array_.end()); }

    // Returns a const_iterator to the end of the array (const version).
    const_iterator cend() const { return const_iterator(m_array_.cend()); }

    // Returns a const_iterator to the end of the array (non-const version).
    const_iterator cend() { return const_iterator(m_array_.cend()); }

    // Returns the number of elements in the array.
    uint32_t size() const { return m_array_.size(); }

    // Returns true if the array is empty, false otherwise.
    bool empty() const { return m_array_.empty(); }

    // Returns true if the array is full, false otherwise.
    bool full() const { return m_array_.full(); }

    // Adds a key and value pointer to the array. Ownership is not taken.
    // - key: The key used for sorting.
    // - val: Pointer to the value associated with the key.
    void insert(key_t_ key_, value_t_* val_) {
        hxassertrelease(!this->full(), "cannot reallocate");

        // This radix sort uses void* to avoid template bloat. The casts are not
        // required by the standard, but fix -Wcast-qual for a const value_t_.
        ::new(m_array_.emplace_back_unconstructed())
            hxkey_value_pair(key_, const_cast<void*>((const void*)val_));
    }

private:
    hxradix_sort(const hxradix_sort&) HX_DELETE_FN;
    void operator=(const hxradix_sort&) HX_DELETE_FN;
};
