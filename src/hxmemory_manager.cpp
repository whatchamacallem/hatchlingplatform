// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hatchling.h"
#include "../include/hx/hxthread.hpp"
#include "../include/hx/hxutility.h"

HX_REGISTER_FILENAME_HASH

// Switches heap to using allocation tracking headers in debug.
#define HX_USE_STD_ALIGNED_ALLOC (HX_CPLUSPLUS >= 201703L && (HX_RELEASE) >= 1)

#if HX_NO_LIBCXX
// Forward declare for C++11.
hxattr_hot void operator delete(void* ptr, size_t) noexcept;
hxattr_hot void operator delete[](void* ptr, size_t) noexcept;

hxattr_hot void* operator new(size_t size) {
	void* ptr = ::malloc(size);
	hxassertrelease(ptr, "malloc %zu", size);
	return ptr;
}
hxattr_hot void* operator new[](size_t size) {
	void* ptr = ::malloc(size);
	hxassertrelease(ptr, "malloc %zu", size);
	return ptr;
}
hxattr_hot void operator delete(void* ptr) noexcept {
	::free(ptr);
}
hxattr_hot void operator delete(void* ptr, size_t) noexcept {
	::free(ptr);
}
hxattr_hot void operator delete[](void* ptr) noexcept {
	::free(ptr);
}
hxattr_hot void operator delete[](void* ptr, size_t) noexcept {
	::free(ptr);
}
#endif

// hxmalloc_checked_. Always check malloc and halt on failure. This is extremely
// important with hardware where 0 is a valid address and can be written to with
// disastrous results.
hxattr_hot hxattr_noexcept static void* hxmalloc_checked_(size_t size) {
	void* t = ::malloc(size);
	hxassertrelease(t, "malloc %zu", size);
#if (HX_RELEASE) >= 3
	if(!t) {
		hxloghandler(hxloglevel_assert, "malloc %zu", size);
		::_Exit(EXIT_FAILURE);
	}
#endif
	return t;
}

// HX_MEMORY_MANAGER_DISABLE. See hxsettings.h.
#if !(HX_MEMORY_MANAGER_DISABLE)

// All pathways are threadsafe by default. In theory locking could be removed
// if threads avoided sharing allocators. But I don't want to scare anyone.
#if HX_USE_THREADS
static hxmutex s_hxmemory_manager_mutex;
#define HX_MEMORY_MANAGER_LOCK_() hxunique_lock memory_manager_lock_(s_hxmemory_manager_mutex)
#else
#define HX_MEMORY_MANAGER_LOCK_() (void)0
#endif

namespace hxdetail_ {

// Needs to be a pointer to prevent a constructor running at a bad time.
class hxmemory_manager* s_hxmemory_manager = hxnull;

// ----------------------------------------------------------------------------
// hxmemory_allocation_header - Used until C++17.
#if !HX_USE_STD_ALIGNED_ALLOC
class hxmemory_allocation_header {
public:
	size_t size;
	uintptr_t actual; // address actually returned by malloc.

#if (HX_RELEASE) < 2
	enum : uint32_t {
		sentinel_value_allocated = (uint32_t)0x00c0ffee,
		sentinel_value_freed = (uint32_t)0xdeadbeef
	} sentinel_value;
#endif
};
#endif
// ----------------------------------------------------------------------------
// hxmemory_allocator_base

class hxmemory_allocator_base {
public:
	hxmemory_allocator_base() : m_label_(hxnull) { }
	hxattr_hot void* allocate(size_t size, hxalignment_t alignment) {
		return on_alloc(size, alignment);
	}

	virtual void begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id) = 0;
	virtual void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t old_id) = 0;
	virtual size_t get_allocation_count(hxsystem_allocator_t id) const = 0;
	virtual size_t get_bytes_allocated(hxsystem_allocator_t id) const = 0;
	virtual size_t get_high_water(hxsystem_allocator_t id) = 0;
	const char* label(void) const { return m_label_; }

protected:
	virtual void* on_alloc(size_t size, hxalignment_t alignment) = 0;
	const char* m_label_;
private:
	void operator=(const hxmemory_allocator_base&) = delete;
};

// ----------------------------------------------------------------------------
// hxmemory_allocator_os_heap
//
// This just calls aligned_alloc when HX_RELEASE > 0. In debug and in C++98
// mode this code wraps heap allocations with a header and adds padding to
// obtain required alignment. This allows tracking bytes allocated in debug.
class hxmemory_allocator_os_heap : public hxmemory_allocator_base {
public:
	hxattr_cold void construct(const char* label) {
		m_label_ = label;
		m_allocation_count = 0u;
		m_bytes_allocated = 0u;
		m_high_water = 0u;
	}

	virtual void begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id) override {
		(void)scope; (void)new_id;
		scope->m_initial_allocation_count_ = m_allocation_count;
		scope->m_initial_bytes_allocated_ = m_bytes_allocated;
	}
	virtual void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t old_id) override { (void)scope; (void)old_id; }
	virtual size_t get_allocation_count(hxsystem_allocator_t id) const override {
		(void)id; return m_allocation_count;
	}
	virtual size_t get_bytes_allocated(hxsystem_allocator_t id) const override {
		(void)id; return m_bytes_allocated;
	}
	virtual size_t get_high_water(hxsystem_allocator_t id) override {
		(void)id; return m_high_water;
	}

	hxattr_hot virtual void* on_alloc(size_t size, hxalignment_t alignment) override {
#if HX_USE_STD_ALIGNED_ALLOC
		++m_allocation_count;

		// Round up the size to be a multiple of the alignment so aligned_alloc
		// doesn't fail. This has to work for every kind of allocation including
		// a 5 byte string that has default sizeof(void*) alignment.
		size = (size + (alignment - 1)) & ~((size_t)alignment - 1);

		void* t = ::aligned_alloc(alignment, size);
		hxassertrelease(t, "aligned_alloc %zu %zu", (size_t)alignment, size);
		return t;
#else
		// hxmemory_allocation_header has an HX_ALIGNMENT alignment requirement as well.
		alignment = hxmax(alignment, HX_ALIGNMENT);
		--alignment; // use as a mask.

		// Place header immediately before aligned allocation.
		const uintptr_t actual = (uintptr_t)hxmalloc_checked_(
			size + sizeof(hxmemory_allocation_header) + alignment);
		const uintptr_t aligned = (actual + sizeof(hxmemory_allocation_header) + alignment) & ~(size_t)alignment;
		hxmemory_allocation_header& hdr = ((hxmemory_allocation_header*)aligned)[-1];
		hdr.size = size;
		hdr.actual = actual;
#if (HX_RELEASE) < 2
		hdr.sentinel_value = hxmemory_allocation_header::sentinel_value_allocated;
#endif
		++m_allocation_count;
		m_bytes_allocated += size; // ignore overhead
		m_high_water = hxmax(m_high_water, m_bytes_allocated);

		return (void*)aligned;
#endif
	}

	hxattr_hot void on_free_non_virtual(void* ptr) {
#if HX_USE_STD_ALIGNED_ALLOC
		--m_allocation_count;
		::free(ptr);
#else

		hxmemory_allocation_header& hdr = ((hxmemory_allocation_header*)ptr)[-1];
#if (HX_RELEASE) < 2
		hxassertrelease(hdr.sentinel_value == hxmemory_allocation_header::sentinel_value_allocated,
			"bad_free sentinel corrupt");
#endif
		hxassertmsg(hdr.size > 0u && m_allocation_count > 0u
			&& m_bytes_allocated > 0u, "bad_free sentinel corrupt");
		--m_allocation_count;
		m_bytes_allocated -= hdr.size;

		uintptr_t actual = hdr.actual;
#if (HX_RELEASE) < 2
		hdr.sentinel_value = hxmemory_allocation_header::sentinel_value_freed;
		::memset(ptr, 0xde, hdr.size);
#endif
		::free((void*)actual);
#endif
	}

private:
	size_t m_allocation_count;
	size_t m_bytes_allocated;
	size_t m_high_water;
};

// ----------------------------------------------------------------------------
// hxmemory_allocator_stack: Nothing can be freed.

class hxmemory_allocator_stack : public hxmemory_allocator_base {
public:
	hxattr_cold void construct(void* ptr, size_t size, const char* label) {
		m_label_ = label;

		m_allocation_count = 0u;
		m_begin_ = ((uintptr_t)ptr);
		m_end_ = ((uintptr_t)ptr + size);
		m_current = ((uintptr_t)ptr);

		if((HX_RELEASE) < 1) {
			::memset(ptr, 0xcd, size);
		}
	}

	virtual void begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id) override {
			(void)scope; (void)new_id;
			scope->m_initial_allocation_count_ = m_allocation_count;
			scope->m_initial_bytes_allocated_ = m_current - m_begin_;

		}
	virtual void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t old_id) override { (void)scope; (void)old_id; }
	bool contains(void* ptr) {
		return (uintptr_t)ptr >= m_begin_ && (uintptr_t)ptr < m_end_;
	}
	virtual size_t get_allocation_count(hxsystem_allocator_t id) const override {
		(void)id; return m_allocation_count;
	}
	virtual size_t get_bytes_allocated(hxsystem_allocator_t id) const override {
		(void)id; return m_current - m_begin_;
	}
	virtual size_t get_high_water(hxsystem_allocator_t id) override {
		(void)id; return m_current - m_begin_;
	}

	hxattr_cold void* release(void) {
		void* t = (void*)m_begin_;
		m_begin_ = 0;
		return t;
	}

	hxattr_hot void* allocate_non_virtual(size_t size, hxalignment_t alignment) {
		--alignment; // use as a mask.
		const uintptr_t aligned = (m_current + alignment) & ~(uintptr_t)alignment;
		if((aligned + size) > m_end_) {
			return hxnull;
		}

		++m_allocation_count;
		m_current = aligned + size;
		return (void*)aligned;
	}

	hxattr_hot void on_free_non_virtual(void* ptr) {
		hxassertmsg(m_allocation_count > 0 && (uintptr_t)ptr >= m_begin_
			&& (uintptr_t)ptr < m_current, "bad_free %s", m_label_);
		--m_allocation_count; (void)ptr;
		return;
	}

protected:
	hxattr_hot virtual void* on_alloc(size_t size, hxalignment_t alignment) override {
		return allocate_non_virtual(size, alignment);
	}

protected:
	uintptr_t m_begin_;
	uintptr_t m_end_;
	uintptr_t m_current;
	size_t m_allocation_count;
};

// ----------------------------------------------------------------------------
// hxmemory_allocator_temp_stack: Resets after a scope closes.

class hxmemory_allocator_temp_stack : public hxmemory_allocator_stack {
public:
	hxattr_cold void construct(void* ptr, size_t size, const char* label) {
		hxmemory_allocator_stack::construct(ptr, size, label);
		m_high_water = 0u;
	}

	hxattr_hot virtual void end_allocation_scope(hxsystem_allocator_scope* scope,
			hxsystem_allocator_t old_id) override {
		(void)old_id;
		hxassertmsg(m_allocation_count <= scope->get_initial_allocation_count(),
			"memory_leak scope %s allocations %zu", m_label_,
			m_allocation_count - scope->get_initial_allocation_count());

		m_high_water = hxmax(m_high_water, m_current);

		// Do not reset m_allocation_count = scope->get_initial_allocation_count()
		// as that just breaks leak tracking.

		const uintptr_t previous_current = m_begin_ + scope->get_initial_bytes_allocated();
		if((HX_RELEASE) < 1) {
			::memset((void*)previous_current, 0xcd, (size_t)(m_current - previous_current));
		}
		m_current = previous_current;
	}

	hxattr_hot virtual size_t get_high_water(hxsystem_allocator_t id) override {
		(void)id;
		m_high_water = hxmax(m_high_water, m_current);
		return m_high_water - m_begin_;
	}

protected:
	uintptr_t m_high_water;
};

// ----------------------------------------------------------------------------
// hxmemory_manager

class hxmemory_manager {
public:
	hxattr_cold void construct();
	hxattr_cold void destruct();
	hxattr_cold size_t leak_count();

	hxattr_hot hxsystem_allocator_t begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id);
	hxattr_hot void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t previous_id);

	hxattr_hot hxmemory_allocator_base& get_allocator(hxsystem_allocator_t id) {
		hxassertmsg(id >= 0 && id < hxsystem_allocator_current, "invalid_parameter %d", (int)id);
		return *m_memory_allocators[id];
	}

	hxattr_hot void* allocate(size_t size, hxsystem_allocator_t id, hxalignment_t alignment);
	hxattr_hot void free(void* ptr);

private:
	friend class hxsystem_allocator_scope;

	// NOTA BENE:  The current allocator is a thread local attribute.
	static hxthread_local<hxsystem_allocator_t> s_hxcurrent_memory_allocator;

	hxmemory_allocator_base* m_memory_allocators[hxsystem_allocator_current];

	hxmemory_allocator_os_heap	  m_memory_allocator_heap;
	hxmemory_allocator_stack	  m_memory_allocator_permanent;
	hxmemory_allocator_temp_stack m_memory_allocator_temporary_stack;
};

hxthread_local<hxsystem_allocator_t>
hxmemory_manager::s_hxcurrent_memory_allocator(hxsystem_allocator_heap);

void hxmemory_manager::construct(void) {
	s_hxcurrent_memory_allocator = hxsystem_allocator_heap;

	m_memory_allocators[hxsystem_allocator_heap] = &m_memory_allocator_heap;
	m_memory_allocators[hxsystem_allocator_permanent] = &m_memory_allocator_permanent;
	m_memory_allocators[hxsystem_allocator_temporary_stack] = &m_memory_allocator_temporary_stack;

	::new(&m_memory_allocator_heap) hxmemory_allocator_os_heap(); // set vtable ptr.
	::new(&m_memory_allocator_permanent) hxmemory_allocator_stack();
	::new(&m_memory_allocator_temporary_stack) hxmemory_allocator_temp_stack();

	m_memory_allocator_heap.construct("heap");
	m_memory_allocator_permanent.construct(hxmalloc_checked_(HX_MEMORY_BUDGET_PERMANENT),
		(HX_MEMORY_BUDGET_PERMANENT), "perm");
	m_memory_allocator_temporary_stack.construct(hxmalloc_checked_(HX_MEMORY_BUDGET_TEMPORARY_STACK),
		(HX_MEMORY_BUDGET_TEMPORARY_STACK), "temp");
}

void hxmemory_manager::destruct(void) {
	::free(m_memory_allocator_permanent.release());
	::free(m_memory_allocator_temporary_stack.release());
}

size_t hxmemory_manager::leak_count(void) {
	size_t leak_count = 0;
	HX_MEMORY_MANAGER_LOCK_();
	for(int32_t i = 0; i != hxsystem_allocator_current; ++i) {
		hxmemory_allocator_base& allocator = *m_memory_allocators[i];
		if(allocator.get_allocation_count((hxsystem_allocator_t)i)) {
			hxloghandler(hxloglevel_warning,
				"memory_leak %s count %zu size %zu high_water %zu",
				allocator.label(),
				allocator.get_allocation_count((hxsystem_allocator_t)i),
				allocator.get_bytes_allocated((hxsystem_allocator_t)i),
				allocator.get_high_water((hxsystem_allocator_t)i));
		}
		leak_count += (size_t)allocator.get_allocation_count((hxsystem_allocator_t)i);
	}
	return leak_count;
}

hxsystem_allocator_t hxmemory_manager::begin_allocation_scope(
		hxsystem_allocator_scope* scope, hxsystem_allocator_t new_id) {

	HX_MEMORY_MANAGER_LOCK_();
	hxsystem_allocator_t previous_id = s_hxcurrent_memory_allocator;
	s_hxcurrent_memory_allocator = new_id;
	get_allocator(s_hxcurrent_memory_allocator).begin_allocation_scope(
		scope, s_hxcurrent_memory_allocator);
	return previous_id;
}

void hxmemory_manager::end_allocation_scope(
		hxsystem_allocator_scope* scope, hxsystem_allocator_t previous_id) {
	HX_MEMORY_MANAGER_LOCK_();
	get_allocator(s_hxcurrent_memory_allocator).end_allocation_scope(
		scope, s_hxcurrent_memory_allocator);
	s_hxcurrent_memory_allocator = previous_id;
}

void* hxmemory_manager::allocate(size_t size, hxsystem_allocator_t id, hxalignment_t alignment) {
	if(id == hxsystem_allocator_current) {
		id = s_hxcurrent_memory_allocator;
	}

	if(size == 0u) {
		size = 1u; // Enforce unique pointer values.
	}

	// following code assumes that "alignment-1" is a valid mask of unused bits.
	if(alignment == 0u) {
		alignment = 1u;
	}

	hxassertmsg(((alignment - 1u) & (alignment)) == 0u, \
		"alignment_error not pow2 %zu", (size_t)alignment);

	HX_MEMORY_MANAGER_LOCK_();
	void* ptr = get_allocator(id).allocate(size, alignment);
	hxassertmsg(((uintptr_t)ptr & (alignment - (uintptr_t)1)) == 0,
		"alignment_error wrong %zx from %d", (size_t)(uintptr_t)ptr, id);
	if(ptr) { return ptr; }
	hxlogwarning("overflowing_to_heap %s size %zu", get_allocator(id).label(), size);
	return m_memory_allocator_heap.allocate(size, alignment);
}

void hxmemory_manager::free(void* ptr) {
	if(ptr == hxnull) {
		return;
	}

	// this path is hard-coded for efficiency.
	HX_MEMORY_MANAGER_LOCK_();

	if(m_memory_allocator_temporary_stack.contains(ptr)) {
		m_memory_allocator_temporary_stack.on_free_non_virtual(ptr);
		return;
	}

	if(m_memory_allocator_permanent.contains(ptr)) {
		hxwarnmsg(g_hxsettings.deallocate_permanent, "ERROR: free from permanent");
		m_memory_allocator_permanent.on_free_non_virtual(ptr);
		return;
	}

	m_memory_allocator_heap.on_free_non_virtual(ptr);
}

} // hxdetail_
using namespace hxdetail_;

// ----------------------------------------------------------------------------
// hxsystem_allocator_scope

hxattr_noexcept hxsystem_allocator_scope::hxsystem_allocator_scope(hxsystem_allocator_t id)
{
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	m_this_allocator_ = id;
	m_initial_allocator_ = s_hxmemory_manager->begin_allocation_scope(this, id);
}

hxattr_noexcept hxsystem_allocator_scope::~hxsystem_allocator_scope(void) {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	s_hxmemory_manager->end_allocation_scope(this, m_initial_allocator_);
}

size_t hxsystem_allocator_scope::get_current_allocation_count(void) const {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_allocation_count(m_this_allocator_);
}

size_t hxsystem_allocator_scope::get_current_bytes_allocated(void) const {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_bytes_allocated(m_this_allocator_);
}

// ----------------------------------------------------------------------------
// C API

extern "C"
hxattr_noexcept void* hxmalloc(size_t size) {
	hxinit();
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	void* ptr = s_hxmemory_manager->allocate(size, hxsystem_allocator_current, HX_ALIGNMENT);
	if((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
hxattr_noexcept void* hxmalloc_ext(size_t size, hxsystem_allocator_t id, hxalignment_t alignment) {
	hxinit();
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	void* ptr = s_hxmemory_manager->allocate(size, (hxsystem_allocator_t)id, alignment);
	if((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
hxattr_noexcept void hxfree(void *ptr) {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");

	// Nothing allocated from the OS memory manager can be freed here. Not unless
	// it is wrapped with hxmemory_allocator_os_heap.
	s_hxmemory_manager->free(ptr);
}

void hxmemory_manager_init(void) {
	hxassertrelease(!s_hxmemory_manager, "reinit memory manager");

	s_hxmemory_manager = (hxmemory_manager*)hxmalloc_checked_(sizeof(hxmemory_manager));
	::memset((void*)s_hxmemory_manager, 0x00, sizeof(hxmemory_manager));

	s_hxmemory_manager->construct();
}

void hxmemory_manager_shut_down(void) {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");

	// Any allocations made while active will crash when free'd. If these are
	// not fixed you will hit a leak sanitizer elsewhere.
	const size_t leak_count = s_hxmemory_manager->leak_count();
	hxassertrelease(leak_count == 0, "memory_leak at shutdown %zu", leak_count); (void)leak_count;

	// Return everything to the system allocator.
	s_hxmemory_manager->destruct();
	::free(s_hxmemory_manager);
	s_hxmemory_manager = hxnull;
}

size_t hxmemory_manager_leak_count(void) {
	hxassertmsg(s_hxmemory_manager, "not_init memory manager");
	return s_hxmemory_manager->leak_count();
}

// ----------------------------------------------------------------------------
#else // HX_MEMORY_MANAGER_DISABLE

extern "C"
hxattr_noexcept void* hxmalloc(size_t size) {
	return hxmalloc_checked_(size);
}

// No support for special alignments when disabled. This is enough for WASM.
extern "C"
hxattr_noexcept void* hxmalloc_ext(size_t size, hxsystem_allocator_t id, hxalignment_t alignment) {
	(void)id; (void)alignment;
	hxassertmsg(alignment <= HX_ALIGNMENT, "alignment_error Memory manager is disabled.");
	return hxmalloc_checked_(size);
}

extern "C"
hxattr_noexcept void hxfree(void *ptr) {
	::free(ptr);
}

void hxmemory_manager_init(void) { }

void hxmemory_manager_shut_down(void) { }

size_t hxmemory_manager_leak_count(void) { return 0; }

hxattr_noexcept hxsystem_allocator_scope::hxsystem_allocator_scope(hxsystem_allocator_t id)
{
	(void)id;
	m_initial_allocation_count_ = 0;
	m_initial_bytes_allocated_ = 0;
}

hxattr_noexcept hxsystem_allocator_scope::~hxsystem_allocator_scope(void) { }

size_t hxsystem_allocator_scope::get_current_allocation_count(void) const { return 0; }

size_t hxsystem_allocator_scope::get_current_bytes_allocated(void) const { return 0; }

#endif // HX_MEMORY_MANAGER_DISABLE
