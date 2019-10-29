#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hxAllocator.h>

#if HX_USE_CPP11_THREADS
#include <atomic>
#endif

// ----------------------------------------------------------------------------
// hxStockpile
//
// Provides atomic storage for results of multi-threaded processing.  Requests
// for entries beyond Capacity will fail.

template<typename T, uint32_t Capacity>
class hxStockpile : private hxAllocator<T, Capacity> {
public:
	HX_STATIC_ASSERT(Capacity > 0u, "fixed size only");

	HX_INLINE explicit hxStockpile() { m_size = 0u; }
	HX_INLINE ~hxStockpile() { destruct(); }

	HX_INLINE const T& operator[](uint32_t index) const { hxAssert(index < hxMin((uint32_t)m_size, Capacity)); return this->getStorage()[index]; }
	HX_INLINE       T& operator[](uint32_t index)       { hxAssert(index < hxMin((uint32_t)m_size, Capacity)); return this->getStorage()[index]; }

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
	HX_INLINE void* emplace_back_atomic() {
		uint32_t index = m_size++;
		if (index < Capacity) {
			return this->getStorage() + index;
		}
		m_size = Capacity;
		return hxnull;
	}

	HX_INLINE void clear() {
		destruct();
		m_size = 0u;
	}

private:
	HX_INLINE void destruct() {
		T* t = this->getStorage();
		uint32_t sz = hxMin((uint32_t)m_size, Capacity);
		while (sz--) {
			t++->~T();
		}
	}

	explicit hxStockpile(const hxStockpile& rhs); // = delete
	void operator=(const hxStockpile& rhs); // = delete

#if HX_USE_CPP11_THREADS
	std::atomic<uint32_t> m_size;
#else
	uint32_t m_size;
#endif
};
