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

// Restricts direct access to these variables to this file.
inline void hxsystem_allocator_scope_init_(hxsystem_allocator_scope* scope_,
		size_t allocation_count_, size_t bytes_allocated_) {
	scope_->m_initial_allocation_count_ = allocation_count_;
	scope_->m_initial_bytes_allocated_ = bytes_allocated_;
}

// hxmalloc_checked_ always checks malloc and halts on failure. It enforces the
// overall policy against allocation failure handling routines and the static
// analysis contract described by hxattr_allocator.
hxattr_allocator(free) hxattr_hot hxattr_noexcept static void* hxmalloc_checked_(size_t size) {
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

// All pathways are thread-safe by default. In theory locking could be removed
// if threads avoided sharing allocators, but I do not want to scare anyone.
#if HX_USE_THREADS
static hxmutex s_hxmemory_manager_mutex;
#define HX_MEMORY_MANAGER_LOCK_() const hxunique_lock memory_manager_lock_(s_hxmemory_manager_mutex)
#else
#define HX_MEMORY_MANAGER_LOCK_() (void)0
#endif

namespace {

// ----------------------------------------------------------------------------
// hxmemory_allocation_header - Used until C++17.
#if !HX_USE_STD_ALIGNED_ALLOC
class hxmemory_allocation_header {
public:
	size_t size;
	uintptr_t actual; // Address actually returned by malloc.

#if (HX_RELEASE) < 2
	enum : uint32_t {
		sentinel_value_allocated = 0x00c0ffeeu,
		sentinel_value_freed = 0xdeadbeefu
	} sentinel_value;
#endif
};
#endif
// ----------------------------------------------------------------------------
// hxmemory_allocator_base

class hxmemory_allocator_base {
public:
	hxmemory_allocator_base() : m_label_(hxnull) { }
	virtual ~hxmemory_allocator_base() = default;
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

	void begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id) override {
		(void)scope; (void)new_id;
		hxsystem_allocator_scope_init_(scope, m_allocation_count, m_bytes_allocated);
	}
	void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t old_id) override { (void)scope; (void)old_id; }
	size_t get_allocation_count(hxsystem_allocator_t id) const override {
		(void)id; return m_allocation_count;
	}
	size_t get_bytes_allocated(hxsystem_allocator_t id) const override {
		(void)id; return m_bytes_allocated;
	}
	size_t get_high_water(hxsystem_allocator_t id) override {
		(void)id; return m_high_water;
	}

	hxattr_hot void* on_alloc(size_t size, hxalignment_t alignment) override {
#if HX_USE_STD_ALIGNED_ALLOC
		++m_allocation_count;

		// Round up the size to be a multiple of the alignment so aligned_alloc
		// doesn't fail. This has to work for every kind of allocation including
		// a 5-byte string that has the default sizeof(void*) alignment.
		const size_t alignment_mask = static_cast<size_t>(alignment) - 1u;
		size = (size + alignment_mask) & ~alignment_mask;

		void* t = ::aligned_alloc(alignment, size);
		hxassertrelease(t, "aligned_alloc %zu %zu", static_cast<size_t>(alignment), size);
		return t;
#else
		// hxmemory_allocation_header has an HX_ALIGNMENT alignment requirement as well.
		alignment = hxmax(alignment, HX_ALIGNMENT);
		const uintptr_t alignment_mask = static_cast<uintptr_t>(alignment - 1u);

		// Place header immediately before aligned allocation.
		const uintptr_t actual = reinterpret_cast<uintptr_t>(hxmalloc_checked_(
			size + sizeof(hxmemory_allocation_header) + alignment_mask));
		const uintptr_t aligned = (actual + sizeof(hxmemory_allocation_header) + alignment_mask) & ~alignment_mask;
		hxmemory_allocation_header& hdr = reinterpret_cast<hxmemory_allocation_header*>(aligned)[-1];
		hdr.size = size;
		hdr.actual = actual;
#if (HX_RELEASE) < 2
		hdr.sentinel_value = hxmemory_allocation_header::sentinel_value_allocated;
#endif
		++m_allocation_count;
		m_bytes_allocated += size; // Ignore overhead.
		m_high_water = hxmax(m_high_water, m_bytes_allocated);

		return reinterpret_cast<void*>(aligned);
#endif
	}

	hxattr_hot void on_free_non_virtual(void* ptr) {
#if HX_USE_STD_ALIGNED_ALLOC
		--m_allocation_count;
		::free(ptr);
#else

		hxmemory_allocation_header& hdr = reinterpret_cast<hxmemory_allocation_header*>(ptr)[-1];
#if (HX_RELEASE) < 2
		hxassertrelease(hdr.sentinel_value == hxmemory_allocation_header::sentinel_value_allocated,
			"bad_free sentinel corrupt");
#endif
		hxassertmsg(hdr.size > 0u && m_allocation_count > 0u
			&& m_bytes_allocated > 0u, "bad_free sentinel corrupt");
		--m_allocation_count;
		m_bytes_allocated -= hdr.size;

		const uintptr_t actual = hdr.actual;
#if (HX_RELEASE) < 2
		hdr.sentinel_value = hxmemory_allocation_header::sentinel_value_freed;
		::memset(ptr, 0xde, hdr.size);
#endif
		::free(reinterpret_cast<void*>(actual));
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
		m_begin_ = reinterpret_cast<uintptr_t>(ptr);
		m_end_ = reinterpret_cast<uintptr_t>(ptr) + size;
		m_current = reinterpret_cast<uintptr_t>(ptr);

		if((HX_RELEASE) < 1) {
			::memset(ptr, 0xcd, size);
		}
	}

	void begin_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t new_id) override {
			(void)scope; (void)new_id;
			hxsystem_allocator_scope_init_(scope, m_allocation_count, m_current - m_begin_);
		}
	void end_allocation_scope(hxsystem_allocator_scope* scope,
		hxsystem_allocator_t old_id) override { (void)scope; (void)old_id; }
	bool contains(void* ptr) const {
		const uintptr_t ptr_value = reinterpret_cast<uintptr_t>(ptr);
		return ptr_value >= m_begin_ && ptr_value < m_end_;
	}
	size_t get_allocation_count(hxsystem_allocator_t id) const override {
		(void)id; return m_allocation_count;
	}
	size_t get_bytes_allocated(hxsystem_allocator_t id) const override {
		(void)id; return m_current - m_begin_;
	}
	size_t get_high_water(hxsystem_allocator_t id) override {
		(void)id; return m_current - m_begin_;
	}

	hxattr_cold void* release(void) {
		void* t = reinterpret_cast<void*>(m_begin_);
		m_begin_ = 0;
		return t;
	}

	hxattr_hot void* allocate_non_virtual(size_t size, hxalignment_t alignment) {
		const uintptr_t alignment_mask = static_cast<uintptr_t>(alignment - 1u);
		const uintptr_t aligned = (m_current + alignment_mask) & ~alignment_mask;
		if((aligned + size) > m_end_) {
			return hxnull;
		}

		++m_allocation_count;
		m_current = aligned + size;
		return reinterpret_cast<void*>(aligned);
	}

	hxattr_hot void on_free_non_virtual(void* ptr) {
		// Use <= because a valid outstanding allocation of size 0 could have
		// been made at m_current.
		const uintptr_t ptr_value = reinterpret_cast<uintptr_t>(ptr);
		hxassertmsg(m_allocation_count > 0 && ptr_value >= m_begin_
			&& ptr_value <= m_current, "bad_free %s", m_label_);
		--m_allocation_count; (void)ptr;
	}

protected:
	hxattr_hot void* on_alloc(size_t size, hxalignment_t alignment) override {
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

	hxattr_hot void end_allocation_scope(hxsystem_allocator_scope* scope,
			hxsystem_allocator_t old_id) override {
		(void)old_id;
		hxassertmsg(m_allocation_count <= scope->get_initial_allocation_count(),
			"memory_leak scope %s allocations %zu", m_label_,
			m_allocation_count - scope->get_initial_allocation_count());

		m_high_water = hxmax(m_high_water, m_current);

		// Do not reset m_allocation_count = scope->get_initial_allocation_count()
		// because that would break leak tracking.

		const uintptr_t previous_current = m_begin_ + scope->get_initial_bytes_allocated();
		if((HX_RELEASE) < 1) {
			::memset(reinterpret_cast<void*>(previous_current), 0xcd,
				static_cast<size_t>(m_current - previous_current));
		}
		m_current = previous_current;
	}

	hxattr_hot size_t get_high_water(hxsystem_allocator_t id) override {
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

	// NOTA BENE:  The current allocator is a thread-local attribute.
	static hxthread_local<hxsystem_allocator_t> s_hxcurrent_memory_allocator;

	hxmemory_allocator_base* m_memory_allocators[hxsystem_allocator_current];

	hxmemory_allocator_os_heap	  m_memory_allocator_heap;
	hxmemory_allocator_stack	  m_memory_allocator_permanent;
	hxmemory_allocator_temp_stack m_memory_allocator_temporary_stack;
};

hxthread_local<hxsystem_allocator_t>
hxmemory_manager::s_hxcurrent_memory_allocator(hxsystem_allocator_heap);

static hxmemory_manager s_hxmemory_manager;

void hxmemory_manager::construct(void) {
	s_hxcurrent_memory_allocator = hxsystem_allocator_heap;

	m_memory_allocators[hxsystem_allocator_heap] = &m_memory_allocator_heap;
	m_memory_allocators[hxsystem_allocator_permanent] = &m_memory_allocator_permanent;
	m_memory_allocators[hxsystem_allocator_temporary_stack] = &m_memory_allocator_temporary_stack;

	::new(&m_memory_allocator_heap) hxmemory_allocator_os_heap(); // Set vtable pointer.
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
		const hxsystem_allocator_t allocator_id = static_cast<hxsystem_allocator_t>(i);
		if(allocator.get_allocation_count(allocator_id) != 0u) {
			hxloghandler(hxloglevel_warning,
				"memory_leak %s count %zu size %zu high_water %zu",
				allocator.label(),
				allocator.get_allocation_count(allocator_id),
				allocator.get_bytes_allocated(allocator_id),
				allocator.get_high_water(allocator_id));
		}
		leak_count += allocator.get_allocation_count(allocator_id);
	}
	return leak_count;
}

hxsystem_allocator_t hxmemory_manager::begin_allocation_scope(
		hxsystem_allocator_scope* scope, hxsystem_allocator_t new_id) {

	HX_MEMORY_MANAGER_LOCK_();
	const hxsystem_allocator_t previous_id = s_hxcurrent_memory_allocator;
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

// NOTA BENE: It is undefined behavior to compare pointers to different
// allocations. This is consistent with the C++ standard. Allocations of size 0
// may or may not return the same pointer as previous allocations.
void* hxmemory_manager::allocate(size_t size, hxsystem_allocator_t id, hxalignment_t alignment) {
	if(id == hxsystem_allocator_current) {
		id = s_hxcurrent_memory_allocator;
	}

	// Size 0 allocations are only logged as a warning. Size zero is tested and
	// expected to work without overhead.
	hxwarnmsg(size != 0u, "allocation_error Size 0 allocation.");

	// Provide an alignment of 1 for strings and unaligned allocations. The
	// following code assumes that "alignment-1" is a valid mask of unused bits
	// and not a mask containing every bit.
	hxassertmsg(alignment != 0u, "alignment_error Allocate with alignment 1 and not 0.");
	hxassertmsg(((alignment - 1u) & alignment) == 0u,
		"alignment_error Not pow2 %zu.", static_cast<size_t>(alignment));

	HX_MEMORY_MANAGER_LOCK_();
	void* ptr = get_allocator(id).allocate(size, alignment);

	const uintptr_t alignment_mask = static_cast<uintptr_t>(alignment) - 1u;
	const uintptr_t ptr_value = reinterpret_cast<uintptr_t>(ptr);
	hxassertmsg((ptr_value & alignment_mask) == 0,
		"alignment_error wrong %zx from %d", static_cast<size_t>(ptr_value), id);

	if(ptr != hxnull) { return ptr; }

	// Will not return null.
	hxlogwarning("allocation_error %s size %zu", get_allocator(id).label(), size);
	return m_memory_allocator_heap.allocate(size, alignment);
}

void hxmemory_manager::free(void* ptr) {
	if(ptr == hxnull) {
		return;
	}

	// This path is hard-coded for efficiency.
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

} // namespace {

// ----------------------------------------------------------------------------
// hxsystem_allocator_scope

hxattr_noexcept hxsystem_allocator_scope::hxsystem_allocator_scope(hxsystem_allocator_t id)
{
	hxinit();
	m_this_allocator_ = id;
	m_initial_allocator_ = s_hxmemory_manager.begin_allocation_scope(this, id);
}

hxattr_noexcept hxsystem_allocator_scope::~hxsystem_allocator_scope(void) {
	s_hxmemory_manager.end_allocation_scope(this, m_initial_allocator_);
}

size_t hxsystem_allocator_scope::get_current_allocation_count(void) const {
	hxinit();
	return s_hxmemory_manager.get_allocator(m_this_allocator_).get_allocation_count(m_this_allocator_);
}

size_t hxsystem_allocator_scope::get_current_bytes_allocated(void) const {
	hxinit();
	return s_hxmemory_manager.get_allocator(m_this_allocator_).get_bytes_allocated(m_this_allocator_);
}

// ----------------------------------------------------------------------------
// C API

extern "C"
hxattr_noexcept void* hxmalloc(size_t size) {
	hxinit();
	void* ptr = s_hxmemory_manager.allocate(size, hxsystem_allocator_current, HX_ALIGNMENT);
	if((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
hxattr_noexcept void* hxmalloc_ext(size_t size, hxsystem_allocator_t id, hxalignment_t alignment) {
	hxinit();
	void* ptr = s_hxmemory_manager.allocate(size, id, alignment);
	if((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
hxattr_noexcept void hxfree(void *ptr) {
	hxinit();

	// Nothing allocated from the OS memory manager can be freed here unless it is
	// wrapped with hxmemory_allocator_os_heap.
	s_hxmemory_manager.free(ptr);
}

void hxmemory_manager_init(void) {
	hxassertrelease(!g_hxinit_ver_, "hxmemory_manager_init Reinit.");

	s_hxmemory_manager.construct();
}

void hxmemory_manager_shut_down(void) {
	hxassertrelease(g_hxinit_ver_, "hxmemory_manager_shut_down Not init.");

	// Any allocations made while active will crash when freed. If these are not
	// fixed you will hit a leak sanitizer elsewhere.
	const size_t leak_count = s_hxmemory_manager.leak_count();
	hxassertrelease(leak_count == 0, "memory_leak at shutdown %zu", leak_count); (void)leak_count;

	// Return everything to the system allocator.
	s_hxmemory_manager.destruct();
}

size_t hxmemory_manager_leak_count(void) {
	hxinit();
	return s_hxmemory_manager.leak_count();
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
	hxassertmsg(alignment <= HX_ALIGNMENT, "alignment_error Memory manager disabled: %zu",
		static_cast<size_t>(alignment));
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
