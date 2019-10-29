#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxArray.h>

// ----------------------------------------------------------------------------
// hxLess
//
// Templated parameter type for performing comparisons.  Invokes operator <.
// This is C++14's std::less<void>.

struct hxLess {
	template<typename T1, typename T2>
	HX_INLINE bool operator()(const T1& lhs, const T2& rhs) const { return lhs < rhs; }
};

// ----------------------------------------------------------------------------
// hxInsertionSort
//
// Sorts the elements in the range [first, last) in comparison order using the
// insertion sort algorithm.
//
// The compare parameter is a function object that returns true if the first
// argument is ordered before (i.e. is less than) the second.  See hxLess.

template<typename T, typename Compare>
HX_INLINE void hxInsertionSort(T* first, T* last, const Compare& compare) {
	for (T* i = first + 1, *j = first; i < last; j = i++) {
		if (compare(*i, *j)) {
			T t = *i;
			*i = *j;
			while (j > first) {
				T* k = j - 1;
				if (compare(t, *k)) {
					*j = *k;
					j = k;
				}
				else { break; }
			}
			*j = t;
		}
	}
}

// ----------------------------------------------------------------------------
// A specialization of hxInsertionSort using hxLess.
template<typename T>
HX_INLINE void hxInsertionSort(T* first, T* last) {
	hxInsertionSort(first, last, hxLess());
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

	HX_INLINE void reserve(uint32_t sz) { m_array.reserve(sz); }
	HX_INLINE void clear() { m_array.clear(); }
	void sort(hxMemoryManagerId tempMemory);

protected:
	struct KeyValuePair {
		HX_INLINE KeyValuePair(uint8_t key, void* val) : m_key(key), m_val(val) { }
		HX_INLINE KeyValuePair(uint16_t key, void* val) : m_key(key), m_val(val) { }
		HX_INLINE KeyValuePair(uint32_t key, void* val) : m_key(key), m_val(val) { }
		// key + INT_MIN. (same as key ^ INT_MIN without room to carry.)
		HX_INLINE KeyValuePair(int32_t key, void* val) : m_val(val) { m_key = (uint32_t)(key ^ 0x80000000); }
		// Flip all the bits if the sign bit is set, flip only the sign otherwise.
		HX_INLINE KeyValuePair(float key, void* val) : m_val(val) {
			// Support strict aliasing.  And expect memcpy to inline fully.  
			int32_t t;
			::memcpy(&t, &key, 4);
			m_key = (uint32_t)(t ^ ((t >> 31) | 0x80000000));
		}

		HX_INLINE bool operator<(const KeyValuePair& rhs) const { return m_key < rhs.m_key; }

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

template<typename K, class V>
class hxRadixSort : public hxRadixSortBase {
public:
	typedef K Key;
	typedef V Value;

	// ForwardIterator over Vales.  Not currently bound to std::iterator_traits
	// or std::forward_iterator_tag. 
	class const_iterator
	{
	public:
		HX_INLINE const_iterator(hxArray<KeyValuePair>::const_iterator it) : m_ptr(it) { }
		HX_INLINE const_iterator() : m_ptr(hxnull) { } // invalid
		HX_INLINE const_iterator& operator++() { ++m_ptr; return *this; }
		HX_INLINE const_iterator operator++(int) { const_iterator t(*this); operator++(); return t; }
		HX_INLINE bool operator==(const const_iterator& rhs) const { return m_ptr == rhs.m_ptr; }
		HX_INLINE bool operator!=(const const_iterator& rhs) const { return m_ptr != rhs.m_ptr; }
		HX_INLINE const Value& operator*() const { return *(const Value*)m_ptr->m_val; }
		HX_INLINE const Value* operator->() const { return (const Value*)m_ptr->m_val; }

	protected:
		hxArray<KeyValuePair>::const_iterator m_ptr;
	};

	class iterator : public const_iterator
	{
	public:
		HX_INLINE iterator(hxArray<KeyValuePair>::iterator it) : const_iterator(it) { }
		HX_INLINE iterator() { }
		HX_INLINE iterator& operator++() { const_iterator::operator++(); return *this; }
		HX_INLINE iterator operator++(int) { iterator t(*this); const_iterator::operator++(); return t; }
		HX_INLINE Value& operator*() const { return *(Value*)this->m_ptr->m_val; }
		HX_INLINE Value* operator->() const { return (Value*)this->m_ptr->m_val; }
	};

	HX_INLINE const Value& operator[](uint32_t index) const { return *(Value*)m_array[index].m_val; }
	HX_INLINE       Value& operator[](uint32_t index) { return *(Value*)m_array[index].m_val; }

	HX_INLINE const Value* get(uint32_t index) const { return (Value*)m_array[index].m_val; }
	HX_INLINE       Value* get(uint32_t index) { return (Value*)m_array[index].m_val; }

	HX_INLINE const_iterator begin() const { return const_iterator(m_array.cbegin()); }
	HX_INLINE iterator begin() { return iterator(m_array.begin()); }
	HX_INLINE const_iterator cbegin() const { return const_iterator(m_array.cbegin()); }

	HX_INLINE const_iterator end() const { return const_iterator(m_array.cend()); }
	HX_INLINE iterator end() { return iterator(m_array.end()); }
	HX_INLINE const_iterator cend() const { return const_iterator(m_array.cend()); }

	HX_INLINE uint32_t size() const { return m_array.size(); }
	HX_INLINE bool empty() const { return m_array.empty(); }

	// Adds a key and value pointer.
	HX_INLINE void insert(Key key, Value* val) {
		// Perform casts safe for -Wcast-qual with a const and volatile Value type.
		::new(m_array.emplace_back_unconstructed()) KeyValuePair(key, const_cast<void*>((const volatile void*)val));
	}
};
