#pragma once
/// Copyright 2017-2025 Adrian Johnston

#include <hx/hxkey.hpp>

template<typename T_, typename less_t_>
void hxinsertion_sort(T_* begin_, T_* end_, const less_t_& less_);

template<typename T_, typename less_t_>
void hxheapsort(T_* begin_, T_* end_, const less_t_& less_);

namespace hxdetail_ {

enum { hxpartition_sort_cutoff_ = 32 };

/// Restores the heap property by pushing a value down until it is not greater
/// than its children.
/// - `begin` : Pointer to the first element in the heap.
/// - `end` : Pointer to one past the last element in the heap.
/// - `current` : Pointer to the current element being heapified.
/// - `less` : Comparison functor.
template<typename T_, typename less_t_>
void hxheapsort_heapify_(T_* begin_, const T_* end_, T_* current_, const less_t_& less_) {
	for (;;) {
		hxassertmsg(begin_ <= current_ && current_ < end_, "invalid_iterator");

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
			// Hopefully this uses the processor status flags from the last
			// compare.
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

/// Heapify operation for parents (but not grandparents).
/// - `begin` : Pointer to the first element in the heap.
/// - `end` : Pointer to one past the last element in the heap.
/// - `current` : Pointer to the current element being heapified.
/// - `less` : Comparison functor.
template<typename T_, typename less_t_>
void hxheapsort_heapify_bottom_(T_* begin_, const T_* end_, T_* current_, const less_t_& less_) {
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
template<typename T_, typename less_t_, typename sort_callback_t_>
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
	// with a processor that has branch prediction. Pairs taken from
	// github.com/Morwenn/cpp-sort/.../sort5.h.
	if (less_(*p3_, *p0_)) { hxswap(p3_, p0_); }
	if (less_(*p4_, *p1_)) { hxswap(p4_, p1_); }
	if (less_(*p2_, *p0_)) { hxswap(p2_, p0_); }
	if (less_(*p3_, *p1_)) { hxswap(p3_, p1_); }
	if (less_(*p1_, *p0_)) { hxswap(p1_, p0_); }
	if (less_(*p4_, *p2_)) { hxswap(p4_, p2_); }
	if (less_(*p2_, *p1_)) { hxswap(p2_, p1_); }
	if (less_(*p4_, *p3_)) { hxswap(p4_, p3_); }
	if (less_(*p3_, *p2_)) { hxswap(p3_, p2_); }

    T_* back_ = end_ - 1; // Pointer to the last value.

	// Move the selected pivots out of the way by placing them at the ends of
	// the range.
    hxswap(*begin_, *p1_);
    hxswap(*back_, *p3_);

	// Three-way partition into [<p₁], [p₁ ≤ … ≤ p₂], [>p₂]

	// Points to end of less-than range, which is empty and is right after the
	// first pivot.
	T_* lt_ = begin_ + 1;
	// Points to beginning of greater-than range, which is empty and also where
	// the last pivot is stashed.
	T_* gt_ = back_;

	for (T_* i_ = lt_; i_ < gt_; ) {
		if (less_(*i_, *begin_)) {
			// Swap into less-than range and extend it.
			if(lt_ != i_) {
				hxswap(*i_, *lt_);
			}
			++i_; ++lt_;
		}
		else if (less_(*back_, *i_)) {
			// Swap into greater-than range and extend it.
			if(--gt_ != i_) {
				hxswap(*i_, *gt_);
			}
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
	if(back_ != gt_) {
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
/// and insertion sort. hxsort is implemented using `hxintro_sort_`.
/// `hxintro_sort_` calls itself recursively until it hits its depth limit.
/// - `begin` : Pointer to the first element in the range.
/// - `end` : Pointer to one past the last element in the range.
/// - `less` : Comparison functor.
/// - `depth` : Current recursion depth remaining.
template<typename T_, typename less_t_>
void hxintro_sort_(T_* begin_, T_* end_, const less_t_& less_, int depth_) {
	hxassertmsg(begin_ != hxnull && begin_ <= end_, "range_error hxsort");

	if ((end_ - begin_) <= hxpartition_sort_cutoff_) {
		hxinsertion_sort(begin_, end_, less_);
	} else if (depth_ == 0u) {
		hxheapsort(begin_, end_, less_);
	} else {
		// Have the partition sort call back to hxsort for each sub-partition.
		hxpartition_sort_(begin_, end_, less_, hxintro_sort_<T_, less_t_>, depth_ - 1u);
	}
}

} // namespace hxdetail_ {
using namespace hxdetail_;
