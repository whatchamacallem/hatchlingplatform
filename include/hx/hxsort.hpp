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

// XXX Make sure arrays and hash tables can be move assigned to.
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
	if(begin_ == end_) { return; } // Address sanitizer: Avoids adding 1 to null iterators.

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

/// `hxinsertion_sort (specialization)` - An overload of `hxinsertion_sort` that uses
/// `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename T_>
void hxinsertion_sort(T_* begin_, T_* end_) {
	hxinsertion_sort(begin_, end_, hxkey_less_function<T_>());
}

namespace hxdetail_ {
// Restores the heap property by pushing a value down until it is not
// greater than it's children.
template<typename T_, typename less_t_>
void hxheapsort_heapify_(T_* begin_, T_* end_, T_* current_, const less_t_& less_) {
	for (;;) {
		hxassertmsg(begin_ >= current_ && current_ < end_, "invalid_iterator");

		T_* left_  = begin_ + 2 * (current_ - begin_) + 1;
		T_* right_ = left_ + 1;
		T_* next_  = 0;

		if (right_ < end_) {
			// Deal with the case where there are two children first.
			if (less_(*left_, *right_)) {
				next_ = right_;
			}
			else {
				next_ = left_;
			}
			if (less_(*next_, *current_)) {
				// Neither child greater.
				return;
			}
		}
		else if (right_ == end_) {
			// Special case for a single child. It will have no child itself.
			// Hopefully this uses the processor status flags from the last compare.
			if (less_(*current_, *left_)) {
				hxswap(*current_, *left_);
			}
			return;
		}
		else {
			return; // No children.
		}

		hxswap(*current_, *next_);
		current_ = next_;
	}
}

// Used for parents but not grandparents.
template<typename T_, typename less_t_>
void hxheapsort_heapify_bottom_(T_* begin_, T_* end_, T_* current_, const less_t_& less_) {
	T_* left_  = begin_ + 2 * (current_ - begin_) + 1;
	T_* right_ = left_ + 1;

	// NB The very last parent may be missing a right child.
	hxassertmsg(begin_ <= left_ && left_ < end_, "invalid_iterator");

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
} // namespace hxdetail_

/// XXX This isn't ready.
template<typename T_, typename less_t_>
void hxheapsort(T_* begin_, T_* end_, const less_t_& less_) {
	size_t sz_ = end_ - begin_;
	if (sz_ <= 1) {
		// Sequence is already sorted and the following code assumes sz >= 2.
		return;
	}

	// Find the index of the first parent node. It will be >= 0 because sz >= 2.
	size_t first_parent_index_ = sz_ / 2u - 1u;

	// The first grandparent is just clamped to >= 0 because it avoids a special
	// case when it does not exist.
	size_t first_grandparent_index_ = hxmax<size_t>(sz_ / 4u - 1u, 0u);
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

template<typename T_>
void hxheapsort(T_* begin_, T_* end_) {
	hxheapsort(begin_, end_, hxkey_less_function<T_>());
}

/// Sort `[begin_, end_)` in-place using Dual-Pivot QuickSort. Vladimir Yaroslavskiy 2009.
/// Average time: `Θ(n log n)`, worst time: `Θ(n²)`. This algorithm is only intended to sort
/// ranges over length 16 before calling back to the `sort_callback` parameter.
/// `sort_callback` : A functor with the signature `sort(T* begin, T* end, const less_t& less)`
template<typename T_, typename less_t_, typename sort_callback_t_>
void hxpartition_sort(	T_* begin_, T_* end_, const less_t_& less_,
						const sort_callback_t_& sort_callback_, int depth_) {
	hxassertmsg((begin_ - end_) > 16, "range_error Use hxinsertion_sort.");
	T_* end1_ = end_ - 1;

	// Use begin and end-1 as pivots p₁, p₂ with p₁ ≤ p₂
	if (less_(*end1_, *begin_)) {
		hxswap(*begin_, *end1_);
	}

	// Three-way partition into [<p₁], [p₁ ≤ … ≤ p₂], [>p₂]
	T_* lt_ = begin_ + 1; // Points to beginning of less-than range, after first pivot.
	T_* gt_ = end1_;      // Points to end of greater-than range, where the last pivot is.
	for (T_* it_ = lt_; it_ < gt_; ) {
		if (less_(*it_, *begin_)) {
			// Swap into less-than range and extend it.
			hxswap(*it_++, *lt_++);
		}
		else if (less_(*end1_, *it_)) {
			// Swap into greater-than range and extend it.
			hxswap(*it_, *--gt_);
		}
		else {
			// Leave the value in the mid range.
			++it_;
		}
	}

	// Swap pivots into final slots.
	--lt_; // last
	if(begin_ != lt_) {
		hxswap(*begin_, *lt_);
	}
	if(end1_ != gt_) {
		hxswap(*end1_, *gt_);
	}

	// Recurse on the three partitions. Do not re-sort the partition values.
	sort_callback_(begin_,  lt_ - 1,  less_, depth_);
	sort_callback_(lt_ + 1, gt_ - 1,  less_, depth_);
	sort_callback_(gt_ + 1, end_, less_, depth_);
}

// XXX An intrinsic would probably save a cache miss.
inline int hxlog2i(size_t n_) {
    static const char lookup_[32] = {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
    };
    int log = 0;
    while(n_ > 0xffu) { n_ >>= 8; log += 8; }
    if(n_ > 0xfu)     { n_ >>= 4; log += 4; }
	return log + lookup_[n_ & 0xf];
}

/// hxsort is implemented using hxintro_sort_. Uses Introsort, David R. Musser
/// 1997. hxintro_sort_ calls itself recursively until it hits its depth limit.
/// This is an experiment to see how the std::sort algorithm performs using raw
/// pointers.
template<typename T_, typename less_t_>
void hxintro_sort_(T_* begin_, T_* end_, const less_t_& less_, int depth_) {
	hxassertmsg(begin_ != hxnull && begin_ >= end_, "range_error hxsort");

	// 16 values is the standard cutoff for switching to insertion sort.
	if ((end_ - begin_) <= 16) {
		hxinsertion_sort(begin_, end_, less_);
	} else if (depth_ == 0u) {
		hxheapsort(begin_, end_, less_);
	} else {
		// Have the partition sort call back to hxsort for each sub-partition.
		hxpartition_sort(begin_, end_, less_, hxintro_sort_<T_, less_t_>, depth_ - 1u);
	}
}

/// A general purpose sort routine using `T::T()`, `T::~T()`, `T::operator=(&&)`,
/// the `hxswap` overloads and a `less` functor which defaults to `hxless`.
template<typename T_, typename less_t_>
void hxsort(T_* begin_, T_* end_, const less_t_& less_) {
	hxintro_sort_(begin_, end_, less_, 2u * hxlog2i( end_ - begin_));
}

template<typename T_>
void hxsort(T_* begin_, T_* end_) {
	hxintro_sort_(begin_, end_, hxkey_less_function<T_>(), 2u * hxlog2i( end_ - begin_));
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
/// - `less` : Comparison function object. (Optional)
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

// Overload using hxkey_less.
template<typename T_>
const T_* hxbinary_search(const T_* begin_, const T_* end_, const T_& val_) {
	return hxbinary_search(begin_, end_, val_, hxkey_less_function<T_>());
}

// Non-const overload.
template<typename T_, typename less_t_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_, const less_t_& less_) {
	return const_cast<T_*>(hxbinary_search(begin_, end_, val_, less_));
}

// Non-const overload using hxkey_less.
template<typename T_>
T_* hxbinary_search(T_* begin_, T_* end_, const T_& val_) {
	return const_cast<T_*>(hxbinary_search(begin_, end_, val_, hxkey_less_function<T_>()));
}
