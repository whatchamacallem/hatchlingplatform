#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxAllocator.h"

// ----------------------------------------------------------------------------
// hxArray
//
// Implements some of std::vector.  Requires T to have a default constructor.

template<typename T, uint32_t Capacity=hxAllocatorDynamicCapacity>
class hxArray : private hxAllocator<T, Capacity> {
public:
	typedef T value_type;
	typedef uint32_t size_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef hxAllocator<T, Capacity> allocator_type;

	// m_end will be 0 if Capacity is 0.
	HX_INLINE explicit hxArray() { m_end = this->getStorage(); }

	// Does not allow movement of hxUniquePtrs.  Use assign() for that.
	HX_INLINE explicit hxArray(const hxArray& rhs) {
		m_end = this->getStorage();
		assign(rhs.cbegin(), rhs.cend());
	}

	template <typename Rhs>
	HX_INLINE explicit hxArray(const Rhs& rhs) {
		m_end = this->getStorage();
		assign(rhs.begin(), rhs.end());
	}

	HX_INLINE ~hxArray() {
		this->destruct(this->getStorage(), m_end);
	}

	HX_INLINE void operator=(const hxArray& rhs) {
		assign(rhs.begin(), rhs.end());
	}

	template <typename Rhs>
	HX_INLINE void operator=(const Rhs& rhs) {
		assign(rhs.begin(), rhs.end());
	}

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

	HX_INLINE const T& operator[](uint32_t index) const { hxAssert(index < size()); return this->getStorage()[index]; }
	HX_INLINE       T& operator[](uint32_t index) { hxAssert(index < size()); return this->getStorage()[index]; }

	HX_INLINE uint32_t size() const {
		hxAssert(!m_end == !this->getStorage());
		return (uint32_t)(m_end - this->getStorage());
	}

	HX_INLINE void reserve(uint32_t c) {
		T* prev = this->getStorage();
		this->reserveStorage(c);
		hxAssertMsg(!prev || prev == this->getStorage(), "no reallocation"); (void)prev;
		if (m_end == hx_null) {
			m_end = this->getStorage();
		}
	}

	HX_INLINE uint32_t capacity() const { return this->getCapacity(); }

	HX_INLINE void clear() {
		this->destruct(this->getStorage(), m_end);
		m_end = this->getStorage();
	}

	HX_INLINE bool empty() const { return m_end == this->getStorage(); }

	HX_INLINE void resize(uint32_t sz) {
		reserve(sz);
		if (sz >= size()) {
			this->construct(m_end, this->getStorage() + sz);
		}
		else {
			this->destruct(this->getStorage() + sz, m_end);
		}
		m_end = this->getStorage() + sz;
	}

	HX_INLINE void push_back(const T& t) {
		hxAssert(size() < capacity());
		::new (m_end++) T(t);
	}

	HX_INLINE void pop_back() {
		hxAssert(size());
		(--m_end)->~T();
	}

	// Will not cause allocation when begin==end.
	template <typename Iter>
	HX_INLINE void assign(Iter begin, Iter end) {
		reserve((uint32_t)(end - begin));
		T* it = this->getStorage();
		this->destruct(it, m_end);
		while (begin != end) { ::new (it++) T(*begin++); }
		m_end = it;
	}

	HX_INLINE const T* data() const { return this->getStorage(); }
	HX_INLINE       T* data() { return this->getStorage(); }

	// --------------------------------------------------------------------------
	// Non-standard but useful

	// Returns pointer for use with placement new.
	HX_INLINE void* emplace_back_unconstructed() {
		hxAssert(size() < capacity());
		return (void*)m_end++;
	}

	// Moves the end element down to replace erased element.
	HX_INLINE void erase_unordered(uint32_t index) {
		hxAssert(index < size());
		T* it = this->getStorage() + index;
		if (it != --m_end) {
			*it = *m_end;
		}
		m_end->~T();
	}

	// Moves the end element down to replace erased element.
	HX_INLINE void erase_unordered(T* it) {
		hxAssert((uint32_t)(it - this->getStorage()) < size());
		if (it != --m_end) {
			*it = *m_end;
		}
		m_end->~T();
	}

	HX_INLINE bool full() {
		return size() == capacity();
	}

private:
	HX_INLINE void construct(T* begin, T* end) {
		while (begin != end) {
			::new (begin++) T;
		}
	}

	HX_INLINE void destruct(T* begin, T* end) {
		while (begin != end) {
			(begin++)->~T();
		}
	}
	T* m_end;
};
