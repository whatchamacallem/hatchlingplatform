#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"
#include "hxkey.hpp"

#if !HX_FREESTANDING // No hxinitializer_list freestanding.
#include <initializer_list>
#endif

/// `hxarray` - Another vector class. Uses raw pointers as an iterator type so
/// that you get compile errors and a debug experience that is in plain C++
/// instead of the std. There are asserts. Please run a memory sanitizer and an
/// undefined behavior sanitizer too. Use a C array if you need `constexpr`. The
/// excessive number of operators is due to the rules about default operators.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxarray : public hxallocator<T_, capacity_> {
public:
	/// Publishes the value type.
	typedef T_ value_t;
	/// Random access iterator.
	typedef T_* iterator;
	/// Const random access iterator.
	typedef const T_* const_iterator;

	/// Constructs an empty array with a capacity of Capacity. m_end_ will be 0
	/// if Capacity is 0.
	explicit hxarray(void)
		: hxallocator<T_, capacity_>(), m_end_(this->data()) { }

	/// Constructs an array of a given size using T_'s default constructor.
	/// - `size` : Sets array size as if resize(size) were called.
	explicit hxarray(size_t size_)
			: hxallocator<T_, capacity_>(), m_end_(this->data()) {
		this->resize(size_);
	}

	/// Constructs an array of a given size by making copies of t.
	/// - `size` : Sets array size as if resize(size, t) were called.
	/// - `t` : The const T& to be duplicated.
	explicit hxarray(size_t size_, const T_& t_)
			: hxallocator<T_, capacity_>(), m_end_(this->data()) {
		this->resize(size_, t_);
	}

	/// Copy constructs an array. Non-explicit to allow assignment constructor.
	/// - `x` : A non-temporary Array<T>.
	hxarray(const hxarray& x_) : hxallocator<T_, capacity_>() {
		m_end_ = this->data();
		this->assign<const T_*>(x_.data(), x_.m_end_);
	}

	/// Copy constructs an array. Non-explicit to allow assignment constructor.
	/// - `x` : A non-temporary Array<T>.
	template <size_t capacity2_>
	hxarray(const hxarray<T_, capacity2_>& x_) : hxallocator<T_, capacity_>() {
		m_end_ = this->data();
		this->assign<const T_*>(x_.data(), x_.end());
	}

	/// Copy construct from a temporary using `swap`. Refuses to copy construct
	/// from a statically allocated temporary for efficiency. Only works with
	/// `hxallocator_dynamic_capacity`. Dynamically allocated arrays are swapped
	/// with very little overhead.
	/// - `x` : A temporary Array<T>.
	hxarray(hxarray&& x_) : hxarray() {
		this->swap(x_); // NOTA BENE: Requires capacity 0 to compile.
	}

#if !HX_FREESTANDING
	/// Pass values of std::initializer_list as initializers to an array of T.
	/// WARNING: This constructor will override the other constructors when
	/// uniform initialization is used.  E.g. hxarry<int>x{1,2} is an array
	/// containing {1,2} and hxarry<int>x(1,2) is the array containing {2}.
	/// - `x` : A std::initializer_list<x_t>.
	template <typename x_t_>
	hxarray(std::initializer_list<x_t_> list_) : hxarray() {
		this->assign(list_.begin(), list_.end());
	}
#endif

	/// Destructs the array and destroys all elements.
	~hxarray(void) {
		this->destruct_(this->data(), m_end_);
	}

	/// Appends an element.  (Non-standard.) Vector math is not a goal so this
	/// should not end up overloaded.
	/// - `x` : An object to append. Not a temporary.
	void operator+=(const T_& x_) {
		::new(this->emplace_back_unconstructed()) T_(x_);
	}

	/// Appends an element.  (Non-standard.) Vector math is not a goal so this
	/// should not end up overloaded.
	/// - `x` : An object to append. Passed as a temporary.
	void operator+=(T_&& x_) {
		::new(this->emplace_back_unconstructed()) T_(hxmove(x_));
	}

	/// Appends the contents of another array.  (Non-standard, from Python.)
	/// Vector math is not a goal so this should not end up overloaded.
	/// - `x` : Another array. Not a temporary.
	void operator+=(const hxarray& x_) {
		for(const T_ *it_ = x_.data(), *end_ = x_.m_end_; it_ != end_; ++it_) {
			::new(this->emplace_back_unconstructed()) T_(*it_);
		}
	}

	/// Appends the contents of another array.  (Non-standard, from Python.)
	/// Vector math is not a goal so this should not end up overloaded.
	/// - `x` : Another array passed as a temporary.
	void operator+=(hxarray&& x_) {
		for(const T_ *it_ = x_.data(), *end_ = x_.m_end_; it_ != end_; ++it_) {
			::new(this->emplace_back_unconstructed()) T_(hxmove(*it_));
		}
	}

	/// Assigns the contents of another hxarray to this array. Standard except
	/// reallocation is disallowed.
	/// - `x` : A non-temporary Array<T>.
	void operator=(const hxarray& x_) {
		this->assign<const T_*>(x_.data(), x_.m_end_);
	}

	/// Swap contents with a temporary array using `swap`. Only works with
	/// `hxallocator_dynamic_capacity`. Dynamically allocated arrays are swapped
	/// with very little overhead.
	/// - `x` : A temporary Array<T>.
	void operator=(hxarray&& x_) {
		this->swap(x_);
	}

	/// Copies the elements of a container with random access iterators.
	/// - `x` : Any container implementing begin and end.
	template <typename x_t_>
	void operator=(const x_t_& x_) {
		this->assign(x_.begin(), x_.end());
	}

	/// Returns a const reference to the element at the specified index.
	/// - `index` : The index of the element.
	const T_& operator[](size_t index_) const {
		hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
		return this->data()[index_];
	}

	/// Returns a reference to the element at the specified index.
	/// - `index` : The index of the element.
	T_& operator[](size_t index_) {
		hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
		return this->data()[index_];
	}

	/// Constructs an array of T from a C-style array of U. (Non-standard.)
	/// - `a` : The array.
	/// - `Sz` : Its size.
	template<typename U_, size_t size_>
	void assign(const U_(&a_)[size_]) {
		this->assign(a_ + 0, a_ + size_);
	}

	/// Assigns elements from a range defined by iterators.
	/// - `begin` : The beginning iterator.
	/// - `end` : The end iterator.
	template <typename iter_t_>
	void assign(iter_t_ begin_, iter_t_ end_) {
		this->reserve((size_t)(end_ - begin_));
		T_* it_ = this->data();
		this->destruct_(it_, m_end_);
		while(begin_ != end_) {
			::new (it_++) T_(*begin_++);
		}
		m_end_ = it_;
	}

	/// Returns a const reference to the end element in the array.
	const T_& back(void) const {
		hxassertmsg(!this->empty(), "invalid_reference");
		return m_end_[-1];
	}

	/// Returns a reference to the end element in the array.
	T_& back(void) {
		hxassertmsg(!this->empty(), "invalid_reference");
		return m_end_[-1];
	}

	/// Returns a const_iterator to the beginning of the array.
	const T_* begin(void) const { return this->data(); }

	/// Returns an iterator to the beginning of the array.
	T_* begin(void) { return this->data(); }

	/// Returns a const_iterator to the beginning of the array (alias for
	/// begin()).
	const T_* cbegin(void) const { return this->data(); }

	/// Returns a const_iterator to the end of the array.
	const T_* cend(void) const { return m_end_; }

	/// Clears the array, destroying all elements.
	void clear(void) {
		this->destruct_(this->data(), m_end_);
		m_end_ = this->data();
	}

	/// Variant of emplace_back() that returns a pointer for use with placement
	/// new. (Non-standard.)
	void* emplace_back_unconstructed(void) {
		hxassertmsg(!this->full(), "stack_overflow");
		return (void*)m_end_++;
	}

	/// Returns true if the arrays compare as equivalent. This version takes a
	/// functor for key comparison.
	/// - `x` : The other array.
	/// - `equal` : A key comparison functor definining an equivalence relationship.
	template<typename equal_t_, size_t capacity_x_>
	bool equal(const hxarray<T_, capacity_x_>& x_, const equal_t_& equal_) const {
		if(this->size() != x_.size()) {
			return false;
		}
		for(const T_ *it0_ = this->data(), *it1_ = x_.data(), *end_ = m_end_;
				it0_ != end_; ++it0_, ++it1_) {
			if(!equal_(*it0_, *it1_)) {
				return false;
			}
		}
		return true;
	}

	/// Returns true if the arrays compare equivalent using hxkey_equal.
	/// - `x` : The other array.
	template<size_t capacity_x_>
	bool equal(const hxarray<T_, capacity_x_>& x_) const {
		return this->equal(x_, hxkey_equal_function<T_, T_>());
	}

	/// Returns true if the array is empty.
	bool empty(void) const {
		return m_end_ == this->data();
	}

	/// Returns a const_iterator to the end of the array.
	const T_* end(void) const { return m_end_; }

	/// Returns an iterator to the end of the array.
	T_* end(void) { return m_end_; }

	/// Erases the element indicated.
	/// - `pos` : Pointer to the element to erase.
	void erase(T_* pos_) {
		hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
		while((pos_ + 1) != m_end_) {
			*pos_ = hxmove(*(pos_ + 1));
			++pos_;
		}
		(--m_end_)->~T_();
	}

	/// Erases the element indicated.
	/// - `index` : Index of the element to erase.
	void erase(size_t index_) {
		hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
		this->erase(this->data() + index_);
	}

	/// Variant of erase() that moves the end element down to replace the erased
	/// element. (Non-standard.)
	/// - `pos` : Pointer to the element to erase.
	void erase_unordered(T_* pos_) {
		hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
		if(pos_ != --m_end_) {
			*pos_ = hxmove(*m_end_);
		}
		m_end_->~T_();
	}

	/// Variant of erase() that moves the end element down to replace erased
	/// element. (Non-standard.)
	/// - `index` : The index of the element to erase.
	void erase_unordered(size_t index_) {
		hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
		this->erase_unordered(this->data() + index_);
	}

	/// Calls a function, lambda or std::function on each element.
	/// (Non-standard.)
	/// `fn` - A function like object.
	template<typename functor_t_>
	void for_each(functor_t_& fn_) {
		for(T_ *it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
			fn_(*it_);
		}
	}

	/// Calls a function, lambda or `std::function` on each element.
	/// (Non-standard.) Lambdas and `std::function` can be provided as
	/// temporaries and that has to be allowed. The `&&` variant of
	/// `functor_t::operator()` is being selected using `hxmove`. This is a
	/// traditional way to signal to the functor that it is a temporary.
	/// `fn` - A function like object.
	template<typename functor_t_>
	void for_each(functor_t_&& fn_) {
		for(T_ *it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
			hxmove(fn_)(*it_);
		}
	}

	/// Returns a const reference to the begin element in the array.
	const T_& front(void) const {
		hxassertmsg(!this->empty(), "invalid_reference");
		return *this->data();
	}

	/// Returns a reference to the begin element in the array.
	T_& front(void) {
		hxassertmsg(!this->empty(), "invalid_reference");
		return *this->data();
	}

	/// Returns true if the array is full.
	bool full(void) const {
		return m_end_ == this->data() + this->capacity();
	}

	/// Returns true when the array is full (size equal capacity).
	/// (Non-standard.)
	bool full(void) {
		return this->size() == this->capacity();
	}

	/// Inserts the element at the offset indicated. `insert(begin(), x)` and
	/// `insert(end(), x)` will work as long as the array is allocated. Not
	/// intended for objects that are expensive to move. Consider using
	/// emplace_back_unconstructed for storing large objects.
	/// - `pos` : Pointer to the location to insert the new element at.
	/// - `t` : The new element.
	void insert(T_* pos_, const T_& t_) {
		hxassertmsg(pos_ >= this->data() && pos_ <= m_end_ && !this->full(), "invalid_insert");
		if(pos_ == m_end_) {
			// Single constructor call for last element.
			::new (m_end_++) T_(t_);
		}
		else {
			// A copy constructor for a new end element followed by a series of
			// assignment operations.
			T_* it_ = m_end_++;
			::new(it_) T_(it_[-1]);
			while(pos_ < --it_) {
				*it_ = it_[-1];
			}
			*it_ = t_;
		}
	}

	/// Inserts the element at the offset indicated. `insert(begin(), x)` and
	/// `insert(end(), x)` will work as long as the array is allocated.
	/// - `index` : Index of the location to insert the new element at.
	/// - `x` : The new element.
	void insert(size_t index_, const T_& t_) {
		hxassertmsg(index_ <= this->size(), "invalid_index %zu", index_);
		this->insert(this->data() + index_, t_);
	}

	/// Returns true if this array compares as less than x. Sorts [1] before
	/// [1,2]. This version takes two functors for key comparison.
	/// - `x` : The other array.
	/// - `less` : A key comparison functor definining a less-than ordering relationship.
	/// - `equal` : A key comparison functor definining an equivalence relationship.
	template<typename less_t_, typename equal_t_, size_t capacity_x_>
	bool less(const hxarray<T_, capacity_x_>& x_, const less_t_& less_, const equal_t_& equal_) const {
		size_t sz_ = hxmin(this->size(), x_.size());
		for(const T_ *it0_ = this->data(), *it1_ = x_.data(), *end_ = it0_ + sz_;
				it0_ != end_; ++it0_, ++it1_) {
			// Use `a == b` instead of `a < b && b < a` for performance.
			if(!equal_(*it0_, *it1_)) {
				return less_(*it0_, *it1_);
			}
		}
		// Order the prefix before the other.
		return this->size() < x_.size();
	}

	/// Returns true if this array compares less than `x` using `hxkey_equal`
	/// and `hxkey_less`.
	/// Sorts `[1]` before `[1,2]`.
	/// - `x` : The other array.
	template<size_t capacity_x_>
	bool less(const hxarray<T_, capacity_x_>& x_) const {
		return this->less(x_, hxkey_less_function<T_, T_>(), hxkey_equal_function<T_, T_>());
	}

	/// Removes the end element from the array.
	void pop_back(void) {
		hxassertmsg(!this->empty(), "stack_underflow");
		(--m_end_)->~T_();
	}

	/// Adds a copy of the element to the end of the array.
	/// - `t` : The element to add.
	void push_back(const T_& t_) {
		hxassertmsg(!this->full(), "stack_overflow");
		::new (m_end_++) T_(t_);
	}

	/// Move the element to the end of the array.
	/// - `t` : The element to move.
	void push_back(T_&& t_) {
		hxassertmsg(!this->full(), "stack_overflow");
		::new (m_end_++) T_(hxmove(t_));
	}

	/// Reserves storage for at least the specified number of elements.
	/// - `size` : The number of elements to reserve storage for.
	/// - `allocator` : The memory manager ID to use for allocation (default: hxsystem_allocator_current)
	/// - `alignment` : The alignment to for the allocation. (default: HX_ALIGNMENT)
	void reserve(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		T_* prev = this->data();
		this->reserve_storage(size_, allocator_, alignment_);
		hxassertmsg(!prev || prev == this->data(), "reallocation_disallowed"); (void)prev;
		if(m_end_ == hxnull) {
			m_end_ = this->data();
		}
	}

	/// Resizes the array to the specified size, constructing or destroying
	/// elements as needed. Requires a default constructor. Integers and floats
	/// will be value-initialized to zero as per the standard.
	/// - `size` : The new size of the array.
	void resize(size_t size_) {
		this->reserve(size_);
		T_* end_ = this->data() + size_;
		if(size_ >= this->size()) {
			while(m_end_ != end_) {
				// This version uses a default constructor.
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
	void resize(size_t size_, const T_& t_) {
		this->reserve(size_);
		T_* end_ = this->data() + size_;
		if(size_ >= this->size()) {
			while(m_end_ != end_) {
				// This version uses a copy constructor.
				::new (m_end_++) T_(t_);
			}
		}
		else {
			this->destruct_(end_, m_end_);
		}
		m_end_ = end_;
	}

	/// Returns the number of elements in the array.
	size_t size(void) const {
		return (size_t)(m_end_ - this->data());
	}

	/// Returns the number of bytes in the array. (Non-standard.)
	size_t size_bytes(void) const {
		return sizeof(T_) * (size_t)(m_end_ - this->data());
	}

	/// Swap contents with a temporary array. Only works with
	/// `hxallocator_dynamic_capacity`. Dynamically allocated arrays are swapped
	/// with very little overhead.
	/// - `x` : The array to swap with.
	void swap(hxarray& x_) {
		// NOTA BENE: Only hxallocator_dynamic_capacity works here.
		hxallocator<T_, capacity_>::swap(x_);
		hxswap(x_.m_end_, m_end_);
	}

private:
	// Destroys elements in the range [begin, end).
	void destruct_(T_* begin_, T_* end_) {
		while(begin_ != end_) {
			begin_++->~T_();
		}
	}
	T_* m_end_;
};

// The array overloads of hxkey_equal, hxkey_less and hxswap are C++20 only.
// Without the "requires" keyword these end up being ambiguous. Use T::equal,
// T::less and T::swap. Use C++20 if you want to use arrays as generic keys.
#if HX_CPLUSPLUS >= 202002L

/// `bool hxequal(hxarray<T>& x, hxarray<T>& y)` - Compares the contents of x
/// and y for equivalence.
template<typename T_, size_t capacity_x_, size_t capacity_y_>
bool hxkey_equal(const hxarray<T_, capacity_x_>& x_, const hxarray<T_, capacity_y_>& y_) {
    return x_.equal(y_);
}

/// `bool hxkey_less(hxarray<T>& x, hxarray<T>& y)` - Compares the contents of
// x and y using hxkey_equal and hxkey_less on each element.
template<typename T_, size_t capacity_x_, size_t capacity_y_>
bool hxkey_less(const hxarray<T_, capacity_x_>& x_, const hxarray<T_, capacity_y_>& y_) {
	return x_.less(y_);
}

/// `void hxswap(hxarray<T>& x, hxarray<T>& y)` - Exchanges the contents of x
/// and y. Only works with `hxallocator_dynamic_capacity`. Dynamically allocated
/// arrays are swapped with very little overhead.
template<typename T_>
void hxswap(const hxarray<T_>& x_, const hxarray<T_>& y_) {
	x_.swap(y_);
}

#endif // HX_CPLUSPLUS >= 202002L
