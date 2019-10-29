#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxAllocator.h"

// ----------------------------------------------------------------------------
// hxUniquePtr
//
// An implementation of std::unique_ptr.

template<typename T, class Deleter=hxDeleter>
struct hxUniquePtr {
public:
	HX_INLINE hxUniquePtr() { m_ptr = null; }
	HX_INLINE explicit hxUniquePtr(T* t) { m_ptr = t; }
	HX_INLINE hxUniquePtr(hxUniquePtr& rhs) { m_ptr = rhs.release(); } // move semantics
	HX_INLINE ~hxUniquePtr() { reset(); }

	HX_INLINE void operator=(hxUniquePtr& rhs) { reset(rhs.release()); }

	HX_INLINE T* release() {
		T* ptr = m_ptr;
		m_ptr = null;
		return ptr;
	}

	// Check for reassignment is non-standard.
	HX_INLINE void reset(T* ptr = null) {
		if (m_ptr && m_ptr != ptr) {
			Deleter d; d(m_ptr);
		}
		m_ptr = ptr;
	}

	HX_INLINE T* get() const { return m_ptr; }

	HX_INLINE T& operator*() const { return *m_ptr; }
	HX_INLINE T* operator->() const { return m_ptr; }
	HX_INLINE operator bool() const { return m_ptr != null; }
	HX_INLINE bool operator==(const T* ptr) const { return ptr == m_ptr; }
	HX_INLINE bool operator==(const hxUniquePtr& rhs) const { return m_ptr == rhs.m_ptr; }
	HX_INLINE bool operator!=(const T* ptr) const { return ptr != m_ptr; }
	HX_INLINE bool operator!=(const hxUniquePtr& rhs) const { return m_ptr != rhs.m_ptr; }

private:
	T* m_ptr;
};
