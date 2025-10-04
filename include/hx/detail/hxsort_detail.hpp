#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../hxkey.hpp"

template<typename T_, typename less_t_> hxattr_nonnull(1,2) hxattr_hot
void hxinsertion_sort(T_* begin_, T_* end_, const less_t_& less_);

template<typename T_, typename less_t_> hxattr_nonnull(1,2) hxattr_hot
void hxheapsort(T_* begin_, T_* end_, const less_t_& less_);

namespace hxdetail_ {

enum : ptrdiff_t { hxpartition_sort_cutoff_ = 32 };

/// Restores the heap property by pushing a value down until it is not greater
/// than its children.
/// - `begin` : Pointer to the first element in the heap.
/// - `end` : Pointer to one past the last element in the heap.
/// - `current` : Pointer to the current element being heapified.
/// - `less` : Comparison functor.
template<typename T_, typename less_t_> hxattr_nonnull(1,2,3) hxattr_hot
void hxheapsort_heapify_(T_* begin_, const T_* end_, T_* current_, const less_t_& less_) {
	hxassertmsg(begin_ <= current_ && current_ < end_, "invalid_iterator");
	for(;;) {
		T_* left_ = begin_ + (((current_ - begin_) << 1) + 1);
		if(left_ >= end_) {
			return;
		}

		T_* next_ = left_;
		T_* right_ = left_ + 1;
		if(right_ < end_ && less_(*next_, *right_)) {
			next_ = right_;
		}

		if(!less_(*current_, *next_)) {
			return;
		}

		hxswap(*current_, *next_);
		current_ = next_;
	}
}

/// `hxmake_heap_` - Converts the range `[begin, end)` into a max heap using the
/// provided comparator. Should work well for mostly heapified data.
/// - `begin` : Pointer to the beginning of the range to heapify.
/// - `end` : Pointer to one past the last element in the range to heapify.
/// - `less` : A key comparison functor definining a less-than ordering relationship.
template<typename T_, typename less_t_> hxattr_nonnull(1,2) hxattr_hot
void hxmake_heap_(T_* begin_, T_* end_, const less_t_& less_) {
	for(T_* heap_end_ = begin_ + 1; heap_end_ < end_; ) {
		T_* node_ = heap_end_++;
		T_* parent_ = begin_ + ((node_ - begin_ - 1) >> 1);
		if(less_(*parent_, *node_)) {
			T_ value_ = hxmove(*node_);
			do {
				*node_ = hxmove(*parent_);
				node_ = parent_;
				if(node_ == begin_) {
					break;
				}
				parent_ = begin_ + ((node_ - begin_ - 1) >> 1);
			}
			while(less_(*parent_, value_))
				/**/;
			*node_ = hxmove(value_);
		}
	}
}

/// Sort `[begin, end)` in-place using Dual-Pivot QuickSort. Based on Java's
/// `Array.sort` implementation details. Should be resistant to degeneration.
/// Average time: `Θ(n log n)`, worst time: `Θ(n²)`. This algorithm is only
/// intended to sort ranges over a minimum length before calling back to the
/// `sort_callback` parameter.
/// - `begin` : Pointer to the first element in the range.
/// - `end` : Pointer to one past the last element in the range.
/// - `less` : Comparison functor.
/// - `callback` : Callback functor matching `void callback(T* begin, T* end, const
/// less_t& less, int depth)` for recursive sorting.
/// - `depth` : Current recursion depth.
template<typename T_, typename less_t_, typename sort_callback_t_>  hxattr_nonnull(1,2) hxattr_hot
void hxpartition_sort_(	T_* begin_, T_* end_, const less_t_& less_,
						const sort_callback_t_& sort_callback_, int depth_) {
	hxassertmsg((end_ - begin_) > hxpartition_sort_cutoff_, "range_error Use hxinsertion_sort.");
	ptrdiff_t length_ = end_ - begin_;

	// Select 5 pivot values at 1/7th increments. And allow them to be naturally
	// sorted.
    ptrdiff_t seventh_ = (length_ >> 3) + (length_ >> 6) + 1;
    T_* p2_ = begin_ + (length_ >> 1);
    T_* p1_ = p2_ - seventh_;
    T_* p0_ = p1_ - seventh_;
    T_* p3_ = p2_ + seventh_;
    T_* p4_ = p3_ + seventh_;

	// This is a Bose-Nelson sorting network for 5 elements. It should work well
	// with a processor that has branch prediction.
	if(less_(*p3_, *p0_)) { hxswap(p3_, p0_); }
	if(less_(*p4_, *p1_)) { hxswap(p4_, p1_); }
	if(less_(*p2_, *p0_)) { hxswap(p2_, p0_); }
	if(less_(*p3_, *p1_)) { hxswap(p3_, p1_); }
	if(less_(*p1_, *p0_)) { hxswap(p1_, p0_); }
	if(less_(*p4_, *p2_)) { hxswap(p4_, p2_); }
	if(less_(*p2_, *p1_)) { hxswap(p2_, p1_); }
	if(less_(*p4_, *p3_)) { hxswap(p4_, p3_); }
	if(less_(*p3_, *p2_)) { hxswap(p3_, p2_); }

    T_* back_ = end_ - 1; // Pointer to the last value.

	// Move the selected pivots out of the way by placing them at the ends of
	// the range.
    hxswap(*begin_, *p1_);
    hxswap(*back_, *p3_);

	// Three-way partition into [<p₁], [p₁ ≤ … ≤ p₂], [>p₂]

	// Points to end of less-than range, which is empty and is right after the
	// first pivot.
	T_* lt_ = begin_ + 1;
	// Points to end of greater-than range, which is empty and right before the
	// last pivot. This is an end iterator that goes left.
	T_* gt_ = back_ - 1;

	for(T_* i_ = lt_; i_ <= gt_; ) {
		if(less_(*i_, *begin_)) {
			// Swap into less-than range and extend it.
			if(lt_ != i_) {
				hxswap(*i_, *lt_);
			}
			else {
				++i_;
			}
			++lt_;
		}
		else if(less_(*back_, *i_)) {
			// Swap into greater-than range and extend it. If gt == i then the
			// loop is about to terminate due to --gt.
			if(gt_ != i_) {
				hxswap(*i_, *gt_);
			}
			--gt_;
		}
		else {
			// Leave the value in the mid range.
			++i_;
		}
	}

	// Swap pivots into final slots. Insert the lt_ pivot value where the last
	// last less-than value is, if it exists.
	if(begin_ != --lt_) {
		hxswap(*begin_, *lt_);
	}
	// Swap the first greater-than value with the gt_ pivot value, if it exists.
	if(back_ != ++gt_) {
		hxswap(*back_, *gt_);
	}

	// Recurse on the three partitions. Do not re-sort the partition values. At
	// this time lt_ and gt_ point right at their pivot values and they are
	// being used where [begin, end) semantics are expected.
	sort_callback_(begin_,  lt_,  less_, depth_);
	sort_callback_(lt_ + 1, gt_,  less_, depth_);
	sort_callback_(gt_ + 1, end_, less_, depth_);
}

/// Implements the introsort algorithm which is a hybrid of quicksort, heapsort
/// and insertion sort. `hxsort` is implemented using `hxintro_sort_`.
/// `hxintro_sort_` calls itself recursively until it hits its depth limit.
/// - `begin` : Pointer to the first element in the range.
/// - `end` : Pointer to one past the last element in the range.
/// - `less` : Comparison functor.
/// - `depth` : Current recursion depth remaining.
template<typename T_, typename less_t_> hxattr_nonnull(1,2) hxattr_hot
void hxintro_sort_(T_* begin_, T_* end_, const less_t_& less_, int depth_) {
	hxassertmsg(begin_ <= end_, "range_error hxsort");

	if((end_ - begin_) <= hxpartition_sort_cutoff_) {
		hxinsertion_sort(begin_, end_, less_);
	} else if(depth_ == 0u) {
		hxheapsort(begin_, end_, less_);
	} else {
		// Have the partition sort call back to hxsort for each sub-partition.
		hxpartition_sort_(begin_, end_, less_, hxintro_sort_<T_, less_t_>, depth_ - 1);
	}
}

} // namespace hxdetail_ {
using namespace hxdetail_;
