#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxsort.hpp Sorting and searching utilities for hatchling platform.
/// Provides insertion sort, binary search, and a general purpose sort
/// implementation. Includes support for template partial specialization
/// (overloads of `hxkey_equal`, `hxkey_less`, `hxswap`) and functors when defining
/// custom key operations. Otherwise `T::T(T&&)`, `T::~T()`, `T::operator=(T&&)`,
/// `T::operator<(const T&)` and `T::operator==(const T&)` are used.
///
/// `hxradix_sort.hpp` is recommended as an `Θ(n)` sorting strategy for any
/// primitive type that is 4-bytes or less. This implementation does not cause
/// code bloat and is the fastest sorting algorithm available for scalar keys.
/// Radix sort is best when you need real-time guarantees and have a massive
/// workload. This is not a toy. It was actually how IBM sorted punch cards.
///
/// `hxinsertion_sort` is recommended when you have under a kilobyte of data to
/// sort and you don't want to add 10k to your executable just to sort it.
/// `hxheap_sort` may also be useful for keeping code size down while providing
/// `Θ(n log n)`.
///
/// `hxsort` is meant to be competitive with smaller types and "resistant to
/// attack." It instantiates about 200 lines of template code.
///
/// If sorting is important to your application then the "cpp-sort" project is
/// recommended as a way to study your data and identify the best algorithms for
/// you: https://github.com/Morwenn/cpp-sort.
///
/// If you do find you need a specific algorithm that isn't here then take a look
/// at `hxintro_sort_` as an example of how you might compose a new sorting
/// function built from these routines.

#include "detail/hxsort_detail.hpp"

/// `hxinsertion_sort` - Sorts the elements in the range `[begin, end)` in
/// comparison order using the insertion sort algorithm. The `end` parameter
/// points just past the end of the array. Exception handling during operation
/// is undefined. Declare your copy constructor and assignment operator
/// `noexcept` or turn off exceptions. All of `T::T(&&)`, `T::~T()` and
/// `T::operator=(T&&)` are used.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor definining a less-than ordering relationship.
template<typename T_, typename less_t_>
void hxinsertion_sort(T_* begin_, T_* end_, const less_t_& less_) {
	// Address sanitizer: Avoids adding 1 to null iterators.
	if(begin_ == end_) { return; }

	for (T_ *i_ = begin_, *j_ = begin_ + 1; j_ < end_; i_ = j_++) {
		if (!less_(*i_, *j_)) {
			// Default value construct. Use hxmove instead of hxswap because it
			// should be more efficient for simple types. Complex types will
			// require an T::operator=(T&&) to be efficient.
			T_ t_ = hxmove(*j_);
			*j_ = hxmove(*i_);
			while (begin_ < i_ && !less_(i_[-1], t_)) {
				*i_ = hxmove(i_[-1]);
				--i_;
			}
			*i_ = hxmove(t_);
		}
	}
}

/// `hxinsertion_sort (specialization)` - An overload of `hxinsertion_sort` that
/// uses `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename T_>
void hxinsertion_sort(T_* begin_, T_* end_) {
	hxinsertion_sort(begin_, end_, hxkey_less_function<T_, T_>());
}

/// `hxheapsort` - Sorts the elements in the range `[begin, end)` in comparison
/// order using the heapsort algorithm.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor definining a less-than ordering relationship.
template<typename T_, typename less_t_>
void hxheapsort(T_* begin_, T_* end_, const less_t_& less_) {
	ptrdiff_t sz_ = end_ - begin_;
	if (sz_ <= 1) {
		// Sequence is already sorted and the following code assumes sz >= 2.
		return;
	}

	// Find the index of the first parent node. It will be >= 0 because sz >= 2.
	ptrdiff_t first_parent_index_ = sz_ / 2u - 1u;

	// The first grandparent is just clamped to >= 0 because it avoids a special
	// case when it does not exist.
	ptrdiff_t first_grandparent_index_ = hxmax<ptrdiff_t>(sz_ / 4u - 1u, 0u);
	T_* first_grandparent_ = begin_ + first_grandparent_index_;

	// Do not visit leaf nodes at all. Then run an optimized version on half the
	// remaining values that doesn't recurse and do range checks on children
	// that don't exist. This still handles the case where the right child is
	// missing from the last parent.
	for (T_* it_ = begin_ + first_parent_index_; it_ > first_grandparent_; --it_) {
		hxheapsort_heapify_bottom_(begin_, end_, it_, less_);
	}

	// Heapify the grandparents using in iterative method.
	for (T_* it_ = first_grandparent_; it_ >= begin_; --it_) {
		hxheapsort_heapify_(begin_, end_, it_, less_);
	}

    // Sort phase. Swap the largest values to the end of the array.
    for (T_* it_ = end_ - 1; it_ > begin_; --it_) {
        hxswap(*begin_, *it_);
        hxheapsort_heapify_(begin_, it_, begin_, less_);
    }
}

/// `hxheapsort (specialization)` - An overload of `hxheapsort` that uses
/// `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename T_>
void hxheapsort(T_* begin_, T_* end_) {
	hxheapsort(begin_, end_, hxkey_less_function<T_, T_>());
}

/// `hxsort` - A general purpose sort routine using `T::T()`, `T::~T()`,
/// `T::operator=(&&)`, the `hxswap` overloads and a `less` functor which
/// defaults to `hxkey_less`. This version is intended for sorting large numbers
/// of small objects.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor definining a less-than ordering relationship.
template<typename T_, typename less_t_>
void hxsort(T_* begin_, T_* end_, const less_t_& less_) {
	hxintro_sort_(begin_, end_, less_, 2u * hxlog2i(end_ - begin_));
}

/// `hxsort (specialization)` - An overload of `hxsort` that uses `hxkey_less`.
/// This version is intended for sorting large numbers of small objects.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename T_>
void hxsort(T_* begin_, T_* end_) {
	hxintro_sort_(begin_, end_, hxkey_less_function<T_, T_>(), 2u * hxlog2i(end_ - begin_));
}

/// `hxbinary_search` - Performs a binary search in the range [first, last).
/// Returns `null` if the value is not found. Unsorted data will lead to errors.
/// Non-unique values will be selected from arbitrarily. The compare parameter
/// is a functor that returns true if the first argument is ordered before (i.e.
/// is less than) the second. See `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `val` : The value to search for.
/// - `less` : A key comparison functor definining a less-than ordering relationship. (Optional.)
template<typename T_, typename less_t_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_, const less_t_& less_) {
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

/// `hxbinary_search (specialization)` - An overload of `hxbinary_search` that
/// uses `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `val` : The value to search for.
template<typename T_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_) {
	return hxbinary_search(begin_, end_, val_, hxkey_less_function<T_, T_>());
}

/// `hxbinary_search` - Non-const overload of binary search.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `val` : The value to search for.
/// - `less` : A key comparison functor definining a less-than ordering relationship. (Optional.)
template<typename T_, typename less_t_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_, const less_t_& less_) {
	return const_cast<T_*>(hxbinary_search(const_cast<const T_*>(begin_),
		const_cast<const T_*>(end_), val_, less_));
}

/// `hxbinary_search (specialization)` - Non-const overload using `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `val` : The value to search for.
template<typename T_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_) {
	return const_cast<T_*>(hxbinary_search(const_cast<const T_*>(begin_),
		const_cast<const T_*>(end_), val_, hxkey_less_function<T_, T_>()));
}
