#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxAllocator.h>

// ----------------------------------------------------------------------------
// hxUniquePtr
//
// An implementation of std::unique_ptr using hxDeleter.  Missing move semantics.

template<typename T, class Deleter=hxDeleter>
struct hxUniquePtr {
public:
	HX_INLINE hxUniquePtr() { m_ptr = hxnull; }
	HX_INLINE explicit hxUniquePtr(T* t) { m_ptr = t; }
	HX_INLINE hxUniquePtr(hxUniquePtr& rhs) { m_ptr = rhs.release(); }
	HX_INLINE ~hxUniquePtr() { reset(); }
	HX_INLINE void operator=(hxUniquePtr& rhs) { reset(rhs.release()); }
	HX_INLINE T* get() const { return m_ptr; }
	HX_INLINE T& operator*() const { return *m_ptr; }
	HX_INLINE T* operator->() const { return m_ptr; }
	HX_INLINE operator bool() const { return m_ptr != hxnull; }
	HX_INLINE bool operator==(const T* ptr) const { return ptr == m_ptr; }
	HX_INLINE bool operator==(const hxUniquePtr& rhs) const { return m_ptr == rhs.m_ptr; }
	HX_INLINE bool operator!=(const T* ptr) const { return ptr != m_ptr; }
	HX_INLINE bool operator!=(const hxUniquePtr& rhs) const { return m_ptr != rhs.m_ptr; }

	HX_INLINE T* release() {
		T* ptr = m_ptr;
		m_ptr = hxnull;
		return ptr;
	}

	HX_INLINE void reset(T* ptr = hxnull) {
		hxAssert(!ptr || m_ptr != ptr);
		if (m_ptr && m_ptr != ptr) {
			Deleter d; d(m_ptr);
		}
		m_ptr = ptr;
	}

private:
	T* m_ptr;
};

// Use an hxArray instead.
template<typename T, class Deleter>
struct hxUniquePtr<T[], Deleter>; // = delete
