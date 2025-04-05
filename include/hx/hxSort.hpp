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
	// Arithmetic left shift i >> 31 can be implemented with a negative unsigned
	// left shift: -(u >> 31). 
	HX_STATIC_ASSERT((int32_t)0x80000000u >> 31 == ~(int32_t)0, "arithmetic left shift expected");

	HX_INLINE void reserve(uint32_t sz_) { m_array.reserve(sz_); }
	HX_INLINE void clear() { m_array.clear(); }
	void sort(hxMemoryManagerId tempMemory_);

protected:
	struct KeyValuePair {
		HX_INLINE KeyValuePair(uint8_t key_, void* val_) : m_key(key_), m_val(val_) { }
		HX_INLINE KeyValuePair(uint16_t key_, void* val_) : m_key(key_), m_val(val_) { }
		HX_INLINE KeyValuePair(uint32_t key_, void* val_) : m_key(key_), m_val(val_) { }
		// key_ + INT_MIN. (same as key_ ^ INT_MIN without room to carry.)
		HX_INLINE KeyValuePair(int32_t key_, void* val_) : m_val(val_) { m_key = (uint32_t)(key_ ^ 0x80000000); }
		// Flip all the bits if the sign bit is set, flip only the sign otherwise.
		HX_INLINE KeyValuePair(float key_, void* val_) : m_val(val_) {
			// Support strict aliasing.  And expect memcpy to inline fully.  
			int32_t t_;
			::memcpy(&t_, &key_, 4);
			m_key = (uint32_t)(t_ ^ ((t_ >> 31) | 0x80000000));
		}

		HX_INLINE bool operator<(const KeyValuePair& rhs_) const { return m_key < rhs_.m_key; }

		uint32_t m_key;
		void* m_val;
	};

	hxArray<KeyValuePair> m_array;
};


// ----------------------------------------------------------------------------
// hxRadixSort.  Sorts an array of value* by keys.  K is the key and V the value.
//
// Nota bene: Keys of double, int64_t and uint64_t are not supported.  Keys
// are stored as uint32_t.

template<typename K_, class V_>
class hxRadixSort : public hxRadixSortBase {
public:
	typedef K_ Key;
	typedef V_ Value;

	// ForwardIterator over Vales.  Not currently bound to std::iterator_traits
	// or std::forward_iterator_tag. 
	class constIterator
	{
	public:
		HX_INLINE constIterator(hxArray<KeyValuePair>::constIterator it_) : m_ptr(it_) { }
		HX_INLINE constIterator() : m_ptr(hxnull) { } // invalid
		HX_INLINE constIterator& operator++() { ++m_ptr; return *this; }
		HX_INLINE constIterator operator++(int) { constIterator t(*this); operator++(); return t; }
		HX_INLINE bool operator==(const constIterator& rhs_) const { return m_ptr == rhs_.m_ptr; }
		HX_INLINE bool operator!=(const constIterator& rhs_) const { return m_ptr != rhs_.m_ptr; }
		HX_INLINE const Value& operator*() const { return *(const Value*)m_ptr->m_val; }
		HX_INLINE const Value* operator->() const { return (const Value*)m_ptr->m_val; }

	protected:
		hxArray<KeyValuePair>::constIterator m_ptr;
	};

	class iterator : public constIterator
	{
	public:
		HX_INLINE iterator(hxArray<KeyValuePair>::iterator it_) : constIterator(it_) { }
		HX_INLINE iterator() { }
		HX_INLINE iterator& operator++() { constIterator::operator++(); return *this; }
		HX_INLINE iterator operator++(int) { iterator t_(*this); constIterator::operator++(); return t_; }
		HX_INLINE Value& operator*() const { return *(Value*)this->m_ptr->m_val; }
		HX_INLINE Value* operator->() const { return (Value*)this->m_ptr->m_val; }
	};

	HX_INLINE const Value& operator[](uint32_t index_) const { return *(Value*)m_array[index_].m_val; }
	HX_INLINE       Value& operator[](uint32_t index_) { return *(Value*)m_array[index_].m_val; }

	HX_INLINE const Value* get(uint32_t index_) const { return (Value*)m_array[index_].m_val; }
	HX_INLINE       Value* get(uint32_t index_) { return (Value*)m_array[index_].m_val; }

	HX_INLINE constIterator begin() const { return constIterator(m_array.cBegin()); }
	HX_INLINE iterator begin() { return iterator(m_array.begin()); }
	HX_INLINE constIterator cBegin() const { return constIterator(m_array.cBegin()); }
	HX_INLINE constIterator cBegin() { return constIterator(m_array.cBegin()); }

	HX_INLINE constIterator end() const { return constIterator(m_array.cEnd()); }
	HX_INLINE iterator end() { return iterator(m_array.end()); }
	HX_INLINE constIterator cEnd() const { return constIterator(m_array.cEnd()); }
	HX_INLINE constIterator cEnd() { return constIterator(m_array.cEnd()); }

	HX_INLINE uint32_t size() const { return m_array.size(); }
	HX_INLINE bool empty() const { return m_array.empty(); }

	// Adds a key and value pointer.
	HX_INLINE void insert(Key key_, Value* val_) {
		// Perform casts safe for -Wcast-qual with a const and volatile Value type.
		::new(m_array.emplaceBackUnconstructed()) KeyValuePair(key_, const_cast<void*>((const volatile void*)val_));
	}
};
