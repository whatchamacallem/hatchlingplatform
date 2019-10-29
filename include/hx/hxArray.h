#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxAllocator.h>

// ----------------------------------------------------------------------------
// hxArray
//
// Implements some of std::vector.  Requires T to have a default constructor.
// Undocumented functions follow std::vector.  Undocumented methods have a
// standard interface.

template<typename T_, uint32_t Capacity_=hxAllocatorDynamicCapacity>
class hxArray : private hxAllocator<T_, Capacity_> {
public:
	typedef T_ T;
	typedef T value_type;
	typedef uint32_t size_type; // 32-bit indices. 
	typedef T* iterator; // Random access iterator.
	typedef const T* const_iterator; // Const random access iterator.
	typedef hxAllocator<T, Capacity_> allocator_type; 

	// Constructs an empty array with a capacity of Capacity.  m_end will be 0
	// if Capacity is 0.
	HX_INLINE explicit hxArray() { m_end = this->getStorage(); }

	// Copy constructs an array.  Does not allow movement of hxUniquePtrs.  Use
	// assign() for that.
	HX_INLINE explicit hxArray(const hxArray& rhs_) : hxAllocator<T, Capacity_>() {
		m_end = this->getStorage();
		assign(rhs_.cbegin(), rhs_.cend());
	}

	// Copy constructs an array from a container with begin() and end() methods and
	// a random access iterator.
	template <typename Rhs>
	HX_INLINE explicit hxArray(const Rhs& rhs_) : hxAllocator<T, Capacity_>() {
		m_end = this->getStorage();
		assign(rhs_.begin(), rhs_.end());
	}

	// Destructs array.
	HX_INLINE ~hxArray() {
		destruct_(this->getStorage(), m_end);
	}

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

	// Standard interface.
	HX_INLINE const allocator_type& get_allocator() const { return *this; }
	HX_INLINE       allocator_type& get_allocator() { return *this; }

	HX_INLINE const T* begin() const { return this->getStorage(); }
	HX_INLINE       T* begin() { return this->getStorage(); }
	HX_INLINE const T* cbegin() const { return this->getStorage(); }

	HX_INLINE const T* end() const { return m_end; }
	HX_INLINE       T* end() { return m_end; }
	HX_INLINE const T* cend() const { return m_end; }

	HX_INLINE const T& front() const { hxAssert(size()); return *this->getStorage(); }
	HX_INLINE       T& front() { hxAssert(size()); return *this->getStorage(); }

	HX_INLINE const T& back() const { hxAssert(size()); return *(m_end - 1); }
	HX_INLINE       T& back() { hxAssert(size()); return *(m_end - 1); }

	HX_INLINE const T& operator[](uint32_t index_) const {
		hxAssert(index_ < size());
		return this->getStorage()[index_];
	}
	HX_INLINE       T& operator[](uint32_t index_) {
		hxAssert(index_ < size());
		return this->getStorage()[index_];
	}

	HX_INLINE uint32_t size() const {
		hxAssert(!m_end == !this->getStorage());
		return (uint32_t)(m_end - this->getStorage());
	}

	HX_INLINE void reserve(uint32_t size_) {
		T* prev = this->getStorage();
		this->reserveStorage(size_);
		hxAssertMsg(!prev || prev == this->getStorage(), "no reallocation"); (void)prev;
		if (m_end == hxnull) {
			m_end = this->getStorage();
		}
	}

	HX_INLINE uint32_t capacity() const { return this->getCapacity(); }

	HX_INLINE void clear() {
		destruct_(this->getStorage(), m_end);
		m_end = this->getStorage();
	}

	HX_INLINE bool empty() const { return m_end == this->getStorage(); }

	HX_INLINE void resize(uint32_t size_) {
		reserve(size_);
		if (size_ >= size()) {
			this->construct_(m_end, this->getStorage() + size_);
		}
		else {
			destruct_(this->getStorage() + size_, m_end);
		}
		m_end = this->getStorage() + size_;
	}

	HX_INLINE void push_back(const T& t_) {
		hxAssert(size() < capacity());
		::new (m_end++) T(t_);
	}

	HX_INLINE void pop_back() {
		hxAssert(size());
		(--m_end)->~T();
	}

	HX_INLINE const T* data() const { return this->getStorage(); }
	HX_INLINE       T* data() { return this->getStorage(); }

	// Copies the elements of a container with begin() and end() methods and a random
	// access iterator.
	template <typename Iter>
	HX_INLINE void assign(Iter first_, Iter last_) {
		reserve((uint32_t)(last_ - first_));
		T* it_ = this->getStorage();
		destruct_(it_, m_end);
		while (first_ != last_) { ::new (it_++) T(*first_++); }
		m_end = it_;
	}

	// --------------------------------------------------------------------------
	// Non-standard but useful

	// Constructs an array of T from an array of T2.
	template<typename T2_, size_t Sz_>
	HX_INLINE void assign(const T2_(&a_)[Sz_]) { assign(a_ + 0u, a_ + Sz_); }

	// Variant of emplace_back() that returns a pointer for use with placement new.
	HX_INLINE void* emplace_back_unconstructed() {
		hxAssert(size() < capacity());
		return (void*)m_end++;
	}

	// Variant of erase() that moves the end element down to replace erased
	// element.
	HX_INLINE void erase_unordered(uint32_t index_) {
		hxAssert(index_ < size());
		T* it_ = this->getStorage() + index_;
		if (it_ != --m_end) {
			*it_ = *m_end;
		}
		m_end->~T();
	}

	// Variant of erase() that moves the end element down to replace the erased
	// element.
	HX_INLINE void erase_unordered(T* it_) {
		hxAssert((uint32_t)(it_ - this->getStorage()) < size());
		if (it_ != --m_end) {
			*it_ = *m_end;
		}
		m_end->~T();
	}

	// Returns true when size equals capacity.
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
