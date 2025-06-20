// Copyright 2017-2025 Adrian Johnston

// HX_RELEASE < 1 memory markings:
//
//   ab - Allocated to client code.
//   cd - Allocated to hxallocator static allocation.
//   dd - Belongs to memory manager.
//   ee - Freed to OS heap.

#include <hx/hatchling.h>
#include <hx/hxmemory_manager.h>

#if HX_USE_THREADS
#include <mutex>
#endif

HX_REGISTER_FILENAME_HASH

#if !HX_HOSTED
void* operator new(size_t size) {
    void* ptr = ::malloc(size);
	hxassertrelease(ptr, "malloc fail: %u bytes\n", (unsigned int)size);
    return ptr;
}
void* operator new[](size_t size) {
    void* ptr = ::malloc(size);
	hxassertrelease(ptr, "malloc fail: %u bytes\n", (unsigned int)size);
    return ptr;
}
void operator delete(void* ptr) hxnoexcept {
    ::free(ptr);
}
void operator delete(void* ptr, size_t) hxnoexcept {
    ::free(ptr);
}
void operator delete[](void* ptr) hxnoexcept {
    ::free(ptr);
}
void operator delete[](void* ptr, size_t) hxnoexcept {
    ::free(ptr);
}
#endif

// hxmalloc_checked. Always check malloc and halt on failure. This is extremely
// important with hardware where 0 is a valid address and can be written to with
// disastrous results.
static hxconstexpr_fn void* hxmalloc_checked(size_t size) {
	void* t = ::malloc(size);
	hxassertrelease(t, "malloc fail: %zu bytes\n", size);
#if (HX_RELEASE) >= 3
	if (!t) {
		hxloghandler(hxloglevel_assert, "malloc fail");
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
static std::mutex s_hxmemory_manager_mutex;
#define HX_MEMORY_MANAGER_LOCK_() std::unique_lock<std::mutex> hxmutex_lock_(s_hxmemory_manager_mutex)
#else
#define HX_MEMORY_MANAGER_LOCK_() (void)0
#endif

namespace {

// Needs to be a pointer to prevent a constructor running at a bad time.
class hxmemory_manager* s_hxmemory_manager = hxnull;

// ----------------------------------------------------------------------------
// hxmemory_allocation_header

class hxmemory_allocation_header {
public:
	uintptr_t size;
	uintptr_t actual; // address actually returned by malloc.

#if (HX_RELEASE) < 2
	static const uint32_t guard_value_ = 0xc811b135u;
	uint32_t guard;
#endif
};

// ----------------------------------------------------------------------------
// hxmemory_allocator_base

class hxmemory_allocator_base {
public:
	hxmemory_allocator_base() : m_label_(hxnull) { }
	void* allocate(size_t size, size_t alignment) {
		return on_alloc(size, alignment);
	}

	virtual void begin_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator new_id) = 0;
	virtual void end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator old_id) = 0;
	virtual size_t get_allocation_count(hxmemory_allocator id) const = 0;
	virtual size_t get_bytes_allocated(hxmemory_allocator id) const = 0;
	virtual size_t get_high_water(hxmemory_allocator id) = 0;
	const char* label() const { return m_label_; }

protected:
	virtual void* on_alloc(size_t size, size_t alignment) = 0;
	const char* m_label_;
private:
	void operator=(const hxmemory_allocator_base&) hxdelete_fn;
};

// ----------------------------------------------------------------------------
// hxmemory_allocator_os_heap
//
// Wraps heap allocations with a header and adds padding to obtain required
// alignment. This is only intended for large or debug allocations with small
// alignment requirements. For lots of small allocations use a small block
// allocator. For large alignments see if aligned_alloc() is available.

class hxmemory_allocator_os_heap : public hxmemory_allocator_base {
public:
	void construct(const char* label) {
		m_label_ = label;
		m_allocation_count = 0u;
		m_bytes_allocated = 0u;
		m_high_water = 0u;
	}

	virtual void begin_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator new_id) hxoverride { (void)scope; (void)new_id; }
	virtual void end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator old_id) hxoverride { (void)scope; (void)old_id; }
	virtual size_t get_allocation_count(hxmemory_allocator id) const hxoverride { (void)id; return m_allocation_count; }
	virtual size_t get_bytes_allocated(hxmemory_allocator id) const hxoverride { (void)id; return m_bytes_allocated; }
	virtual size_t get_high_water(hxmemory_allocator id) hxoverride { (void)id; return m_high_water; }

	virtual void* on_alloc(size_t size, size_t alignment) hxoverride {
		hxassert(size != 0u); // hxmemory_allocator_base::allocate

		// hxmemory_allocation_header has an HX_ALIGNMENT alignment requirement as well.
		alignment = hxmax(alignment, HX_ALIGNMENT);
		--alignment; // use as a mask.

		// Place header immediately before aligned allocation.
		uintptr_t actual = (uintptr_t)hxmalloc_checked(size + sizeof(hxmemory_allocation_header) + alignment);
		uintptr_t aligned = (actual + sizeof(hxmemory_allocation_header) + alignment) & ~alignment;
		hxmemory_allocation_header& hdr = ((hxmemory_allocation_header*)aligned)[-1];
		hdr.size = size;
		hdr.actual = actual;
#if (HX_RELEASE) < 2
		hdr.guard = hxmemory_allocation_header::guard_value_;
#endif
		++m_allocation_count;
		m_bytes_allocated += size; // ignore overhead
		m_high_water = hxmax(m_high_water, m_bytes_allocated);

		return (void*)aligned;
	}

	void on_free_non_virtual(void* p) {
		if (p == hxnull) {
			return;
		}

		hxmemory_allocation_header& hdr = ((hxmemory_allocation_header*)p)[-1];
#if (HX_RELEASE) < 2
		hxassertrelease(hdr.guard == hxmemory_allocation_header::guard_value_, "heap free corrupt");
#endif
		hxassert(hdr.size > 0u && m_allocation_count > 0u && m_bytes_allocated > 0u);
		--m_allocation_count;
		m_bytes_allocated -= hdr.size;

		uintptr_t actual = hdr.actual;
		if ((HX_RELEASE) < 1) {
			::memset((void*)&hdr, 0xee, hdr.size + sizeof(hxmemory_allocation_header));
		}
		::free((void*)actual);
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
	void construct(void* ptr, size_t size, const char* label) {
		m_label_ = label;

		m_allocation_count = 0u;
		m_begin_ = ((uintptr_t)ptr);
		m_end_ = ((uintptr_t)ptr + size);
		m_current = ((uintptr_t)ptr);

		if ((HX_RELEASE) < 1) {
			::memset(ptr, 0xdd, size);
		}
	}

	virtual void begin_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator new_id) hxoverride { (void)scope; (void)new_id; }
	virtual void end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator old_id) hxoverride { (void)scope; (void)old_id; }
	bool contains(void* ptr) { return (uintptr_t)ptr >= m_begin_ && (uintptr_t)ptr < m_end_; }
	virtual size_t get_allocation_count(hxmemory_allocator id) const hxoverride { (void)id; return m_allocation_count; }
	virtual size_t get_bytes_allocated(hxmemory_allocator id) const hxoverride { (void)id; return m_current - m_begin_; }
	virtual size_t get_high_water(hxmemory_allocator id) hxoverride { (void)id; return m_current - m_begin_; }

	void* release() {
		void* t = (void*)m_begin_;
		m_begin_ = 0;
		return t;
	}

	void* allocate_non_virtual(size_t size, size_t alignment) {
		--alignment; // use as a mask.
		uintptr_t aligned = (m_current + alignment) & ~alignment;
		if ((aligned + size) > m_end_) {
			return hxnull;
		}

		++m_allocation_count;
		m_current = aligned + size;
		return (void*)aligned;
	}

	void on_free_non_virtual(void* ptr) {
		hxassertmsg(m_allocation_count > 0 && (uintptr_t)ptr >= m_begin_
			&& (uintptr_t)ptr < m_current, "unexpected free: %s", m_label_);
		--m_allocation_count; (void)ptr;
		return;
	}

protected:
	virtual void* on_alloc(size_t size, size_t alignment) hxoverride {
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
	void construct(void* ptr, size_t size, const char* label) {
		hxmemory_allocator_stack::construct(ptr, size, label);
		m_high_water = 0u;
	}

	virtual void end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator old_id) hxoverride {
		(void)old_id;
		hxassertmsg(m_allocation_count <= scope->get_previous_allocation_count(),
			"%s leaked %zd allocations", m_label_, m_allocation_count - scope->get_previous_allocation_count());

		m_high_water = hxmax(m_high_water, m_current);

		uintptr_t previous_current = m_begin_ + scope->get_previous_bytes_allocated();
		if ((HX_RELEASE) < 1) {
			::memset((void*)previous_current, 0xdd, (size_t)(m_current - previous_current));
		}
		m_current = previous_current;

		// This assert indicates overwriting the stack trashing *scope.
		hxassertrelease(m_current <= m_end_, "error resetting temp stack");
	}

	virtual size_t get_high_water(hxmemory_allocator id) hxoverride {
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
	void construct();
	void destruct();
	size_t leak_count();

	hxmemory_allocator begin_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator new_id);
	void end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator previous_id);

	hxmemory_allocator_base& get_allocator(hxmemory_allocator id) {
		hxassert(id >= 0 && id < hxmemory_allocator_current);
		return *m_memory_allocators[id];
	}

	void* allocate(size_t size, hxmemory_allocator id, size_t alignment);
	void free(void* ptr);

private:
	friend class hxmemory_allocator_scope;

	// Nota bene:  The current allocator is a thread local attribute.
	static hxthread_local hxmemory_allocator s_hxcurrent_memory_allocator;

	hxmemory_allocator_base* m_memory_allocators[hxmemory_allocator_current];

	hxmemory_allocator_os_heap     m_memory_allocator_heap;
	hxmemory_allocator_stack      m_memory_allocator_permanent;
	hxmemory_allocator_temp_stack  m_memory_allocator_temporary_stack;
};

hxthread_local hxmemory_allocator hxmemory_manager::s_hxcurrent_memory_allocator = hxmemory_allocator_heap;

void hxmemory_manager::construct() {
	s_hxcurrent_memory_allocator = hxmemory_allocator_heap;

	m_memory_allocators[hxmemory_allocator_heap] = &m_memory_allocator_heap;
	m_memory_allocators[hxmemory_allocator_permanent] = &m_memory_allocator_permanent;
	m_memory_allocators[hxmemory_allocator_temporary_stack] = &m_memory_allocator_temporary_stack;

	::new (&m_memory_allocator_heap) hxmemory_allocator_os_heap(); // set vtable ptr.
	::new (&m_memory_allocator_permanent) hxmemory_allocator_stack();
	::new (&m_memory_allocator_temporary_stack) hxmemory_allocator_temp_stack();

	m_memory_allocator_heap.construct("heap");
	m_memory_allocator_permanent.construct(hxmalloc_checked(HX_MEMORY_BUDGET_PERMANENT),
		(HX_MEMORY_BUDGET_PERMANENT), "perm");
	m_memory_allocator_temporary_stack.construct(hxmalloc_checked(HX_MEMORY_BUDGET_TEMPORARY_STACK),
		(HX_MEMORY_BUDGET_TEMPORARY_STACK), "temp");
}

void hxmemory_manager::destruct() {
	hxassertmsg(m_memory_allocator_permanent.get_allocation_count(hxmemory_allocator_permanent) == 0,
		"leaked permanent allocation");
	hxassertmsg(m_memory_allocator_temporary_stack.get_allocation_count(hxmemory_allocator_temporary_stack) == 0,
		"leaked temporary allocation");

	::free(m_memory_allocator_permanent.release());
	::free(m_memory_allocator_temporary_stack.release());
}

size_t hxmemory_manager::leak_count() {
	size_t leak_count = 0;
	HX_MEMORY_MANAGER_LOCK_();
	for (int32_t i = 0; i != hxmemory_allocator_current; ++i) {
		hxmemory_allocator_base& al = *m_memory_allocators[i];
		if(al.get_allocation_count((hxmemory_allocator)i)) {
			hxlogwarning("LEAK IN ALLOCATOR %s count %zu size %zu high_water %zu", al.label(),
				al.get_allocation_count((hxmemory_allocator)i),
				al.get_bytes_allocated((hxmemory_allocator)i),
				al.get_high_water((hxmemory_allocator)i));
		}
		leak_count += (size_t)al.get_allocation_count((hxmemory_allocator)i);
	}
	return leak_count;
}

hxmemory_allocator hxmemory_manager::begin_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator new_id) {
	hxassert((unsigned int)new_id < (unsigned int)hxmemory_allocator_current);

	HX_MEMORY_MANAGER_LOCK_();
	hxmemory_allocator previous_id = s_hxcurrent_memory_allocator;
	s_hxcurrent_memory_allocator = new_id;
	m_memory_allocators[s_hxcurrent_memory_allocator]->begin_allocation_scope(scope, s_hxcurrent_memory_allocator);
	return previous_id;
}

void hxmemory_manager::end_allocation_scope(hxmemory_allocator_scope* scope, hxmemory_allocator previous_id) {
	hxassert((unsigned int)previous_id < (unsigned int)hxmemory_allocator_current);

	HX_MEMORY_MANAGER_LOCK_();
	m_memory_allocators[s_hxcurrent_memory_allocator]->end_allocation_scope(scope, s_hxcurrent_memory_allocator);
	s_hxcurrent_memory_allocator = previous_id;
}

void* hxmemory_manager::allocate(size_t size, hxmemory_allocator id, size_t alignment) {
	if (id == hxmemory_allocator_current) {
		id = s_hxcurrent_memory_allocator;
	}

	if (size == 0u) {
		size = 1u; // Enforce unique pointer values.
	}

	// following code assumes that "alignment-1" is a valid mask of unused bits.
	if (alignment == 0) {
		alignment = 1u;
	}

	hxassertmsg(((alignment - 1) & (alignment)) == 0u, "alignment %zd is not power of 2", alignment);
	hxassertmsg((unsigned int)id < (unsigned int)hxmemory_allocator_current, "bad allocator: %ud", id);

	HX_MEMORY_MANAGER_LOCK_();
	void* ptr = m_memory_allocators[id]->allocate(size, alignment);
	hxassertmsg(((uintptr_t)ptr & (alignment - (uintptr_t)1)) == 0, "alignment wrong %zx from %d",
		(size_t)(uintptr_t)ptr, id);
	if (ptr) { return ptr; }
	hxlogwarning("%s is overflowing to heap, size %zd", m_memory_allocators[id]->label(), size);
	return m_memory_allocator_heap.allocate(size, alignment);
}

void hxmemory_manager::free(void* ptr) {
	// this path is hard-coded for efficiency.
	HX_MEMORY_MANAGER_LOCK_();

	if (m_memory_allocator_temporary_stack.contains(ptr)) {
		m_memory_allocator_temporary_stack.on_free_non_virtual(ptr);
		return;
	}

	if (m_memory_allocator_permanent.contains(ptr)) {
		hxwarnmsg(g_hxsettings.deallocate_permanent, "ERROR: free from permanent");
		m_memory_allocator_permanent.on_free_non_virtual(ptr);
		return;
	}

	m_memory_allocator_heap.on_free_non_virtual(ptr);
}

} // namespace

// ----------------------------------------------------------------------------
// hxmemory_allocator_scope

hxmemory_allocator_scope::hxmemory_allocator_scope(hxmemory_allocator id)
{
	hxassertrelease(s_hxmemory_manager, "no memory manager");

	// sets Current_allocator():
	m_this_allocator_ = id;
	m_previous_allocator_ = s_hxmemory_manager->begin_allocation_scope(this, id);
	hxmemory_allocator_base& al = s_hxmemory_manager->get_allocator(id);
	m_previous_allocation_count_ = al.get_allocation_count(id);
	m_previous_bytes_allocated_ = al.get_bytes_allocated(id);
}

hxmemory_allocator_scope::~hxmemory_allocator_scope() {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	s_hxmemory_manager->end_allocation_scope(this, m_previous_allocator_);
}

size_t hxmemory_allocator_scope::get_total_allocation_count() const {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_allocation_count(m_this_allocator_);
}

size_t hxmemory_allocator_scope::get_total_bytes_allocated() const {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_bytes_allocated(m_this_allocator_);
}

size_t hxmemory_allocator_scope::get_scope_allocation_count() const {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_allocation_count(m_this_allocator_) - m_previous_allocation_count_;
}

size_t hxmemory_allocator_scope::get_scope_bytes_allocated() const {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	return s_hxmemory_manager->get_allocator(m_this_allocator_).get_bytes_allocated(m_this_allocator_) - m_previous_bytes_allocated_;
}

// ----------------------------------------------------------------------------
// C API

extern "C"
void* hxmalloc(size_t size) {
	hxinit();
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	void* ptr = s_hxmemory_manager->allocate(size, hxmemory_allocator_current, HX_ALIGNMENT);
	if ((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

#include <stdio.h>

extern "C"
void* hxmalloc_ext(size_t size, hxmemory_allocator id, size_t alignment) {
	hxinit();
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	void* ptr = s_hxmemory_manager->allocate(size, (hxmemory_allocator)id, alignment);
	if ((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
void hxfree(void *ptr) {
	hxassertrelease(s_hxmemory_manager, "no memory manager");

	// Nothing allocated from the OS memory manager can be freed here.  Not unless
	// it is wrapped with hxmemory_allocator_os_heap.
	s_hxmemory_manager->free(ptr);
}

void hxmemory_manager_init() {
	hxassertrelease(!s_hxmemory_manager, "re-init memory manager");

	s_hxmemory_manager = (hxmemory_manager*)hxmalloc_checked(sizeof(hxmemory_manager));
	::memset((void*)s_hxmemory_manager, 0x00, sizeof(hxmemory_manager));

	s_hxmemory_manager->construct();
}

void hxmemory_manager_shut_down() {
	hxassertrelease(s_hxmemory_manager, "no memory manager");

	// Any allocations made while active will crash when free'd.
	size_t leak_count = hxmemory_manager_leak_count();
	hxassertrelease(leak_count == 0, "memory leaks: %zd", leak_count); (void)leak_count;

	// Return everything to the system allocator.
	s_hxmemory_manager->destruct();
	::free(s_hxmemory_manager);
	s_hxmemory_manager = hxnull;
}

size_t hxmemory_manager_leak_count() {
	hxassertrelease(s_hxmemory_manager, "no memory manager");
	return s_hxmemory_manager->leak_count();
}

// ----------------------------------------------------------------------------
#else // HX_MEMORY_MANAGER_DISABLE

extern "C"
void* hxmalloc(size_t size) {
	return hxmalloc_checked(size);
}

// No support for alignment when disabled. This makes sense for WASM.
extern "C"
void* hxmalloc_ext(size_t size, hxmemory_allocator id, size_t alignment) {
	(void)id; (void)alignment;
	return hxmalloc_checked(size);
}

extern "C"
void hxfree(void *ptr) {
	::free(ptr);
}

void hxmemory_manager_init() { }

void hxmemory_manager_shut_down() { }

size_t hxmemory_manager_leak_count() { return 0; }

hxmemory_allocator_scope::hxmemory_allocator_scope(hxmemory_allocator id)
{
	(void)id;
	m_previous_allocation_count_ = 0;
	m_previous_bytes_allocated_ = 0;
}

hxmemory_allocator_scope::~hxmemory_allocator_scope() { }

size_t hxmemory_allocator_scope::get_total_allocation_count() const { return 0; }

size_t hxmemory_allocator_scope::get_total_bytes_allocated() const { return 0; }

size_t hxmemory_allocator_scope::get_scope_allocation_count() const { return 0; }

size_t hxmemory_allocator_scope::get_scope_bytes_allocated() const { return 0; }

#endif // HX_MEMORY_MANAGER_DISABLE
