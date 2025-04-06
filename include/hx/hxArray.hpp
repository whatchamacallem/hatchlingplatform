#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hxAllocator.hpp>

// ----------------------------------------------------------------------------
// hxArray
//
// Implements some of std::vector.  Requires T to have a default constructor.

template<typename T_, size_t Capacity_=hxAllocatorDynamicCapacity>
class hxArray : private hxAllocator<T_, Capacity_> {
public:
	typedef T_ T; // value type
	typedef T* iterator; // Random access iterator.
	typedef const T* constIterator; // Const random access iterator.

	// Constructs an empty array with a capacity of Capacity.  m_end will be 0
	// if Capacity is 0.
	HX_INLINE explicit hxArray() { m_end = this->getStorage(); }

	// Copy constructs an array.  Does not allow movement of hxUniquePtrs.  Use
	// assign() for that.
	HX_INLINE explicit hxArray(const hxArray& rhs_) : hxAllocator<T, Capacity_>() {
		m_end = this->getStorage();
		assign(rhs_.cBegin(), rhs_.cEnd());
	}

	// Copy constructs an array from a container with begin() and end() methods and
	// a random access iterator.
	template <typename Rhs>
	HX_INLINE explicit hxArray(const Rhs& rhs_) : hxAllocator<T, Capacity_>() {
		m_end = this->getStorage();
		assign(rhs_.begin(), rhs_.end());
	}

	// Destructs the array and destroys all elements.
	HX_INLINE ~hxArray() {
		destruct_(this->getStorage(), m_end);
	}

	// Assigns the contents of another hxArray to this array.
	// Standard except reallocation is disallowed.
	HX_INLINE void operator=(const hxArray& rhs_) {
		assign(rhs_.begin(), rhs_.end());
	}

	// Copies the elements of a container with begin() and end() methods and a random
	// access iterator.
	template <typename Rhs>
	HX_INLINE void operator=(const Rhs& rhs_) {
		assign(rhs_.begin(), rhs_.end());
	}

	// Returns a const iterator to the beginning of the array.
	HX_INLINE const T* begin() const { return this->getStorage(); }

	// Returns an iterator to the beginning of the array.
	HX_INLINE T* begin() { return this->getStorage(); }

	// Returns a const iterator to the beginning of the array (alias for begin()).
	HX_INLINE const T* cBegin() const { return this->getStorage(); }

	// Returns a const iterator to the beginning of the array (alias for begin()).
	HX_INLINE const T* cBegin() { return this->getStorage(); }

	// Returns a const iterator to the end of the array.
	HX_INLINE const T* end() const { return m_end; }

	// Returns an iterator to the end of the array.
	HX_INLINE T* end() { return m_end; }

	// Returns a const iterator to the end of the array (alias for end()).
	HX_INLINE const T* cEnd() const { return m_end; }

	// Returns a const iterator to the end of the array (alias for end()).
	HX_INLINE const T* cEnd() { return m_end; }

	// Returns a const reference to the first element in the array.
	HX_INLINE const T& front() const { hxAssert(size()); return *this->getStorage(); }

	// Returns a reference to the first element in the array.
	HX_INLINE T& front() { hxAssert(size()); return *this->getStorage(); }

	// Returns a const reference to the last element in the array.
	HX_INLINE const T& back() const { hxAssert(size()); return *(m_end - 1); }

	// Returns a reference to the last element in the array.
	HX_INLINE T& back() { hxAssert(size()); return *(m_end - 1); }

	// Returns a const reference to the element at the specified index.
	HX_INLINE const T& operator[](size_t index_) const {
		hxAssert(index_ < size());
		return this->getStorage()[index_];
	}

	// Returns a reference to the element at the specified index.
	HX_INLINE T& operator[](size_t index_) {
		hxAssert(index_ < size());
		return this->getStorage()[index_];
	}

	// Returns the number of elements in the array.
	HX_INLINE size_t size() const {
		hxAssert(!m_end == !this->getStorage());
		return (size_t)(m_end - this->getStorage());
	}

	// Reserves storage for at least the specified number of elements.
	HX_INLINE void reserve(size_t size_) {
		T* prev = this->getStorage();
		this->reserveStorage(size_);
		hxAssertMsg(!prev || prev == this->getStorage(), "no reallocation"); (void)prev;
		if (m_end == hxnull) {
			m_end = this->getStorage();
		}
	}

	// Returns the capacity of the array.
	HX_INLINE size_t capacity() const { return this->getCapacity(); }

	// Clears the array, destroying all elements.
	HX_INLINE void clear() {
		destruct_(this->getStorage(), m_end);
		m_end = this->getStorage();
	}

	// Returns true if the array is empty.
	HX_INLINE bool empty() const { return m_end == this->getStorage(); }

	// Resizes the array to the specified size, constructing or destroying elements as needed.
	HX_INLINE void resize(size_t size_) {
		reserve(size_);
		if (size_ >= size()) {
			this->construct_(m_end, this->getStorage() + size_);
		}
		else {
			destruct_(this->getStorage() + size_, m_end);
		}
		m_end = this->getStorage() + size_;
	}

	// Adds a copy of the specified element to the end of the array.
	HX_INLINE void pushBack(const T& t_) {
		hxAssert(size() < capacity());
		::new (m_end++) T(t_);
	}

	// Removes the last element from the array.
	HX_INLINE void popBack() {
		hxAssert(size());
		(--m_end)->~T();
	}

	// Returns a const pointer to the array's data.
	HX_INLINE const T* data() const { return this->getStorage(); }

	// Returns a pointer to the array's data.
	HX_INLINE T* data() { return this->getStorage(); }

	// Assigns elements from a range defined by iterators to the array.
	template <typename Iter>
	HX_INLINE void assign(Iter first_, Iter last_) {
		reserve((size_t)(last_ - first_));
		T* it_ = this->getStorage();
		destruct_(it_, m_end);
		while (first_ != last_) { ::new (it_++) T(*first_++); }
		m_end = it_;
	}

	// --------------------------------------------------------------------------
	// Non-standard but useful

	// Constructs an array of T from an array of T2.
	template<typename T2_, size_t Sz_>
	HX_INLINE void assign(const T2_(&a_)[Sz_]) { assign(a_ + 0, a_ + Sz_); }

	// Variant of emplace_back() that returns a pointer for use with placement new.
	HX_INLINE void* emplaceBackUnconstructed() {
		hxAssert(size() < capacity());
		return (void*)m_end++;
	}

	// Variant of erase() that moves the end element down to replace erased element.
	HX_INLINE void eraseUnordered(size_t index_) {
		hxAssert(index_ < size());
		T* it_ = this->getStorage() + index_;
		if (it_ != --m_end) {
			*it_ = *m_end;
		}
		m_end->~T();
	}

	// Variant of erase() that moves the end element down to replace the erased element.
	HX_INLINE void eraseUnordered(T* it_) {
		hxAssert((size_t)(it_ - this->getStorage()) < size());
		if (it_ != --m_end) {
			*it_ = *m_end;
		}
		m_end->~T();
	}

	// Returns true when the array is full (size equals capacity).
	HX_INLINE bool full() {
		return size() == capacity();
	}

private:
	HX_INLINE void construct_(T* first_, T* last_) {
		while (first_ != last_) {
			::new (first_++) T;
		}
	}

	HX_INLINE void destruct_(T* first_, T* last_) {
		while (first_ != last_) {
			first_++->~T();
		}
	}
	T* m_end;
};
