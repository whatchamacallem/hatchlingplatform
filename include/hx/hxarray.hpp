#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxallocator.hpp>
#include <hx/hxkey.hpp>

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
#include <initializer_list>
#endif

/// `hxarray` - Another vector class. Uses raw pointers as an iterator type so
/// that you get compile errors and a debug experience that is in plain C++
/// instead of the std. There are asserts. Please run a memory sanitizer and
/// an undefined behavior sanitizer too.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxarray : public hxallocator<T_, capacity_> {
public:
    typedef T_ value_t;
    typedef T_* iterator; /// Random access iterator.
    typedef const T_* const_iterator; /// Const random access iterator.

    /// Constructs an empty array with a capacity of Capacity. m_end_ will be 0
    /// if Capacity is 0.
    hxconstexpr_fn explicit hxarray(void)
        : hxallocator<T_, capacity_>(), m_end_(this->data()) { }

    /// Constructs an array of a given size using T_'s default constructor.
    /// - `size` : Sets array size as if resize(size) were called.
    hxconstexpr_fn explicit hxarray(size_t size_)
            : hxallocator<T_, capacity_>(), m_end_(this->data()) {
        this->resize(size_);
    }

    /// Constructs an array of a given size by making copies of t.
    /// - `size` : Sets array size as if resize(size, t) were called.
    /// - `t` : The const T& to be duplicated.
    hxconstexpr_fn explicit hxarray(size_t size_, const T_& t_)
            : hxallocator<T_, capacity_>(), m_end_(this->data()) {
        this->resize(size_, t_);
    }

    /// Copy constructs an array. Non-explicit to allow assignment constructor.
    /// - `x` : A non-temporary Array<T>.
    hxconstexpr_fn hxarray(const hxarray& x_) : hxallocator<T_, capacity_>() {
        m_end_ = this->data();
        this->assign(x_.cbegin(), x_.cend());
    }

#if HX_CPLUSPLUS >= 201103L
    /// Copy construct from temporary. Only works with capacity_ ==
    /// hxallocator_dynamic_capacity
    /// - `x` : A temporary Array<T>.
    hxconstexpr_fn hxarray(hxarray&& x_) : hxarray() {
        this->swap(x_);
    }
#endif

#if HX_CPLUSPLUS >= 201103L && HX_HOSTED
    /// Pass values of std::initializer_list as initializers to an array of T.
    /// WARNING: This constructor will override the other constructors when
    /// uniform initialization is used.  E.g. hxarry<int>x{1,2} is an array
    /// containing {1,2} and hxarry<int>x(1,2) is the array containing {2}.
    /// - `x` : A std::initializer_list<x_t>.
    template <typename x_t_>
    hxconstexpr_fn hxarray(std::initializer_list<x_t_> list_) : hxarray() {
        this->assign(list_.begin(), list_.end());
    }
#endif

    /// Copy constructs an array from the elements of a container with random
    /// access iterators. (Non-standard.)
    /// - `x` : Any container implementing begin and end.
    template <typename x_t_>
    hxconstexpr_fn hxarray(const x_t_& x_) : hxallocator<T_, capacity_>() {
        m_end_ = this->data();
        this->assign(x_.begin(), x_.end());
    }

    /// Destructs the array and destroys all elements.
#if HX_CPLUSPLUS >= 202002L
    constexpr
#endif
    ~hxarray(void) {
        this->destruct_(this->data(), m_end_);
    }

    /// Appends an element.  (Non-standard.)
    /// Vector math is not a goal so this should not end up overloaded.
    /// - `x` : An object to append. Not a temporary.
    hxconstexpr_fn void operator+=(const T_& x_) {
        ::new(this->emplace_back_unconstructed()) T_(x_);
    }

    /// Appends the contents of another array.  (Non-standard, from Python.)
    /// Vector math is not a goal so this should not end up overloaded.
    /// - `x` : Another array. Not a temporary.
    hxconstexpr_fn void operator+=(const hxarray& x_) {
        for(const T_ *it_ = x_.begin(), *end_ = x_.end(); it_ != end_; ++it_) {
            ::new(this->emplace_back_unconstructed()) T_(*it_);
        }
    }

    /// Assigns the contents of another hxarray to this array.
    /// Standard except reallocation is disallowed.
    /// - `x` : A non-temporary Array<T>.
    hxconstexpr_fn void operator=(const hxarray& x_) {
        this->assign(x_.begin(), x_.end());
    }

#if HX_CPLUSPLUS >= 201103L
    /// Swap contents with temporary. Only works with capacity_ == hxallocator_dynamic_capacity
    /// - `x` : A temporary Array<T>.
    hxconstexpr_fn void operator=(hxarray&& x_) {
        this->swap(x_);
    }
#endif

    /// Copies the elements of a container with random access iterators.
    /// - `x` : Any container implementing begin and end.
    template <typename x_t_>
    hxconstexpr_fn void operator=(const x_t_& x_) {
        this->assign(x_.begin(), x_.end());
    }

    /// Returns a const reference to the element at the specified index.
    /// - `index` : The index of the element.
    hxconstexpr_fn const T_& operator[](size_t index_) const {
        hxassertmsg(index_ < this->size(), "invalid_index");
        return this->data()[index_];
    }

    /// Returns a reference to the element at the specified index.
    /// - `index` : The index of the element.
    hxconstexpr_fn T_& operator[](size_t index_) {
        hxassertmsg(index_ < this->size(), "invalid_index");
        return this->data()[index_];
    }

    /// Check if array is empty by casting it to bool. (Non-standard, from Python.)
    hxconstexpr_fn operator bool(void) const {
        return !this->empty();
    }

    /// Constructs an array of T from an array of U. (Non-standard.)
    /// - `a` : The array.
    /// - `Sz` : Its size.
    template<typename U_, size_t size_>
    hxconstexpr_fn void assign(const U_(&a_)[size_]) {
        this->assign(a_ + 0, a_ + size_);
    }

    /// Assigns elements from a range defined by iterators to the array.
    /// - `begin` : The beginning iterator.
    /// - `end` : The end iterator.
    template <typename iter_t_>
    hxconstexpr_fn void assign(iter_t_ begin_, iter_t_ end_) {
        this->reserve((size_t)(end_ - begin_));
        T_* it_ = this->data();
        this->destruct_(it_, m_end_);
        while (begin_ != end_) {
            ::new (it_++) T_(*begin_++);
        }
        m_end_ = it_;
    }

    /// Returns a const reference to the end element in the array.
    hxconstexpr_fn const T_& back(void) const {
        hxassertmsg(!this->empty(), "invalid_reference");
        return m_end_[-1];
    }

    /// Returns a reference to the end element in the array.
    hxconstexpr_fn T_& back(void) {
        hxassertmsg(!this->empty(), "invalid_reference");
        return m_end_[-1];
    }

    /// Returns a const_iterator to the beginning of the array.
    hxconstexpr_fn const T_* begin(void) const { return this->data(); }

    /// Returns an iterator to the beginning of the array.
    hxconstexpr_fn T_* begin(void) { return this->data(); }

    /// Returns a const_iterator to the beginning of the array (alias for begin()).
    hxconstexpr_fn const T_* cbegin(void) const { return this->data(); }

    /// Returns a const_iterator to the beginning of the array (alias for begin()).
    hxconstexpr_fn const T_* cbegin(void) { return this->data(); }

    /// Returns a const_iterator to the end of the array.
    hxconstexpr_fn const T_* cend(void) const { return m_end_; }

    /// Returns a const_iterator to the end of the array (alias for end()).
    hxconstexpr_fn const T_* cend(void) { return m_end_; }

    /// Clears the array, destroying all elements.
    hxconstexpr_fn void clear(void) {
        this->destruct_(this->data(), m_end_);
        m_end_ = this->data();
    }

    /// Variant of emplace_back() that returns a pointer for use with placement new.
    /// (Non-standard.)
    hxconstexpr_fn void* emplace_back_unconstructed(void) {
        hxassertmsg(!this->full(), "stack_overflow");
        return (void*)m_end_++;
    }

    /// Returns true if the arrays are equal.
    /// - `x` : The other array.
    hxconstexpr_fn bool equal(const hxarray& x) const {
        if(this->size() != x.size()) {
            return false;
        }
        for(const T_ *it0_ = this->data(), *it1_ = x.data(), *end_ = m_end_; it0_ != end_; ++it0_, ++it1_) {
            if(!hxkey_equal(*it0_, *it1_)) {
                return false;
            }
        }
        return true;
    }

    /// Returns true if the array is empty.
    hxconstexpr_fn bool empty(void) const {
        return m_end_ == this->data();
    }

    /// Returns a const_iterator to the end of the array.
    hxconstexpr_fn const T_* end(void) const { return m_end_; }

    /// Returns an iterator to the end of the array.
    hxconstexpr_fn T_* end(void) { return m_end_; }

    /// Erases the element indicated.
    /// - `pos` : Pointer to the element to erase.
    hxconstexpr_fn void erase(T_* pos_) {
        hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
        while((pos_ + 1) != m_end_) {
            // TODO move semantics.
            *pos_ = *(pos_ + 1);
            ++pos_;
        }
        --m_end_->~T_();
    }

    /// Erases the element indicated.
    /// - `index` : Index of the element to erase.
    hxconstexpr_fn void erase(size_t index_) {
        hxassertmsg(index_ < this->size(), "invalid_index");
        this->erase(this->data() + index_);
    }

    /// Variant of erase() that moves the end element down to replace the erased element.
    /// (Non-standard.)
    /// - `pos` : Pointer to the element to erase.
    hxconstexpr_fn void erase_unordered(T_* pos_) {
        hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
        if (pos_ != --m_end_) {
            *pos_ = *m_end_;
        }
        m_end_->~T_();
    }

    /// Variant of erase() that moves the end element down to replace erased element.
    /// (Non-standard.)
    /// - `index` : The index of the element to erase.
    hxconstexpr_fn void erase_unordered(size_t index_) {
        hxassertmsg(index_ < this->size(), "invalid_index");
        this->erase_unordered(this->data() + index_);
    }

#if HX_CPLUSPLUS >= 201103L
    /// Calls a function, lambda or std::function on each element. (Non-standard.)
    /// `fn` - A function like object.
    template<typename functor_t_>
    void for_each(functor_t_& fn_) {
        for(T_ *it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
            fn_(*it_);
        }
    }

    /// Calls a function, lambda or std::function on each element. (Non-standard.)
    /// Lambdas and std::function can be provided as temporaries and that has to be
    /// allowed. It is standard to cast function objects to && but that is not
    /// being done here as they are not actually intended to be consumed on first
    /// use and are reused.
    /// `fn` - A function like object.
    template<typename functor_t_>
    void for_each(functor_t_&& fn_) {
        for(T_ *it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
            fn_(*it_);
        }
    }
#endif

    /// Returns a const reference to the begin element in the array.
    hxconstexpr_fn const T_& front(void) const {
        hxassertmsg(!this->empty(), "invalid_reference");
        return *this->data();
    }

    /// Returns a reference to the begin element in the array.
    hxconstexpr_fn T_& front(void) {
        hxassertmsg(!this->empty(), "invalid_reference");
        return *this->data();
    }

    /// Returns true if the array is full.
    hxconstexpr_fn bool full(void) const { return m_end_ == this->data() + this->capacity(); }

    /// Returns true when the array is full (size equal capacity). (Non-standard.)
    hxconstexpr_fn bool full(void) {
        return this->size() == this->capacity();
    }

    /// Hash together all the elements and return the result. Uses hxkeyhash which
    /// can be overloaded. Uses FNV-1a hash (modified).
    hxconstexpr_fn hxhash_t hash() const {
        hxhash_t x_ = (hxhash_t)0x9e3779b9;
        for(T_ *it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
            x_ ^= hxkey_hash(*it_);
        	x_ *= (hxhash_t)0x01000193;
        }
        return x_;
    }

    /// Inserts the element at the offset indicated. insert(begin(), x) and
    /// insert(end(), x) will work as long as the array is allocated.
    /// - `pos` : Pointer to the location to insert the new element at.
    /// - `x` : The new element.
    hxconstexpr_fn void insert(T_* pos_, const T_& t_) {
        hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
        T_* it_ = m_end_++;
        if(it_ == this->data()) {
            // Single constructor call for first element.
            ::new (it_) T_(t_);
        }
        else {
            // A constructor for a new element followed by a series of assignments.
            // TODO move semantics.
            ::new(it_) T_(it_[-1]);
            --it_;
            while(pos_ < it_) {
                *it_ = it_[-1];
                --it_;
            }
            *it_ = t_;
        }
    }

    /// Inserts the element at the offset indicated. insert(begin(), x) and
    /// insert(end(), x) will work as long as the array is allocated.
    /// - `index` : Index of the location to insert the new element at.
    /// - `x` : The new element.
    hxconstexpr_fn void insert(size_t index_, const T_& t_) {
        hxassertmsg(index_ < this->size(), "invalid_index");
        this->insert(this->data() + index_, t_);
    }

    /// Returns true if the arrays are equal.
    /// - `x` : The other array.
    hxconstexpr_fn bool less(const hxarray& x) const {
        for(const T_ *it0_ = this->data(), *it1_ = x.data();
                it0_ != m_end_ && it1_ != x.m_end_; ++it0_, ++it1_) {
            if(!hxkey_equal(*it0_, *it1_)) {
                return hxkey_less(*it0_, *it1_);
            }
        }
        return this->size() < x.size();
    }

    /// Removes the end element from the array.
    hxconstexpr_fn void pop_back(void) {
        hxassertmsg(!this->empty(), "stack_underflow");
        (--m_end_)->~T_();
    }

    /// Adds a copy of the specified element to the end of the array.
    /// - `t` : The element to add.
    hxconstexpr_fn void push_back(const T_& t_) {
        hxassertmsg(!this->full(), "stack_overflow");
        ::new (m_end_++) T_(t_);
    }

    /// Reserves storage for at least the specified number of elements.
    /// - `size` : The number of elements to reserve storage for.
    /// - `allocator` : The memory manager ID to use for allocation (default: hxsystem_allocator_current)
    /// - `alignment` : The alignment to for the allocation. (default: HX_ALIGNMENT)
    hxconstexpr_fn void reserve(size_t size_,
            hxsystem_allocator_t allocator_=hxsystem_allocator_current,
            uintptr_t alignment_=HX_ALIGNMENT) {
        T_* prev = this->data();
        this->reserve_storage(size_, allocator_, alignment_);
        hxassertmsg(!prev || prev == this->data(), "reallocation_disallowed"); (void)prev;
        if (m_end_ == hxnull) {
            m_end_ = this->data();
        }
    }

    /// Resizes the array to the specified size, constructing or destroying
    /// elements as needed. Requires a default constructor. Integers and floats
    /// will be value-initialized to zero as per the standard.
    /// - `size` : The new size of the array.
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

    /// An overload with an initial value for new elements. Resizes the array to
    /// the specified size, copy constructing or destroying elements as needed.
    /// - `size` : The new size of the array.
    /// - `t` : Initial value for new elements.
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

    /// Reverse the elements. Uses hxswap which may be overriden. (Non-standard. Python.)
    hxconstexpr_fn void reverse(void) {
        T_* b_ = this->data();
        T_* e_ = m_end_ - 1;
        while (b_ < e_) {
            hxswap(*b_++, *e_--);
        }
    }

    /// Returns the number of elements in the array.
    hxconstexpr_fn size_t size(void) const {
        return (size_t)(m_end_ - this->data());
    }

    /// Swap. Only works with capacity_ == hxallocator_dynamic_capacity
    /// - `x` : The array to swap with.
    hxconstexpr_fn void swap(hxarray& x) {
        /// *** Only hxallocator_dynamic_capacity works here. ***
        hxallocator<T_, capacity_>::swap(x);
        hxswap(x.m_end_, m_end_);
    }

private:
    // Destroys elements in the range [begin, end].
    // - begin: Pointer to the beginning of the range.
    // - end: Pointer to the end of the range.
    hxconstexpr_fn void destruct_(T_* begin_, T_* end_) {
        while (begin_ != end_) {
            begin_++->~T_();
        }
    }
    T_* m_end_;
};

/// `bool hxequal(hxarray<T>& x, hxarray<T>& y)` - Compares the contents of x and y.
template<typename T_>
hxconstexpr_fn void hxkey_equal(hxarray<T_>& x_, hxarray<T_>& y_) {
	x_.equal(y_);
}

/// `bool hxhash(hxarray<T>& x)` - Hashes the contents of x.
template<typename T_>
hxconstexpr_fn void hxhash(hxarray<T_>& x_) {
	x_.hash();
}

// TODO: This is breaking hxkey_less with ambiguous errors.
// `bool hxkey_less(hxarray<T>& x, hxarray<T>& y)` - Compares the contents of x and y.
//template<typename T_>
//hxconstexpr_fn void hxkey_less(hxarray<T_>& x_, hxarray<T_>& y_) {
//	x_.less(y_);
//}

/// `void hxswap(hxarray<T>& x, hxarray<T>& y)` - Exchanges the contents of x and y.
template<typename T_>
hxconstexpr_fn void hxswap(hxarray<T_>& x_, hxarray<T_>& y_) {
	x_.swap(y_);
}
