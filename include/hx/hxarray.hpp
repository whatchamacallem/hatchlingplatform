#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hxallocator.hpp"
#include "hxkey.hpp"
#include "detail/hxsort_detail.hpp"

#if !HX_NO_LIBCXX
#include <initializer_list>
#endif

/// `hxarray` - Implements both `std::vector` and `std::inplace_vector` with a
/// chunk added a few things unimplemented. Uses raw pointers as an iterator
/// type so that you get compile errors and a debug symbols that use plain C++
/// pointers instead. There are exhaustive asserts.
///
/// `hxarray` can be constructed from C string literals as follows:
///   `hxarray<char, HX_MAX_LINE> string_buffer("example C string");`
/// however `operator+=` does not support C strings.
///
/// Please run a memory sanitizer and an undefined behavior sanitizer too. Use a
/// C array for now if you need `constexpr`. The excessive number of operators
/// is due to the rules about default operators.
/// - `T` : Element type stored by the array.
/// - `capacity` : Maximum element count or `hxallocator_dynamic_capacity` for dynamic storage.
template<typename T_, size_t capacity_=hxallocator_dynamic_capacity>
class hxarray : public hxallocator<T_, capacity_> {
public:
	/// Random access iterator.
	typedef T_* iterator;

	/// Const random access iterator.
	typedef const T_* const_iterator;

	/// Publishes the value type.
	typedef T_ value_type;

	/// Constructs an empty array with a capacity of Capacity. `m_end_` will be 0
	/// if `capacity` is 0.
	explicit hxarray(void);

	/// Constructs an array of a given size using `T`'s default constructor.
	/// - `size` : Sets array size as if `resize(size)` were called.
	explicit hxarray(size_t size_);

	/// Constructs an array of a given size by making copies of `x`.
	/// - `size` : Sets array size as if `resize(size, t)` were called.
	/// - `x` : The `const T&` to be duplicated.
	explicit hxarray(size_t size_, const T_& x_);

	/// Copy constructs an array. Non-explicit to allow assignment constructor.
	/// - `x` : An `Array<T>`.
	hxarray(const hxarray& x_);

	/// Copy constructs an array. Non-explicit to allow assignment constructor.
	/// - `x` : A non-temporary `Array<T>`.
	template <size_t capacity_x_>
	hxarray(const hxarray<T_, capacity_x_>& x_);

	/// Copy construct from a temporary. Refuses to copy construct from a
	/// statically allocated temporary for efficiency. Only works with
	/// `hxallocator_dynamic_capacity`.
	/// - `x` : A temporary `Array<T>`.
	hxarray(hxarray&& x_);

	/// Construct from a C-style array. Usable as an `initializer_list` when
	/// the `std` namespace is not available. E.g.,
	/// ```cpp
	/// static const int initial_values[] = { 5, 4, 3 };
	/// hxarray<int> current_values(initial_values);
	/// ```
	/// - `array` : A const array of `array_length` `value_t`.
	template<typename other_value_t_, size_t array_length_>
	hxarray(const other_value_t_(&array_)[array_length_]);

#if !HX_NO_LIBCXX
	/// Pass values of `std::initializer_list` as initializers to an array of `T`.
	/// WARNING: This constructor will override the other constructors when
	/// uniform initialization is used. E.g., `hxarry<int>x{1,2}` is an array
	/// containing `{1,2}` and `hxarry<int>x(1,2)` is the array containing `{2}`.
	/// - `x` : A `std::initializer_list<other_value_t>`.
	template <typename other_value_t_>
	hxarray(std::initializer_list<other_value_t_> x_);
#endif

	/// Destructs the array and destroys all elements.
	~hxarray(void);

	/// Assigns the contents of another `hxarray` to this array. Standard except
	/// reallocation is disallowed.
	/// - `x` : A non-temporary Array<T>.
	void operator=(const hxarray& x_);

	/// Assigns the contents of another `hxarray` to this array. Standard except
	/// reallocation is disallowed.
	/// - `x` : A non-temporary Array<T>.
	template <size_t capacity_x_>
	void operator=(const hxarray<T_, capacity_x_>& x_);

	/// Swap contents with a temporary array using `swap`. Only works with
	/// `hxallocator_dynamic_capacity`. Dynamically allocated arrays are swapped
	/// with very little overhead.
	/// - `x` : A temporary Array<T>.
	void operator=(hxarray&& x_);

	/// Assign from a C-style array. Usable as an `initializer_list` when
	/// the `std` namespace is not available. E.g.,
	/// ```cpp
	/// static const int initial_values[] = { 5, 4, 3 };
	/// hxarray<int, 32> current_values(initial_values);
	/// ```
	/// - `array` : A const array of `array_length` `value_t`.
	template<typename other_value_t_, size_t array_length_>
	void operator=(const other_value_t_(&array_)[array_length_]);

	/// Returns a const reference to the element at the specified index.
	/// - `index` : The 0-based offset of the element.
	const T_& operator[](size_t index_) const;

	/// Returns a reference to the element at the specified index.
	/// - `index` : The 0-based offset of the element.
	T_& operator[](size_t index_);

	/// Appends an element. (Non-standard.) Vector math is not a goal so this
	/// should not end up overloaded. Perfect argument forwarding would be too
	/// ambiguous.
	/// - `x` : An object to append. Not a temporary.
	void operator+=(const T_& x_);

	/// Appends an element. (Non-standard.) Vector math is not a goal so this
	/// should not end up overloaded. Perfect argument forwarding would be too
	/// ambiguous.
	/// - `x` : An object to append. Passed as a temporary.
	void operator+=(T_&& x_);

	/// Appends the contents of another array. (Non-standard, from Python.)
	/// Vector math is not a goal so this should not end up overloaded.
	/// - `x` : Another array. Not a temporary.
	template <size_t capacity_x_>
	void operator+=(const hxarray<T_, capacity_x_>& x_);

	/// Appends the contents of another array. (Non-standard, from Python.)
	/// Vector math is not a goal so this should not end up overloaded.
	/// - `x` : Another array passed as a temporary.
	template <size_t capacity_x_>
	void operator+=(hxarray<T_, capacity_x_>&& x_);

	/// Returns true if the predicate returns true for every element and false
	/// otherwise. Will stop iterating when the predicate returns false.
	/// - `fn` : A functor returning boolean.
	template<typename functor_t_>
	bool all_of(functor_t_&& fn_) const;

	/// Returns true if the predicate returns true for any element and false
	/// otherwise. Will stop iterating when the predicate returns true.
	/// - `fn` : A functor returning boolean.
	template<typename functor_t_>
	bool any_of(functor_t_&& fn_) const;

	/// Assigns elements from a range defined by random access iterators.
	/// `iter_t::operator-` is required.
	/// - `begin` : The beginning iterator.
	/// - `end` : The end iterator.
	template <typename iter_t_>
	void assign(iter_t_ begin_, iter_t_ end_);

#if HX_CPLUSPLUS >= 202002L
	/// Assigns elements from a range referenced by an lvalue. `range_t::begin`
	/// and `range_t::end` are required as `std::begin` and `std::end` may not
	/// exist. Use `operator=` to assign from a C-style array.
	/// - `range` : The range to copy elements from.
	template <typename range_t_>
	void assign_range(range_t_& range_);

	/// Assigns elements from a temporary range. This overload enables moving the
	/// range elements into the array when forwarding rvalues.
	/// - `range` : The range to move elements from.
	template <typename range_t_, typename=hxenable_if_t<!hxis_lvalue_reference<range_t_>::value>>
	void assign_range(range_t_&& range_);
#endif

	/// Returns a const reference to the end element in the array.
	const T_& back(void) const;

	/// Returns a reference to the end element in the array.
	T_& back(void);

	/// Returns a `const T*` to the beginning of the array.
	const T_* begin(void) const;

	/// Returns an `T*` to the beginning of the array.
	T_* begin(void);

	/// Returns a `const T*` to the beginning of the array (alias for
	/// begin()).
	const T_* cbegin(void) const;

	/// Returns a `const T*` to the end of the array.
	const T_* cend(void) const;

	/// Clears the array, destroying all elements.
	void clear(void);

	/// Emplaces an element at the end of the array using forwarded arguments.
	/// Returns a reference to the new element.
	/// - `args` : Arguments forwarded to `T`'s constructor.
	template<typename... args_t_>
	T_& emplace_back(args_t_&&... args_);

	/// Returns true if the arrays compare as equivalent. This version takes a
	/// functor for key comparison.
	/// - `x` : The other array.
	/// - `equal` : A key comparison functor definining an equivalence relationship.
	template<size_t capacity_x_, typename equal_t_>
	bool equal(const hxarray<T_, capacity_x_>& x_, const equal_t_& equal_) const;

	/// Returns true if the arrays compare equivalent using `hxkey_equal`.
	/// - `x` : The other array.
	template<size_t capacity_x_>
	bool equal(const hxarray<T_, capacity_x_>& x_) const;

	/// Returns true if the array is empty.
	bool empty(void) const;

	/// Returns a `const T*` to the end of the array.
	const T_* end(void) const;

	/// Returns an `T*` to the end of the array.
	T_* end(void);

	/// Erases the element indicated. Should not compile with hxnull. Support
	/// for erasing ranges has not ben added yet.
	/// - `pos` : Pointer to the element to erase.
	void erase(const T_* pos_) hxattr_nonnull(2);

	/// Erases the element indicated. Use `(size_t)0` to write the integer literal 0.
	/// - `index` : Index of the element to erase.
	void erase(size_t index_);

	/// Removes elements for which the predicate returns true. (Non-standard.)
	/// Equivalent to calling `erase_unordered` inside a reverse loop. Returns
	/// the number of erased elements.
	/// - `fn` : A functor returning boolean.
	template<typename functor_t_>
	size_t erase_if(functor_t_&& fn_);

	/// Variant of `erase` that moves the end element down to replace the erased
	/// element. Should not compile with hxnull. (Non-standard.) Can be used to
	/// erase elements of an array as it is traversed. E.g.,
	/// ```cpp
	/// for(size_t i = a.size(); i--; ) {
	/// 	if(should_erase(a[i])) {
	/// 		a.erase_unordered(i);
	/// 	}
	/// }
	/// ```
	/// - `pos` : Pointer to the element to erase.
	void erase_unordered(const T_* pos_) hxattr_nonnull(2);

	/// Variant of `erase` that moves the end element down to replace erased
	/// element. Use `(size_t)0` to write the integer literal 0. (Non-standard.)
	/// - `index` : The index of the element to erase.
	void erase_unordered(size_t index_);

	/// Calls a function, lambda or `std::function` on each element.
	/// (Non-standard.) Lambdas and `std::function` can be provided as
	/// temporaries and that has to be allowed. The `&&` variant of
	/// `functor_t::operator()` may be selected using `hxmove`. This is the
	/// standard way to signal to the functor that it is a temporary. E.g.,
	/// ```cpp
	/// hxarray<int> a(3, 0);
	/// a.for_each([](int& x) { ++x; }); // Produces { 1, 1, 1 }.
	/// ```
	/// - `fn` : A functor.
	template<typename functor_t_>
	void for_each(functor_t_&& fn_);

	/// Returns a const reference to the begin element in the array.
	const T_& front(void) const;

	/// Returns a reference to the begin element in the array.
	T_& front(void);

	/// Returns true when the array is full (size equal capacity).
	/// (Non-standard.)
	bool full(void) const;

	/// Returns a `const T*` to element at the `index` or `hxnull` otherwise.
	/// - `index` : The 0-based offset of the element.
	const T_* get(size_t index_) const;

	/// Returns a `T*` to element at the `index` or `hxnull` otherwise.
	/// - `index` : The 0-based offset of the element.
	T_* get(size_t index_);

	/// Inserts the element at the offset indicated.  Should not compile with
	/// `hxnull`. `insert(begin(), x)` and `insert(end(), x)` will work as long as
	/// the array is allocated. Not intended for objects that are expensive to
	/// move. Support for inserting ranges has not ben added yet. Consider using
	/// `emplace_back` for storing large objects.
	/// - `pos` : Pointer to the location to insert the new element at.
	/// - `x` : The new element.
	template<typename ref_t_>
	void insert(const T_* pos_, ref_t_&& x_) hxattr_nonnull(2);

	/// Inserts the element at the offset indicated. Use `(size_t)0` to write
	/// the integer literal 0. `insert(begin(), x)` and `insert(end(), x)` will
	/// work as long as the array is allocated.
	/// - `index` : Index of the location to insert the new element at.
	/// - `x` : The new element.
	template<typename ref_t_>
	void insert(size_t index_, ref_t_&& x_);

	/// Returns true if this array compares as less than `x`. Sorts [1] before
	/// [1,2]. This version takes two functors for key comparison.
	/// - `x` : The other array.
	/// - `less` : A key comparison functor definining a less-than ordering relationship.
	/// - `equal` : A key comparison functor definining an equivalence relationship.
	template<size_t capacity_x_, typename less_t_, typename equal_t_>
	bool less(const hxarray<T_, capacity_x_>& x_, const less_t_& less_, const equal_t_& equal_) const;

	/// Returns true if this array compares less than `x` using `hxkey_equal`
	/// and `hxkey_less`.
	/// Sorts `[1]` before `[1,2]`.
	/// - `x` : The other array.
	template<size_t capacity_x_>
	bool less(const hxarray<T_, capacity_x_>& x_) const;

	/// Returns the capacity of the array or 0 if unallocated. This is the
	/// standard way to report that reallocation is not allowed.
	size_t max_size(void) const;

	/// Removes the end element from the array.
	void pop_back(void);

	/// Removes the first (maximum) element from a max-heap. This implements
	/// `std::pop_heap` and `std::priority_queue` using `hxless` for ordering. See
	/// `push_heap`.
	void pop_heap(void);

	/// Appends the element to the end of the array. `args_t` may be any types
	/// that can be used to construct `T`. Returns a reference to the new
	/// element.
	/// - `x` : The element to add.
	template<typename... args_t_>
	T_& push_back(args_t_&&... args_);

	/// Inserts an element into a max-heap. This implements `std::push_heap` and
	/// `std::priority_queue` using `hxless` for ordering. See `pop_heap`.
	/// Returns a reference to the element added.
	/// - `args` : Arguments forwarded to `T`'s constructor.
	template<typename ref_t_>
	T_& push_heap(ref_t_&& arg_);

	/// Reserves storage for at least the specified number of elements.
	/// - `size` : The number of elements to reserve storage for.
	/// - `allocator` : The memory manager ID to use for allocation (default: `hxsystem_allocator_current`)
	/// - `alignment` : The alignment to for the allocation. (default: `HX_ALIGNMENT`)
	void reserve(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT);

	/// Resizes the array to the specified size, constructing or destroying
	/// elements as needed. Requires a default constructor. Integers and floats
	/// will be value-initialized to zero as per the standard.
	/// - `size` : The new size of the array.
	void resize(size_t size_);

	/// An overload with an initial value for new elements. Resizes the array to
	/// the specified size, copy constructing or destroying elements as needed.
	/// - `size` : The new size of the array.
	/// - `x` : Initial value for new elements.
	void resize(size_t size_, const T_& x_);

	/// Returns the number of elements in the array.
	size_t size(void) const;

	/// Returns the number of bytes in the array. (Non-standard.)
	size_t size_bytes(void) const;

	/// Swap contents with a temporary array. Only works with
	/// `hxallocator_dynamic_capacity`. Dynamically allocated arrays are swapped
	/// with very little overhead.
	/// - `x` : The array to swap with.
	void swap(hxarray& x_);

private:
	// Returns a pointer for use with placement new.
	void* push_back_unconstructed_(void);

	// Destroys elements in the range [begin, end).
	void destruct_(T_* begin_, T_* end_);

	// 1 past the last element.
	T_* m_end_;
};

// The array overloads of hxkey_equal, hxkey_less and hxswap are C++20 only.
// Without the "requires" keyword these end up being ambiguous. Use
// hxarray::equal, hxarray::less and hxarray::swap. Use C++20 if you want to use
// arrays as generic keys.
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
void hxswap(hxarray<T_, hxallocator_dynamic_capacity>& x_,
			hxarray<T_, hxallocator_dynamic_capacity>& y_) {
	x_.swap(y_);
}

#endif // HX_CPLUSPLUS >= 202002L

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::hxarray(void) : m_end_(this->data()) { }

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::hxarray(size_t size_) : hxarray() {
	this->resize(size_);
}

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::hxarray(size_t size_, const T_& x_) : hxarray() {
	this->resize(size_, x_);
}

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::hxarray(const hxarray& x_) : hxarray() {
	this->assign<const T_*>(x_.data(), x_.m_end_);
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
hxarray<T_, capacity_>::hxarray(const hxarray<T_, capacity_x_>& x_) : hxarray() {
	this->assign<const T_*>(x_.data(), x_.end());
}

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::hxarray(hxarray&& x_) : hxarray() {
	static_assert(capacity_ == hxallocator_dynamic_capacity,
		"Capacity hxallocator_dynamic_capacity required for temporaries.");
	::memcpy((void*)this, &x_, sizeof x_);
	::memset((void*)&x_, 0x00, sizeof x_);
}

template<typename T_, size_t capacity_>
template<typename other_value_t_, size_t array_length_>
hxarray<T_, capacity_>::hxarray(const other_value_t_(&array_)[array_length_]) : hxarray() {
	this->assign(array_ + 0, array_ + array_length_);
}

#if !HX_NO_LIBCXX
template<typename T_, size_t capacity_>
template<typename other_value_t_>
hxarray<T_, capacity_>::hxarray(std::initializer_list<other_value_t_> x_) : hxarray() {
	this->assign(x_.begin(), x_.end());
}
#endif

template<typename T_, size_t capacity_>
hxarray<T_, capacity_>::~hxarray(void) {
	this->destruct_(this->data(), m_end_);
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::operator=(const hxarray& x_) {
	this->assign<const T_*>(x_.data(), x_.m_end_);
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
void hxarray<T_, capacity_>::operator=(const hxarray<T_, capacity_x_>& x_) {
	this->assign<const T_*>(x_.data(), x_.end());
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::operator=(hxarray&& x_) {
	this->swap(x_);
}

template<typename T_, size_t capacity_>
template<typename other_value_t_, size_t array_length_>
void hxarray<T_, capacity_>::operator=(const other_value_t_(&array_)[array_length_]) {
	this->assign(array_ + 0, array_ + array_length_);
}

template<typename T_, size_t capacity_>
const T_& hxarray<T_, capacity_>::operator[](size_t index_) const {
	hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
	return this->data()[index_];
}

template<typename T_, size_t capacity_>
T_& hxarray<T_, capacity_>::operator[](size_t index_) {
	hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
	return this->data()[index_];
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::operator+=(const T_& x_) {
	::new(this->push_back_unconstructed_()) T_(x_);
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::operator+=(T_&& x_) {
	::new(this->push_back_unconstructed_()) T_(hxmove(x_));
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
void hxarray<T_, capacity_>::operator+=(const hxarray<T_, capacity_x_>& x_) {
	for(const T_* hxrestrict it_ = x_.data(), *end_ = x_.end(); it_ != end_; ++it_) {
		::new(this->push_back_unconstructed_()) T_(*it_);
	}
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
void hxarray<T_, capacity_>::operator+=(hxarray<T_, capacity_x_>&& x_) {
	// Non-const mutable operation.
	for(T_* hxrestrict it_ = x_.data(), *end_ = x_.end(); it_ != end_; ++it_) {
		::new(this->push_back_unconstructed_()) T_(hxmove(*it_));
	}
}

template<typename T_, size_t capacity_>
template<typename functor_t_>
bool hxarray<T_, capacity_>::all_of(functor_t_&& fn_) const {
	for(const T_* it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
		if(!hxforward<functor_t_>(fn_)(*it_)) {
			return false;
		}
	}
	return true;
}

template<typename T_, size_t capacity_>
template<typename functor_t_>
bool hxarray<T_, capacity_>::any_of(functor_t_&& fn_) const {
	for(const T_* it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
		if(hxforward<functor_t_>(fn_)(*it_)) {
			return true;
		}
	}
	return false;
}

template<typename T_, size_t capacity_>
template<typename iter_t_>
void hxarray<T_, capacity_>::assign(iter_t_ begin_, iter_t_ end_) {
	hxassertmsg((end_ - begin_) >= 0, "invalid_iterator");
	this->reserve((size_t)(end_ - begin_));
	T_* hxrestrict it0_ = this->data();
	this->destruct_(it0_, m_end_);
	iter_t_ it1_(begin_); // begin_ may be a reference.
	while(it1_ != end_) {
		::new(it0_++) T_(*it1_++);
	}
	m_end_ = it0_;
}

template<typename T_, size_t capacity_>
const T_& hxarray<T_, capacity_>::back(void) const {
	hxassertmsg(!this->empty(), "invalid_reference");
	return m_end_[-1];
}

#if HX_CPLUSPLUS >= 202002L
template<typename T_, size_t capacity_>
template<typename range_t_>
void hxarray<T_, capacity_>::assign_range(range_t_& range_) {
	this->assign(range_.begin(), range_.end());
}

template<typename T_, size_t capacity_>
template<typename range_t_, typename>
void hxarray<T_, capacity_>::assign_range(range_t_&& range_) {
	this->clear();
	// Sorry, std::begin and std::end may not exist.
	for(auto it_ = range_.begin(), end_ = range_.end(); it_ != end_; ++it_) {
		::new(this->push_back_unconstructed_()) T_(hxmove(*it_));
	}
}
#endif

template<typename T_, size_t capacity_>
T_& hxarray<T_, capacity_>::back(void) {
	hxassertmsg(!this->empty(), "invalid_reference");
	return m_end_[-1];
}

template<typename T_, size_t capacity_>
const T_* hxarray<T_, capacity_>::begin(void) const {
	return this->data();
}

template<typename T_, size_t capacity_>
T_* hxarray<T_, capacity_>::begin(void) {
	return this->data();
}

template<typename T_, size_t capacity_>
const T_* hxarray<T_, capacity_>::cbegin(void) const {
	return this->data();
}

template<typename T_, size_t capacity_>
const T_* hxarray<T_, capacity_>::cend(void) const {
	return m_end_;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::clear(void) {
	this->destruct_(this->data(), m_end_);
	m_end_ = this->data();
}

template<typename T_, size_t capacity_>
template<typename... args_t_>
T_& hxarray<T_, capacity_>::emplace_back(args_t_&&... args_) {
	return *::new(this->push_back_unconstructed_()) T_(hxforward<args_t_>(args_)...);
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_, typename equal_t_>
bool hxarray<T_, capacity_>::equal(const hxarray<T_, capacity_x_>& x_, const equal_t_& equal_) const {
	if(this->size() != x_.size()) {
		return false;
	}
	for(const T_* it0_ = this->data(), *it1_ = x_.data(), *end_ = m_end_;
			it0_ != end_; ++it0_, ++it1_) {
		if(!equal_(*it0_, *it1_)) {
			return false;
		}
	}
	return true;
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
bool hxarray<T_, capacity_>::equal(const hxarray<T_, capacity_x_>& x_) const {
	return this->equal(x_, hxkey_equal_function<T_, T_>());
}

template<typename T_, size_t capacity_>
bool hxarray<T_, capacity_>::empty(void) const {
	return m_end_ == this->data();
}

template<typename T_, size_t capacity_>
const T_* hxarray<T_, capacity_>::end(void) const {
	return m_end_;
}

template<typename T_, size_t capacity_>
T_* hxarray<T_, capacity_>::end(void) {
	return m_end_;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::erase(const T_* pos_) {
	hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
	while((pos_ + 1) != m_end_) {
		// Having a non-const this pointer provides valid write access.
		*const_cast<T_*>(pos_) = hxmove(*(pos_ + 1));
		++pos_;
	}
	(--m_end_)->~T_();
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::erase(size_t index_) {
	hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
	this->erase(this->data() + index_);
}

template<typename T_, size_t capacity_>
template<typename functor_t_>
size_t hxarray<T_, capacity_>::erase_if(functor_t_&& fn_) {
	size_t removed_ = 0u;
	for(size_t index_ = this->size(); index_--;) {
		if(hxforward<functor_t_>(fn_)(this->data()[index_])) {
			this->erase_unordered(index_);
			++removed_;
		}
	}
	return removed_;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::erase_unordered(const T_* pos_) {
	hxassertmsg(pos_ >= this->data() && pos_ < m_end_, "invalid_iterator");
	if(pos_ != --m_end_) {
		// Having a non-const this pointer provides valid write access.
		*const_cast<T_*>(pos_) = hxmove(*m_end_);
	}
	m_end_->~T_();
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::erase_unordered(size_t index_) {
	hxassertmsg(index_ < this->size(), "invalid_index %zu", index_);
	this->erase_unordered(this->data() + index_);
}

template<typename T_, size_t capacity_>
template<typename functor_t_>
void hxarray<T_, capacity_>::for_each(functor_t_&& fn_) {
	for(T_* it_ = this->data(), *end_ = m_end_; it_ != end_; ++it_) {
		hxforward<functor_t_>(fn_)(*it_);
	}
}

template<typename T_, size_t capacity_>
const T_& hxarray<T_, capacity_>::front(void) const {
	hxassertmsg(!this->empty(), "invalid_reference");
	return *this->data();
}

template<typename T_, size_t capacity_>
T_& hxarray<T_, capacity_>::front(void) {
	hxassertmsg(!this->empty(), "invalid_reference");
	return *this->data();
}

template<typename T_, size_t capacity_>
bool hxarray<T_, capacity_>::full(void) const {
	return m_end_ == this->data() + this->capacity();
}

template<typename T_, size_t capacity_>
const T_* hxarray<T_, capacity_>::get(size_t index_) const {
	// Casting a signed index is well defined. Comparing pointers would be undefined.
	return index_ < this->size() ? this->data() + index_ : hxnull;
}

template<typename T_, size_t capacity_>
T_* hxarray<T_, capacity_>::get(size_t index_) {
	// Casting a signed index is well defined. Comparing pointers would be undefined.
	return index_ < this->size() ? this->data() + index_ : hxnull;
}

template<typename T_, size_t capacity_>
template<typename ref_t_>
void hxarray<T_, capacity_>::insert(const T_* pos_, ref_t_&& x_) {
	hxassertmsg(pos_ >= this->data() && pos_ <= m_end_, "invalid_insert");
	if(pos_ == m_end_) {
		// Single constructor call for last element.
		::new(m_end_++) T_(hxforward<ref_t_>(x_));
	}
	else {
		// A copy constructor for a new end element followed by a series of
		// assignment operations.
		T_* it_ = m_end_++;
		::new(it_) T_(it_[-1]);
		while(pos_ < --it_) {
			*it_ = it_[-1];
		}
		*it_ = hxforward<ref_t_>(x_);
	}
}

template<typename T_, size_t capacity_>
template<typename ref_t_>
void hxarray<T_, capacity_>::insert(size_t index_, ref_t_&& x_) {
	hxassertmsg(index_ <= this->size(), "invalid_index %zu", index_);
	this->insert(this->data() + index_, hxforward<ref_t_>(x_));
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_, typename less_t_, typename equal_t_>
bool hxarray<T_, capacity_>::less(const hxarray<T_, capacity_x_>& x_, const less_t_& less_, const equal_t_& equal_) const {
	size_t size_ = hxmin(this->size(), x_.size());
	for(const T_* it0_ = this->data(), *it1_ = x_.data(), *end_ = it0_ + size_;
			it0_ != end_; ++it0_, ++it1_) {
		// Use `a == b` instead of `a < b && b < a` for performance.
		if(!equal_(*it0_, *it1_)) {
			return less_(*it0_, *it1_);
		}
	}
	// Order the prefix before the other.
	return this->size() < x_.size();
}

template<typename T_, size_t capacity_>
template<size_t capacity_x_>
bool hxarray<T_, capacity_>::less(const hxarray<T_, capacity_x_>& x_) const {
	return this->less(x_, hxkey_less_function<T_, T_>(), hxkey_equal_function<T_, T_>());
}

template<typename T_, size_t capacity_>
size_t hxarray<T_, capacity_>::max_size(void) const {
	return this->capacity();
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::pop_back(void) {
	hxassertmsg(!this->empty(), "stack_underflow");
	(--m_end_)->~T_();
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::pop_heap(void) {
	hxassertmsg(!this->empty(), "stack_underflow");
	T_* hxrestrict begin_ = this->data();
	--m_end_;
	if(begin_ == m_end_) {
		begin_->~T_();
		return;
	}
	*begin_ = hxmove(*m_end_);
	m_end_->~T_();
	hxdetail_::hxheapsort_heapify_(begin_, m_end_, hxkey_less_function<T_, T_>());
}

template<typename T_, size_t capacity_>
template<typename... args_t_>
T_& hxarray<T_, capacity_>::push_back(args_t_&&... args_) {
	return *::new(this->push_back_unconstructed_()) T_(hxforward<args_t_>(args_)...);
}

template<typename T_, size_t capacity_>
template<typename ref_t_>
T_& hxarray<T_, capacity_>::push_heap(ref_t_&& arg_) {
	T_* begin_ = this->data();
	T_* node_ = static_cast<T_*>(this->push_back_unconstructed_());
	while(node_ != begin_) {
		T_* parent_ = begin_ + ((node_ - begin_ - 1) >> 1);
		// arg_ has to be comparable to T_.
		if(!hxkey_less(*parent_, arg_)) {
			break;
		}
		// Shifts unconstructed element into position.
		::new((void*)node_) T_(hxmove(*parent_));
		parent_->~T_();
		node_ = parent_;
	}
	// Construct new element.
	::new((void*)node_) T_(hxmove(arg_));
	return *node_;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::reserve(size_t size_,
		hxsystem_allocator_t allocator_,
		hxalignment_t alignment_) {
	T_* prev = this->data();
	this->reserve_storage(size_, allocator_, alignment_);
	hxassertmsg(!prev || prev == this->data(), "reallocation_disallowed"); (void)prev;
	if(m_end_ == hxnull) {
		m_end_ = this->data();
	}
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::resize(size_t size_) {
	this->reserve(size_);
	T_* end_ = this->data() + size_;
	if(size_ >= this->size()) {
		while(m_end_ != end_) {
			// This version uses a default constructor.
			::new(this->push_back_unconstructed_()) T_();
		}
	}
	else {
		this->destruct_(end_, m_end_);
	}
	m_end_ = end_;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::resize(size_t size_, const T_& x_) {
	this->reserve(size_);
	T_* end_ = this->data() + size_;
	if(size_ >= this->size()) {
		while(m_end_ != end_) {
			// This version uses a copy constructor.
			::new(this->push_back_unconstructed_()) T_(x_);
		}
	}
	else {
		this->destruct_(end_, m_end_);
	}
	m_end_ = end_;
}

template<typename T_, size_t capacity_>
size_t hxarray<T_, capacity_>::size(void) const {
	return (size_t)(m_end_ - this->data());
}

template<typename T_, size_t capacity_>
size_t hxarray<T_, capacity_>::size_bytes(void) const {
	return sizeof(T_) * (size_t)(m_end_ - this->data());
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::swap(hxarray& x_) {
	static_assert(capacity_ == hxallocator_dynamic_capacity,
		"Dynamic capacity required for hxarray::swap");

	// hxarray has a dynamic allocator that allows memcpy.
	hxswap_memcpy(*this, x_);
}

template<typename T_, size_t capacity_>
void* hxarray<T_, capacity_>::push_back_unconstructed_(void) {
	hxassertmsg(!this->full(), "stack_overflow");
	return (void*)m_end_++;
}

template<typename T_, size_t capacity_>
void hxarray<T_, capacity_>::destruct_(T_* begin_, T_* end_) {
	while(begin_ != end_) {
		begin_++->~T_();
	}
}
