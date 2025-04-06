#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxArray.hpp>

// ----------------------------------------------------------------------------
// hxLess
//
// Templated parameter type for performing comparisons.  Invokes operator <.
// This is C++14's std::less<void>.

struct hxLess {
	template<typename T1_, typename T2_>
	HX_INLINE bool operator()(const T1_& lhs_, const T2_& rhs_) const { return lhs_ < rhs_; }
};

// ----------------------------------------------------------------------------
// hxInsertionSort
//
// Sorts the elements in the range [first, last) in comparison order using
// the insertion sort algorithm.  The last parameter points past the end of
// the array.
//
// The compare parameter is a function object that returns true if the first
// argument is ordered before (i.e. is less than) the second.  See hxLess.
// Parameters:
// - begin_: Pointer to the beginning of the range to sort.
// - end_: Pointer to one past the last element in the range to sort.
// - compare_: Comparison function object.
template<typename T_, typename Compare>
HX_INLINE void hxInsertionSort(T_* begin_, T_* end_, const Compare& compare_) {
	if(begin_ == end_) { return; } // don't add +1 to null.

	// i points to value being inserted. j points to next unsorted value.
	for (T_ *i_ = begin_, *j_ = begin_ + 1; j_ < end_; i_ = j_++) {
		if (!compare_(*i_, *j_)) {
			T_ t_ = *j_;
			*j_ = *i_;
			while (begin_ < i_) {
				T_* k_ = i_ - 1;
				if (!compare_(*k_, t_)) {
					*i_ = *k_;
					i_ = k_;
				}
				else {
					break;
				}
			}
			*i_ = t_;
		}
	}
}

// ----------------------------------------------------------------------------
// A specialization of hxInsertionSort using hxLess.
// Parameters:
// - begin_: Pointer to the beginning of the range to sort.
// - end_: Pointer to one past the last element in the range to sort.
template<typename T_>
HX_INLINE void hxInsertionSort(T_* begin_, T_* end_) {
	hxInsertionSort(begin_, end_, hxLess());
}

// ----------------------------------------------------------------------------
// hxBinarySearch
//
// Performs a binary search in the range [first, last). Returns HX_NULL if the
// value is not found. Unsorted data will lead to errors. Non-unique values
// will be selected between arbitrarily.
//
// The compare parameter is a function object that returns true if the first
// argument is ordered before (i.e. is less than) the second.  See hxLess.
// Parameters:
// - begin_: Pointer to the beginning of the range to search.
// - end_: Pointer to one past the last element in the range to search.
// - val_: The value to search for.
// - compare_: Comparison function object.
template<typename T_, typename Compare>
HX_INLINE T_* hxBinarySearch(T_* begin_, T_* end_, const T_& val_, const Compare& compare_) {
	if(begin_ == end_) { return hxnull; } // don't operate on null.

	ptrdiff_t a_ = 0;
	ptrdiff_t b_ = end_ - begin_ - 1;
	
	while(a_ <= b_) {
		ptrdiff_t mid_ = a_ + (b_ - a_) / 2;
		if(!compare_(val_, begin_[mid_])) {
			if(!compare_(begin_[mid_], val_)) {
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

template<typename T_>
HX_INLINE T_* hxBinarySearch(T_* begin_, T_* end_, const T_& val_) {
	return hxBinarySearch(begin_, end_, val_, hxLess());
}

template<typename T_, typename Compare>
HX_INLINE const T_* hxBinarySearch(const T_* begin_, const T_* end_, const T_& val_, const Compare& compare_) {
	return hxBinarySearch(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_, compare_);
}

template<typename T_>
HX_INLINE const T_* hxBinarySearch(const T_* begin_, const T_* end_, const T_& val_) {
	return hxBinarySearch(const_cast<T_*>(begin_), const_cast<T_*>(end_), val_, hxLess());
}


// ----------------------------------------------------------------------------
// hxRadixSortBase.  Operations that are independent of hxRadixSort type.
//
// See hxRadixSort<K, V> below.

class hxRadixSortBase {
public:
	HX_STATIC_ASSERT((int32_t)0x80000000u >> 31 == ~(int32_t)0, "arithmetic left shift expected");

	// Reserves memory for the internal array to hold at least `sz_` elements.
	// Parameters:
	// - sz_: The number of elements to reserve memory for.
	HX_INLINE void reserve(uint32_t sz_) { m_array.reserve(sz_); }

	// Clears the internal array, removing all elements.
	HX_INLINE void clear() { m_array.clear(); }

	// Sorts the internal array using the provided temporary memory allocator to
	// store histograms.
	void sort(hxMemoryManagerId tempMemory_);

protected:
	// Represents a key-value pair used in radix sorting.
	struct KeyValuePair {
		// Constructor for an 8-bit key and associated value.
		HX_INLINE KeyValuePair(uint8_t key_, void* val_) : m_key(key_), m_val(val_) { }

		// Constructor for a 16-bit key and associated value.
		HX_INLINE KeyValuePair(uint16_t key_, void* val_) : m_key(key_), m_val(val_) { }

		// Constructor for a 32-bit key and associated value.
		HX_INLINE KeyValuePair(uint32_t key_, void* val_) : m_key(key_), m_val(val_) { }

		// Constructor for a signed 32-bit key and associated value.
		// Adjusts the key to handle signed integers correctly.
		HX_INLINE KeyValuePair(int32_t key_, void* val_) : m_val(val_) { m_key = (uint32_t)(key_ ^ 0x80000000); }

		// Constructor for a floating-point key and associated value.
		// Adjusts the key to handle floating-point sorting correctly.
		HX_INLINE KeyValuePair(float key_, void* val_) : m_val(val_) {
			int32_t t_;
			::memcpy(&t_, &key_, 4);
			m_key = (uint32_t)(t_ ^ ((t_ >> 31) | 0x80000000));
		}

		// Comparison operator for sorting KeyValuePair objects by key.
		HX_INLINE bool operator<(const KeyValuePair& rhs_) const { return m_key < rhs_.m_key; }

		uint32_t m_key; // The key used for sorting.
		void* m_val;	// The associated value.
	};

	hxArray<KeyValuePair> m_array; // Internal array of key-value pairs.
};

// ----------------------------------------------------------------------------
// hxRadixSort.  Sorts an array of value* by keys.  K is the key and V the value.
//
// Nota bene: Keys of double, int64_t and uint64_t are not supported.  Keys
// are stored as uint32_t.

template<typename K_, class V_>
class hxRadixSort : public hxRadixSortBase {
public:
	typedef K_ Key;   // Type of the key used for sorting.
	typedef V_ Value; // Type of the value associated with the key.

	// ForwardIterator over Values. Not currently bound to std::iterator_traits
	// or std::forward_iterator_tag.
	class constIterator {
	public:
		// Constructs a constIterator from an hxArray<KeyValuePair>::constIterator.
		HX_INLINE constIterator(hxArray<KeyValuePair>::constIterator it_) : m_ptr(it_) { }

		// Constructs an invalid constIterator.
		HX_INLINE constIterator() : m_ptr(hxnull) { }

		// Pre-increment operator. Moves the iterator to the next element.
		HX_INLINE constIterator& operator++() { ++m_ptr; return *this; }

		// Post-increment operator. Moves the iterator to the next element and returns the previous state.
		HX_INLINE constIterator operator++(int) { constIterator t(*this); operator++(); return t; }

		// Equality comparison operator.
		HX_INLINE bool operator==(const constIterator& rhs_) const { return m_ptr == rhs_.m_ptr; }

		// Inequality comparison operator.
		HX_INLINE bool operator!=(const constIterator& rhs_) const { return m_ptr != rhs_.m_ptr; }

		// Dereference operator. Returns a reference to the value pointed to by the iterator.
		HX_INLINE const Value& operator*() const { return *(const Value*)m_ptr->m_val; }

		// Arrow operator. Returns a pointer to the value pointed to by the iterator.
		HX_INLINE const Value* operator->() const { return (const Value*)m_ptr->m_val; }

	protected:
		hxArray<KeyValuePair>::constIterator m_ptr; // Internal pointer to the current element.
	};

	// Iterator that can be cast to a constIterator.
	class iterator : public constIterator {
	public:
		// Constructs an iterator from an hxArray<KeyValuePair>::iterator.
		HX_INLINE iterator(hxArray<KeyValuePair>::iterator it_) : constIterator(it_) { }

		// Constructs an invalid iterator.
		HX_INLINE iterator() { }

		// Pre-increment operator. Moves the iterator to the next element.
		HX_INLINE iterator& operator++() { constIterator::operator++(); return *this; }

		// Post-increment operator. Moves the iterator to the next element and returns the previous state.
		HX_INLINE iterator operator++(int) { iterator t_(*this); constIterator::operator++(); return t_; }

		// Dereference operator. Returns a reference to the value pointed to by the iterator.
		HX_INLINE Value& operator*() const { return *(Value*)this->m_ptr->m_val; }

		// Arrow operator. Returns a pointer to the value pointed to by the iterator.
		HX_INLINE Value* operator->() const { return (Value*)this->m_ptr->m_val; }
	};

	// Accesses the value at the specified index (const version).
	HX_INLINE const Value& operator[](uint32_t index_) const { return *(Value*)m_array[index_].m_val; }

	// Accesses the value at the specified index (non-const version).
	HX_INLINE Value& operator[](uint32_t index_) { return *(Value*)m_array[index_].m_val; }

	// Returns a pointer to the value at the specified index (const version).
	HX_INLINE const Value* get(uint32_t index_) const { return (Value*)m_array[index_].m_val; }

	// Returns a pointer to the value at the specified index (non-const version).
	HX_INLINE Value* get(uint32_t index_) { return (Value*)m_array[index_].m_val; }

	// Returns a constIterator to the beginning of the array.
	HX_INLINE constIterator begin() const { return constIterator(m_array.cBegin()); }

	// Returns an iterator to the beginning of the array.
	HX_INLINE iterator begin() { return iterator(m_array.begin()); }

	// Returns a constIterator to the beginning of the array (const version).
	HX_INLINE constIterator cBegin() const { return constIterator(m_array.cBegin()); }

	// Returns a constIterator to the beginning of the array (non-const version).
	HX_INLINE constIterator cBegin() { return constIterator(m_array.cBegin()); }

	// Returns a constIterator to the end of the array.
	HX_INLINE constIterator end() const { return constIterator(m_array.cEnd()); }

	// Returns an iterator to the end of the array.
	HX_INLINE iterator end() { return iterator(m_array.end()); }

	// Returns a constIterator to the end of the array (const version).
	HX_INLINE constIterator cEnd() const { return constIterator(m_array.cEnd()); }

	// Returns a constIterator to the end of the array (non-const version).
	HX_INLINE constIterator cEnd() { return constIterator(m_array.cEnd()); }

	// Returns the number of elements in the array.
	HX_INLINE uint32_t size() const { return m_array.size(); }

	// Returns true if the array is empty, false otherwise.
	HX_INLINE bool empty() const { return m_array.empty(); }

	// Adds a key and value pointer to the array.
	// Parameters:
	// - key_: The key used for sorting.
	// - val_: Pointer to the value associated with the key.
	HX_INLINE void insert(Key key_, Value* val_) {
		// Perform casts safe for -Wcast-qual with a const and volatile Value type.
		::new(m_array.emplaceBackUnconstructed()) KeyValuePair(key_, const_cast<void*>((const volatile void*)val_));
	}
};
