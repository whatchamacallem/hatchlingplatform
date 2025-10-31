#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxalgorithm.hpp Sorting, searching and set utilities for the
/// Hatchling Platform. Provides insertion sort, binary search, and a
/// general-purpose sort implementation. Includes support for functors when
/// defining custom key operations. Otherwise `T::T(T&&)`, `T::~T()`,
/// `T::operator=(T&&)`, `T::operator<(const T&)`, and `T::operator==(const T&)`
/// are used.
///
/// `hxradix_sort.hpp` is recommended as an `Θ(n)` sorting strategy for any
/// fundamental type that is four bytes or less. This implementation does not
/// cause code bloat and is the fastest available algorithm for scalar keys.
/// Radix sort is best when you need real-time guarantees and have a large
/// workload. IBM even used it to sort punch cards.
///
/// `hxinsertion_sort` is recommended when you have fewer than a kilobyte of
/// data to sort and you do not want to add 10 KB to your executable just for
/// sorting. `hxheap_sort` may also help keep code size down while providing
/// `Θ(n log n)` behavior.
///
/// `hxsort` is meant to be competitive with smaller types and "resistant to
/// attack." It instantiates about 200 lines of template code.
///
/// If sorting is important to your application, the "cpp-sort" project is
/// recommended as a way to study your data and identify the best algorithms for
/// you: https://github.com/Morwenn/cpp-sort.
///
/// If you ever need a specific algorithm that is not here, take a look at
/// `hxintro_sort_` as an example of how you might compose a new sorting
/// function from these routines.

#include "detail/hxalgorithm_detail.hpp"

/// `hxinsertion_sort` - Sorts the elements in the range `[begin, end)` in
/// comparison order using the insertion sort algorithm. The `end` parameter
/// points just past the end of the array. Exception handling during operation
/// is undefined. Declare your copy constructor and assignment operator
/// `noexcept` or turn off exceptions. All of `T::T(&&)`, `T::~T()` and
/// `T::operator=(T&&)` are used.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor defining a less-than ordering relationship.
template<typename iterator_t_, typename less_t_> hxattr_hot
void hxinsertion_sort(iterator_t_ begin_, iterator_t_ end_, const less_t_& less_) {
	hxassertmsg(begin_ <= end_, "invalid_iterator");

	// Address sanitizer: Avoids adding 1 to null iterators.
	if(begin_ == end_) { return; }

	// This pointer is the sole owner of the output array.
	const hxrestrict_t<iterator_t_> begin_r_(begin_);
	for(iterator_t_ i_ = begin_r_, j_ = begin_r_ + ptrdiff_t{1}; j_ < end_; i_ = j_, ++j_) {
		if(!less_(*i_, *j_)) {
			// Default value construct. Use hxmove instead of hxswap because it
			// should be more efficient for simple types. Complex types will
			// require an T::operator=(T&&) to be efficient.
			auto t_ = hxmove(*j_);
			*j_ = hxmove(*i_);
			while(begin_r_ < i_ && !less_(*(i_ - ptrdiff_t{1}), t_)) {
				*i_ = hxmove(*(i_ - ptrdiff_t{1}));
				--i_;
			}
			*i_ = hxmove(t_);
		}
	}
}

/// `hxinsertion_sort` (specialization) - An overload of `hxinsertion_sort` that
/// uses `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename iterator_t_> hxattr_hot
void hxinsertion_sort(iterator_t_ begin_, iterator_t_ end_) {
	hxinsertion_sort<iterator_t_>(begin_, end_, hxkey_less_function<decltype(*begin_)>());
}

/// `hxheapsort` - Sorts the elements in the range `[begin, end)` in comparison
/// order using the heapsort algorithm.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor defining a less-than ordering relationship.
template<typename iterator_t_, typename less_t_> hxattr_hot
void hxheapsort(iterator_t_ begin_, iterator_t_ end_, const less_t_& less_) {

	// This is std::make_heap with __restrict added to pointers.
	hxmake_heap_<iterator_t_>(begin_, end_, less_);

	// Swaps the largest values to the end of the array. These two implement
	// std::pop_heap with __restrict added as well.
	for(iterator_t_ it_ = end_ - ptrdiff_t{1}; it_ > begin_; --it_) {
		hxswap(*begin_, *it_);
		hxheapsort_heapify_<iterator_t_>(begin_, begin_, it_, less_);
	}
}

/// `hxheapsort` (specialization) - An overload of `hxheapsort` that uses
/// `hxkey_less`.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename iterator_t_> hxattr_hot
void hxheapsort(iterator_t_ begin_, iterator_t_ end_) {
	hxheapsort<iterator_t_>(begin_, end_, hxkey_less_function<decltype(*begin_)>());
}

/// `hxsort` - A general purpose sort routine using `T::T()`, `T::~T()`,
/// `T::operator=(&&)`, the `hxswap` overloads and a `less` functor which
/// defaults to `hxkey_less`. This version is intended for sorting large numbers
/// of small objects.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
/// - `less` : A key comparison functor defining a less-than ordering relationship.
template<typename iterator_t_, typename less_t_> hxattr_hot
void hxsort(iterator_t_ begin_, iterator_t_ end_, const less_t_& less_) {
	hxintro_sort_<iterator_t_>(begin_, end_, less_, 2 * hxlog2i(static_cast<size_t>(end_ - begin_)));
}

/// `hxsort` (specialization) - An overload of `hxsort` that uses `hxkey_less`.
/// This version is intended for sorting large numbers of small objects.
/// - `begin` : Pointer to the beginning of the range to sort.
/// - `end` : Pointer to one past the last element in the range to sort.
template<typename iterator_t_> hxattr_hot
void hxsort(iterator_t_ begin_, iterator_t_ end_) {
	hxintro_sort_<iterator_t_>(begin_, end_, hxkey_less_function<decltype(*begin_)>(),
		2 * hxlog2i(static_cast<size_t>(end_ - begin_)));
}

/// `hxmerge` - Performs a stable merge sort of two ordered ranges `[begin0,
/// end0)` and `[begin1, end1)` -> `output`. The input arrays must not overlap
/// the destination array. Passing a hxarray as an output iterator like this
/// `hxmerge<const int*, hxarray<int>&>(...)` will append to the array.
///
/// Assumes both `[begin0, end0)` and `[begin1, end1)` are ordered by the `less`
/// functor.
/// - `begin0` : Pointer to the beginning of the first ordered range to merge.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range to merge.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the merged output.
/// - `less` : Comparator defining the less-than ordering relationship.
template<typename iterator_t_, typename output_iterator_t_, typename less_t_> hxattr_hot
void hxmerge(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_, iterator_t_ end1_,
		output_iterator_t_&& output_, const less_t_& less_) {
	hxassertmsg(begin0_ <= end0_ && begin1_ <= end1_, "invalid_iterator");
	hxrestrict_t<output_iterator_t_> output_r_(output_);
	while(begin0_ != end0_ && begin1_ != end1_) {
		if(less_(*begin1_, *begin0_)) {
			*output_r_ = hxmove(*begin1_);
			++output_r_; ++begin1_;
		}
		else {
			*output_r_ = hxmove(*begin0_);
			++output_r_; ++begin0_;
		}
	}
	while(begin0_ != end0_) {
		*output_r_ = hxmove(*begin0_);
		++output_r_; ++begin0_;
	}
	while(begin1_ != end1_) {
		*output_r_ = hxmove(*begin1_);
		++output_r_; ++begin1_;
	}
}

/// `hxmerge` (specialization) - Performs a stable merge sort of two ordered
/// ranges `[begin0, end0)` and `[begin1, end1)` -> `output`. The input arrays
/// must not overlap the destination array. Assumes both `[begin0, end0)` and
/// `[begin1, end1)` are ordered by `hxless(a,b)`.  Passing a hxarray as an
/// output iterator like this `hxmerge<const int*, hxarray<int>&>(...)` will
/// append to the array.
/// - `begin0` : Pointer to the beginning of the first ordered range to merge.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range to merge.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the merged output.
template<typename iterator_t_, typename output_iterator_t_> hxattr_hot
void hxmerge(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_) {
	hxmerge<iterator_t_, output_iterator_t_>(begin0_, end0_, begin1_, end1_,
		hxforward<output_iterator_t_>(output_), hxkey_less_function<decltype(*begin0_)>());
}

/// `hxset_union` - Forms the union of two ordered ranges `[begin0, end0)` and
/// `[begin1, end1)` into `output`. Duplicate keys appear once in the output.
/// The input arrays must not overlap the destination array. Passing a hxarray
/// as an output iterator like this `hxset_union<const int*,
/// hxarray<int>&>(...)` will append to the array.
///
/// Assumes both ranges are ordered by the `less` functor.
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the union.
/// - `less` : Comparator defining the less-than ordering relationship.
/// Returns a pointer to one past the last element written.
template<typename iterator_t_, typename output_iterator_t_, typename less_t_> hxattr_hot
output_iterator_t_ hxset_union(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_, const less_t_& less_) {
	hxassertmsg(begin0_ <= end0_ && begin1_ <= end1_, "invalid_iterator");
	hxrestrict_t<output_iterator_t_> output_r_(output_);

	while(begin0_ != end0_ && begin1_ != end1_) {
		if(less_(*begin1_, *begin0_)) {
			*output_r_ = hxmove(*begin1_);
			++output_r_; ++begin1_;
		}
		else if(less_(*begin0_, *begin1_)) {
			*output_r_ = hxmove(*begin0_);
			++output_r_; ++begin0_;
		}
		else {
			*output_r_ = hxmove(*begin0_);
			++output_r_; ++begin0_; ++begin1_;
		}
	}

	while(begin0_ != end0_) {
		*output_r_ = hxmove(*begin0_);
		++output_r_; ++begin0_;
	}
	while(begin1_ != end1_) {
		*output_r_ = hxmove(*begin1_);
		++output_r_; ++begin1_;
	}
	return output_r_;
}

/// `hxset_union` (specialization) - Forms the union of two ordered ranges
/// `[begin0, end0)` and `[begin1, end1)` into `output` using `hxless`.
/// Duplicate keys appear once in the output. The input arrays must not overlap
/// the destination array. Passing a hxarray as an output iterator like this
/// `hxset_union<const int*, hxarray<int>&>(...)` will append to the array.
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the union.
/// Returns an output iterator positioned one past the last element written.
template<typename iterator_t_, typename output_iterator_t_> hxattr_hot
output_iterator_t_ hxset_union(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_) {
	return hxset_union<iterator_t_, output_iterator_t_>(begin0_, end0_, begin1_, end1_,
		hxforward<output_iterator_t_>(output_), hxkey_less_function<decltype(*begin0_)>());
}

/// `hxset_intersection` - Forms the intersection of two ordered ranges
/// `[begin0, end0)` and `[begin1, end1)` into `output`. Only keys present in
/// both ranges appear in the output. The input arrays must not overlap the
/// destination array. Passing a hxarray as an output iterator like this
/// `hxset_intersection<const int*, hxarray<int>&>(...)` will append to the array.
///
/// Assumes both ranges are ordered by the `less` functor.
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the intersection.
/// - `less` : Comparator defining the less-than ordering relationship.
/// Returns a pointer to one past the last element written.
template<typename iterator_t_, typename output_iterator_t_, typename less_t_> hxattr_hot
output_iterator_t_ hxset_intersection(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_, const less_t_& less_) {
	hxassertmsg(begin0_ <= end0_ && begin1_ <= end1_, "invalid_iterator");
	hxrestrict_t<output_iterator_t_> output_r_(output_);

	while(begin0_ != end0_ && begin1_ != end1_) {
		if(less_(*begin0_, *begin1_)) {
			++begin0_;
		}
		else if(less_(*begin1_, *begin0_)) {
			++begin1_;
		}
		else {
			*output_r_ = hxmove(*begin0_);
			++output_r_; ++begin0_; ++begin1_;
		}
	}
	return output_r_;
}

/// `hxset_intersection` (specialization) - Forms the intersection of two
/// ordered ranges `[begin0, end0)` and `[begin1, end1)` into `output` using
/// `hxless`. Only keys present in both ranges appear in the output. The input
/// arrays must not overlap the destination array. Passing a hxarray as an
/// output iterator like this `hxset_intersection<const int*,
/// hxarray<int>&>(...)` will append to the array.
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the intersection.
/// Returns an output iterator positioned one past the last element written.
template<typename iterator_t_, typename output_iterator_t_> hxattr_hot
output_iterator_t_ hxset_intersection(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_) {
	return hxset_intersection<iterator_t_, output_iterator_t_>(begin0_, end0_, begin1_, end1_,
		hxforward<output_iterator_t_>(output_), hxkey_less_function<decltype(*begin0_)>());
}

/// `hxset_difference` - Forms the difference of two ordered ranges
/// `[begin0, end0)` and `[begin1, end1)` into `output`. The output contains keys
/// that appear in the first range but not the second. The input arrays must
/// not overlap the destination array. Passing a hxarray as an output iterator
/// like this `hxset_difference<const int*, hxarray<int>&>(...)` will append
/// to the array.
///
/// Assumes both ranges are ordered by the `less` functor.
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the difference.
/// - `less` : Comparator defining the less-than ordering relationship.
/// Returns a pointer to one past the last element written.
template<typename iterator_t_, typename output_iterator_t_, typename less_t_> hxattr_hot
output_iterator_t_ hxset_difference(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_, const less_t_& less_) {
	hxassertmsg(begin0_ <= end0_ && begin1_ <= end1_, "invalid_iterator");
	hxrestrict_t<output_iterator_t_> output_r_(output_);

	while(begin0_ != end0_ && begin1_ != end1_) {
		if(less_(*begin0_, *begin1_)) {
			*output_r_ = hxmove(*begin0_);
			++output_r_; ++begin0_;
		}
		else if(less_(*begin1_, *begin0_)) {
			++begin1_;
		}
		else {
			++begin0_; ++begin1_;
		}
	}
	while(begin0_ != end0_) {
		*output_r_ = hxmove(*begin0_);
		++output_r_; ++begin0_;
	}
	return output_r_;
}

/// `hxset_difference` (specialization) - Forms the difference of two ordered
/// ranges `[begin0, end0)` and `[begin1, end1)` into `output` using `hxless`.
/// The output contains keys that appear in the first range but not the second.
/// The input arrays must not overlap the destination array. Passing a hxarray
/// as an output iterator like this `hxset_difference<const int*,
/// hxarray<int>&>(...)` will append to the array.
///
/// - `begin0` : Pointer to the beginning of the first ordered range.
/// - `end0` : Pointer to one past the last element of the first ordered range.
/// - `begin1` : Pointer to the beginning of the second ordered range.
/// - `end1` : Pointer to one past the last element of the second ordered range.
/// - `output` : Destination output iterator receiving the difference.
/// Returns an output iterator positioned one past the last element written.
template<typename iterator_t_, typename output_iterator_t_> hxattr_hot
output_iterator_t_ hxset_difference(iterator_t_ begin0_, iterator_t_ end0_, iterator_t_ begin1_,
		iterator_t_ end1_, output_iterator_t_&& output_) {
	return hxset_difference<iterator_t_, output_iterator_t_>(begin0_, end0_, begin1_, end1_,
		hxforward<output_iterator_t_>(output_), hxkey_less_function<decltype(*begin0_)>());
}

/// `hxbinary_search` - Performs a binary search in the range `[first, last)`.
/// Returns `end` if the value is not found. Unsorted data will lead to errors.
/// Non-unique values will be selected arbitrarily. The comparator parameter is
/// a functor that returns true if the first argument is ordered before (i.e.,
/// is less than) the second. The return value is non-standard. Passing a
/// hxarray as an output iterator like this `hxset_difference<const int*,
/// hxarray<int>&>(...)` will append to the array.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `value` : The value to search for.
/// - `less` : A key comparison functor defining a less-than ordering relationship. (Optional.)
template<typename iterator_t_, typename value_t_, typename less_t_> hxattr_hot
iterator_t_ hxbinary_search(iterator_t_ begin_, iterator_t_ end_, const value_t_& value_, const less_t_& less_) {
	// don't operate on null pointer args. unallocated containers have this.
	if(begin_ == end_) { return end_; }
	hxassertmsg(begin_ < end_, "invalid_iterator");
	hxattr_assume(begin_ < end_);

	iterator_t_ first_ = begin_;
	iterator_t_ last_ = end_;
	while(first_ < last_) {
		iterator_t_ mid_ = first_ + ((last_ - first_) >> 1);
		if(less_(*mid_, value_)) {
			first_ = mid_ + ptrdiff_t{1};
		}
		else if(less_(value_, *mid_)) {
			last_ = mid_;
		}
		else {
			return mid_;
		}
	}
	return end_;
}

/// `hxbinary_search` (specialization) - An overload of `hxbinary_search` that
/// uses `hxkey_less`.  Passing a hxarray as an output iterator like this
/// `hxset_difference<const int*, hxarray<int>&>(...)` will append to the array.
/// - `begin` : Pointer to the beginning of the range to search.
/// - `end` : Pointer to one past the last element in the range to search.
/// - `value` : The value to search for.
template<typename iterator_t_, typename value_t_> hxattr_hot
iterator_t_ hxbinary_search(iterator_t_ begin_, iterator_t_ end_, const value_t_& value_) {
	return hxbinary_search<iterator_t_>(begin_, end_, value_,
		hxkey_less_function<decltype(*begin_)>());
}
