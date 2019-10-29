#pragma once
// Copyright 2017 Adrian Johnston

#include "hxAllocator.h"

#if HX_HAS_CPP11_THREADS
#include <atomic>
#endif

// ----------------------------------------------------------------------------
// hxStockpile
//
// Provides atomic storage for results of multi-threaded processing.

template<typename T, uint32_t Capacity>
class hxStockpile : private hxAllocator<T, Capacity> {
public:
	static_assert(Capacity > 0u, "fixed size only");

	HX_INLINE explicit hxStockpile() { m_size = 0u; }
	HX_INLINE ~hxStockpile() { destruct(); }

	HX_INLINE const T& operator[](uint32_t index) const {
		hxAssert(index < hxMin((uint32_t)m_size, Capacity));
		return this->getStorage()[index];
	}
	HX_INLINE       T& operator[](uint32_t index) {
		hxAssert(index < hxMin((uint32_t)m_size, Capacity));
		return this->getStorage()[index];
	}

	HX_INLINE uint32_t size() const { return hxMin((uint32_t)m_size, Capacity); }
	HX_INLINE uint32_t capacity() const { return Capacity; }

	HX_INLINE bool empty() const { return m_size == 0u; }
	HX_INLINE bool full() { return m_size >= Capacity; }

	HX_INLINE const T* data() const { return this->getStorage(); }
	HX_INLINE       T* data() { return this->getStorage(); }

	HX_INLINE bool push_back_atomic(const T& t) {
		uint32_t index = m_size++;
		if (index < Capacity) {
			::new (this->getStorage() + index) T(t);
			return true;
		}
		m_size = Capacity;
		return false;
	}

	// Returns pointer for use with placement new, if available.
	HX_INLINE void* emplace_back_unconstructed_atomic() {
		uint32_t index = m_size++;
		if (index < Capacity) {
			return this->getStorage() + index;
		}
		m_size = Capacity;
		return null;
	}

	HX_INLINE void clear() {
		destruct();
		m_size = 0u;
	}

private:
	HX_INLINE void destruct() {
		T* begin = this->getStorage();
		uint32_t size = hxMin((uint32_t)m_size, Capacity);
		while (size--) {
			(begin++)->~T();
		}
	}

	explicit hxStockpile(const hxStockpile& rhs); // = delete
	void operator=(const hxStockpile& rhs); // = delete

#if HX_HAS_CPP11_THREADS
	std::atomic<uint32_t> m_size;
#else
	uint32_t m_size;
#endif
};
