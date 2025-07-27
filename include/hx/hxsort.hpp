#pragma once
// Copyright 2017-2025 Adrian Johnston
//
// hxsort.hpp - Sorting and searching utilities for hatchling platform. Provides
// insertion sort, binary search, and radix sort implementations for arrays and
// key-value pairs. Includes generic and specialized templates for sorting with
// custom comparators. Designed for use with hxarray and hatchling memory
// management. No exception safety is guaranteed; use noexcept types or disable
// exceptions.

#include <hx/hatchling.h>
#include <hx/hxarray.hpp>

/// `hxinsertion_sort` - Sorts the elements in the range `[begin, end)` in comparison
/// order using the insertion sort algorithm. The `end` parameter points just past the
/// end of the array. Exception handling during operation is undefined. Declare your
/// copy constructor and assignment operator `noexcept` or turn off exceptions.
/// Both `T::T(T&)` and `T::operator=(T&)` are used while `T::T()` is not. See
/// hxinsertion_sort_using_swap if you want move semantics using &&.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor definining a less-than ordering relationship.
template<typename T_, typename less_t_>
void hxinsertion_sort(T_* begin_, T_* end_, const less_t_& less_) {
	if(begin_ == end_) { return; } // Address sanitizer: Avoids adding 1 to null iterators.

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

/// `hxinsertion_sort (specialization)` - An overload of `hxinsertion_sort` that uses
/// `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename T_>
void hxinsertion_sort(T_* begin_, T_* end_) {
	hxinsertion_sort(begin_, end_, hxkey_less_function<T_>());
}

/// `hxinsertion_sort_using_swap` - A version of `hxinsertion_sort` that uses
/// `hxswap` and `T::T()`. Neither `T::T(T&)` or `T::operator=(T&)` are used to
/// make copies. This provides move semantics without speculating based on the
/// operator overloads of T. See the documentation for `hxinsertion_sort`.
/// XXX Check the generated assembly and see if this can be used for fundamental types.
template<typename T_, typename less_t_>
void hxinsertion_sort_using_swap(T_* begin_, T_* end_, const less_t_& less_) {
	if(begin_ == end_) { return; } // Address sanitizer: Avoids adding 1 to null iterators.

	// i points to insertion location. j points to next unsorted value.
	for (T_ *i_ = begin_, *j_ = begin_ + 1; j_ < end_; i_ = j_++) {
		if (!less_(*i_, *j_)) {
			T_ t_; // construct
			hxswap(t_, *j_);
			hxswap(*j_, *i_);
			while (begin_ < i_ && !less_(i_[-1], t_)) {
				hxswap(i_[0], i_[-1]);
				--i_;
			}
			hxswap(*i_, t_);
		}
	}
}

// Restores the heap property by pushing a value down until it is not
// greater than it's children. The heap property is (current >= left &&
// current >= right) for all values in a heap that have left and right
// children.
template<typename T_, typename less_t_>
void hxheapsort_heapify_(T_* begin_, T_* end_, T_* current_, const less_t_& less_) {
	for (;;) {
		T_* left_  = begin_ + 2 * (current_ - begin_) + 1;
		T_* right_ = left_ + 1;

		if (right_ < end_ && less_(*current_, *right_)) {
			// left_ is inside if right_ is.
			if (less_(*left_, *right_)) {
				hxswap(*current_, *right_);
				current_ = right_;
			}
			else {
				hxswap(*current_, *left_);
				current_ = left_;
			}
		}
		else if (left_ < end_ && less_(*current_, *left_)) {
			hxswap(*current_, *left_);
			current_ = left_;
		}
		else {
			break;
		}
		heapify_(begin_, end_, current_, less_);
	}
}

// Used for parents but not grandparents.
template<typename T_, typename less_t_>
void hxheapsort_heapify_bottom_(T_* begin_, T_* end_, T_* current_, const less_t_& less_) {
	T_* left_  = begin_ + 2 * (current_ - begin_) + 1;
	T_* right_ = left_ + 1;

	// The very last parent may be missing a right child.
	if (right_ < end_ && less_(*current_, *right_)) {
		if (less_(*left_, *right_)) {
			hxswap(*current_, *right_);
		}
		else {
			hxswap(*current_, *left_);
		}
	}
	else if (less_(*current_, *left_)) {
		hxswap(*current_, *left_);
	}
}

/// XXX This isn't ready.
template<typename T_, typename less_t_>
void hxheapsort(T_* begin_, T_* end_, const less_t_& less_) {

	size_t sz_ = end_ - begin_;
	if (sz_ <= 1) {
		// Already sorted and the following code makes assumptions.
		return;
	}

	// Find the index of the first parent node. It will be >= 0.
	size_t first_parent_index_ = sz_ / 2u - 1u;

	// The first grandparent is just clamped to >= 0 because it is harmless.
	size_t first_grandparent_index_ = hxmax(sz_ / 4u - 1u, (size_t)0u);
	T_ first_grandparent_ = begin_ + first_grandparent_index_;

	// Do not visit leaf nodes at all. Run an optimized version on half the
	// remaining values that doesn't recurse and do range checks on children
	// that don't exist. This still handles the case where the right child is
	// missing.
	for (T_* it_ = begin_ + first_parent_index_; it_ > first_grandparent_; --it_) {
		hxheapsort_heapify_bottom_(begin_, end_, it_, less_);
	}

	for (T_* it_ = first_grandparent_; it_ >= begin_; --it_) {
		hxheapsort_heapify_(begin_, end_, it_, less_);
	}

    // Sort phase.
    for (T_* it_ = end_ - 1; it_ > begin_; --it_) {
        hxswap(*begin_, *it_);
        hxheapsort_heapify_(begin_, it_, begin_, less_);
    }
}


/// XXX docs todo
/// XXX have a test class that asserts when you swap it with itself.
/// The Lomuto partition scheme.
template<typename T, typename less_t_>
void hxpartition_sort(T* begin_, T* end_, const less_t_& less_) {
	// Avoid pissing off the sanitizer by subtracting null iterators.
	if(begin_ == end_) { return; }

	// 16 values is the standard cutoff for switching to insertion sort.
	if ((end_ - begin_) < 16) {
		hxinsertion_sort(begin_, end_, less_);
	} else {
		// Chose the end value as the location to store the pivot value.
		T* pivot_value_ = end_ - 1;

		// Find the median-of-three values (begin, mid, end-1) to use as a pivot.
		// Swap the selected value up to the end of the array as needed.
        T* mid_ = begin_ + (end_ - begin_) / 2;
		if (hxless(*begin_, *mid_)) {
			if (hxless(*mid_, *pivot_value_)) {
				hxswap(*mid_, *pivot_value_); // begin < mid < pivot
			}
			else if(hxless(*pivot_value_, *begin_)) {
				hxswap(*pivot_value_, *begin_); // pivot < begin < mid
			}
		}
		else {
			if(hxless(*begin_, *pivot_value_)) {
				hxswap(*pivot_value_, *begin_); // mid < begin < pivot
			}
			else if (hxless(*pivot_value_, *mid_)) {
				hxswap(*mid_, *pivot_value_); // pivot < mid < begin
			}
		}

		// Values below the boundary are less than the pivot.
		T* pivot_boundary_ = begin_;

		// Start looking for where to put the pivot by scanning for values
		// larger than it. (This is a slight modification.) It is OK if values
		// equivalent to the partition (!(a < b) && !(b < a)) end up on either
		// side. They will all get sorted to be adjacent eventually.
		for (; pivot_boundary_ < pivot_value_; ++pivot_boundary_) {
			if (less_(*pivot_value_, *pivot_boundary_)) {
				// A larger value than the pivot has been found. Start swapping
				// smaller values down below the larger values.
				for (T it_ = pivot_boundary_ + 1; it_ < pivot_value_; ++it_) {
					if (less_(*it_, *pivot_value_)) {
						// Extend the "less than pivot" section.
						hxswap(*pivot_boundary_, *it_);
						++pivot_boundary_;
					}
				}

				// Done sorting. Swap the pivot into location.
				hxswap(*pivot_boundary_, *pivot_value_);
				break;
			}
		}

		// The value at pivot_boundary_ is not moved again. Sort the ranges
		// before and after.
		hxpartition_sort(begin_, pivot_boundary_, less_);
		hxpartition_sort((pivot_boundary_ + 1), end_, less_);
	}
}

/// `hxbinary_search` - Performs a binary search in the range [first, last). Returns
/// `null` if the value is not found. Unsorted data will lead to errors.
/// Non-unique values will be selected from arbitrarily.
///
/// The compare parameter is a function object that returns true if the first
/// argument is ordered before (i.e. is less than) the second. `See hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `val` : The value to search for.
/// - `less` : Comparison function object.
template<typename T_, typename less_t_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_, const less_t_& less_) {
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
	return hxbinary_search(begin_, end_, val_, hxkey_less_function<T_>());
}

/// const correct wrapper
template<typename T_, typename less_t_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_, const less_t_& less_) {
	return hxbinary_search(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_, less_);
}

// c++98 junk.
template<typename T_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_) {
	return hxbinary_search(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_,
		hxkey_less_function<T_>());
}
