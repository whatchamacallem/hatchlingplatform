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

template<typename T_, uint32_t Capacity_>
class hxStockpile : private hxAllocator<T_, Capacity_> {
public:
	typedef T_ T;
	enum { Capacity = Capacity_ };

	HX_STATIC_ASSERT(Capacity > 0u, "fixed size only");

	HX_INLINE explicit hxStockpile() { m_size = 0u; }
	HX_INLINE ~hxStockpile() { destruct_(); }

	HX_INLINE const T& operator[](uint32_t index_) const {
		hxAssert(index_ < hxMin((uint32_t)m_size, Capacity));
		return this->getStorage()[index_];
	}
	HX_INLINE       T& operator[](uint32_t index_)       {
		hxAssert(index_ < hxMin((uint32_t)m_size, (uint32_t)Capacity));
		return this->getStorage()[index_];
	}

	HX_INLINE uint32_t size() const { return hxMin((uint32_t)m_size, (uint32_t)Capacity); }
	HX_CONSTEXPR_FN uint32_t capacity() const { return Capacity; }

	HX_INLINE bool empty() const { return m_size == 0u; }
	HX_INLINE bool full() { return m_size >= Capacity; }

	HX_INLINE const T* data() const { return this->getStorage(); }
	HX_INLINE       T* data() { return this->getStorage(); }

	HX_INLINE bool push_back_atomic(const T& t_) {
		uint32_t index_ = m_size++;
		if (index_ < Capacity) {
			::new (this->getStorage() + index_) T(t_);
			return true;
		}
		m_size = Capacity;
		return false;
	}

	// Returns pointer for use with placement new, if available.
	HX_INLINE void* emplace_back_atomic() {
		uint32_t index_ = m_size++;
		if (index_ < Capacity) {
			return this->getStorage() + index_;
		}
		m_size = Capacity;
		return hxnull;
	}

	HX_INLINE void clear() {
		destruct_();
		m_size = 0u;
	}

private:
	HX_INLINE void destruct_() {
		T* t_ = this->getStorage();
		uint32_t sz_ = hxMin((uint32_t)m_size, (uint32_t)Capacity);
		while (sz_--) {
			t_++->~T();
		}
	}

	explicit hxStockpile(const hxStockpile& rhs_); // = delete
	void operator=(const hxStockpile& rhs_); // = delete

#if HX_USE_CPP11_THREADS
	std::atomic<uint32_t> m_size;
#else
	uint32_t m_size;
#endif
};
