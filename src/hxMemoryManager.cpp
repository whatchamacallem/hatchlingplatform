// Copyright 2017-2025 Adrian Johnston

// HX_RELEASE < 1 memory markings:
//
//   ab - Allocated to client code.
//   cd - Allocated to hxAllocator static allocation.
//   dd - Belongs to memory manager.
//   ee - Freed to OS heap.

#include <hx/hatchling.h>
#include <hx/hxMemoryManager.h>

HX_REGISTER_FILENAME_HASH

#if HX_USE_CPP_THREADS
#include <mutex>
#endif

// hxMallocChecked. Always check malloc and halt on failure. This is extremely
// important with hardware where 0 is a valid address and can be written to with
// disastrous results.
static HX_CONSTEXPR_FN void* hxMallocChecked(size_t size) {
	void* t = ::malloc(size);
	hxAssertRelease(t, "malloc fail: %u bytes\n", (unsigned int)size);
#if (HX_RELEASE) >= 3
	if (!t) {
		hxLogHandler(hxLogLevel_Assert, "malloc fail");
		::_Exit(EXIT_FAILURE);
	}
#endif
	return t;
}

// HX_MEM_DIAGNOSTIC_LEVEL. See hxSettings.h.
#if (HX_MEM_DIAGNOSTIC_LEVEL) != -1

// All pathways are threadsafe by default. In theory locking could be removed
// if threads avoided sharing allocators. But I don't want to scare anyone.
#if HX_USE_CPP_THREADS
static std::mutex s_hxMemoryManagerMutex;
#define HX_MEMORY_MANAGER_LOCK_() std::unique_lock<std::mutex> hxMutexLock_(s_hxMemoryManagerMutex)
#else
#define HX_MEMORY_MANAGER_LOCK_() (void)0
#endif

namespace {

// Needs to be a pointer to prevent a constructor running at a bad time.
class hxMemoryManager* s_hxMemoryManager = hxnull;

// ----------------------------------------------------------------------------
// hxMemoryAllocationHeader

struct hxMemoryAllocationHeader {
	uintptr_t size;
	uintptr_t actual; // address actually returned by malloc.

#if (HX_RELEASE) < 2
	static const uint32_t c_guard = 0xc811b135u;
	uint32_t guard;
#endif
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorBase

class hxMemoryAllocatorBase {
public:
	hxMemoryAllocatorBase() : m_label_(hxnull) { }
	void* allocate(size_t size, size_t alignment) {
		return onAlloc(size, alignment);
	}

	virtual void beginAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator newId) = 0;
	virtual void endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator oldId) = 0;
	virtual size_t getAllocationCount(hxMemoryAllocator id) const = 0;
	virtual size_t getBytesAllocated(hxMemoryAllocator id) const = 0;
	virtual size_t getHighWater(hxMemoryAllocator id) = 0;
	const char* label() const { return m_label_; }

protected:
	virtual void* onAlloc(size_t size, size_t alignment) = 0;
	const char* m_label_;
private:
	void operator=(const hxMemoryAllocatorBase&) HX_DELETE_FN;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorOsHeap
//
// Wraps heap allocations with a header and adds padding to obtain required
// alignment. This is only intended for large or debug allocations with small
// alignment requirements. For lots of small allocations use a small block
// allocator. For large alignments see if aligned_alloc() is available.

class hxMemoryAllocatorOsHeap : public hxMemoryAllocatorBase {
public:
	void construct(const char* label) {
		m_label_ = label;
		m_allocationCount = 0u;
		m_bytesAllocated = 0u;
		m_highWater = 0u;
	}

	virtual void beginAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator newId) HX_OVERRIDE { (void)scope; (void)newId; }
	virtual void endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator oldId) HX_OVERRIDE { (void)scope; (void)oldId; }
	virtual size_t getAllocationCount(hxMemoryAllocator id) const HX_OVERRIDE { (void)id; return m_allocationCount; }
	virtual size_t getBytesAllocated(hxMemoryAllocator id) const HX_OVERRIDE { (void)id; return m_bytesAllocated; }
	virtual size_t getHighWater(hxMemoryAllocator id) HX_OVERRIDE { (void)id; return m_highWater; }

	virtual void* onAlloc(size_t size, size_t alignment) HX_OVERRIDE {
		hxAssert(size != 0u); // hxMemoryAllocatorBase::allocate

		// hxMemoryAllocationHeader has an HX_ALIGNMENT alignment requirement as well.
		alignment = hxmax(alignment, HX_ALIGNMENT);
		--alignment; // use as a mask.

		// Place header immediately before aligned allocation.
		uintptr_t actual = (uintptr_t)hxMallocChecked(size + sizeof(hxMemoryAllocationHeader) + alignment);
		uintptr_t aligned = (actual + sizeof(hxMemoryAllocationHeader) + alignment) & ~alignment;
		hxMemoryAllocationHeader& hdr = ((hxMemoryAllocationHeader*)aligned)[-1];
		hdr.size = size;
		hdr.actual = actual;
#if (HX_RELEASE) < 2
		hdr.guard = hxMemoryAllocationHeader::c_guard;
#endif
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 3
		// Record the size of the allocation in debug. Cast via (uintptr_t)
		// because %p is not portable.
		hxLog("%s: %d at %x  (count %d, bytes %d)\n", m_label_, (int)size, (unsigned int)aligned,
			(int)m_allocationCount, (int)m_bytesAllocated);
#endif
		++m_allocationCount;
		m_bytesAllocated += size; // ignore overhead
		m_highWater = hxmax(m_highWater, m_bytesAllocated);

		return (void*)aligned;
	}

	void onFreeNonVirtual(void* p) {
		if (p == hxnull) {
			return;
		}

		hxMemoryAllocationHeader& hdr = ((hxMemoryAllocationHeader*)p)[-1];
#if (HX_RELEASE) < 2
		hxAssertRelease(hdr.guard == hxMemoryAllocationHeader::c_guard, "heap free corrupt");
#endif
		hxAssert(hdr.size > 0u && m_allocationCount > 0u && m_bytesAllocated > 0u);
		--m_allocationCount;
		m_bytesAllocated -= hdr.size;

		#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 3
		// Record the size of the allocation in debug. Cast via (uintptr_t) because
		// Mac and supporting %p.
		hxLog("%s: -%d at %x  (count %d, bytes %d)\n", m_label_, (int)hdr.size,
			(unsigned int)(uintptr_t)p, (int)m_allocationCount, (int)m_bytesAllocated);
#endif
		uintptr_t actual = hdr.actual;
		if ((HX_RELEASE) < 1) {
			::memset((void*)&hdr, 0xee, hdr.size + sizeof(hxMemoryAllocationHeader));
		}
		::free((void*)actual);
	}

private:
	size_t m_allocationCount;
	size_t m_bytesAllocated;
	size_t m_highWater;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorStack: Nothing can be freed.

class hxMemoryAllocatorStack : public hxMemoryAllocatorBase {
public:
	void construct(void* ptr, size_t size, const char* label) {
		m_label_ = label;

		m_allocationCount = 0u;
		m_begin_ = ((uintptr_t)ptr);
		m_end_ = ((uintptr_t)ptr + size);
		m_current = ((uintptr_t)ptr);

		if ((HX_RELEASE) < 1) {
			::memset(ptr, 0xdd, size);
		}
	}

	virtual void beginAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator newId) HX_OVERRIDE { (void)scope; (void)newId; }
	virtual void endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator oldId) HX_OVERRIDE { (void)scope; (void)oldId; }
	bool contains(void* ptr) { return (uintptr_t)ptr >= m_begin_ && (uintptr_t)ptr < m_end_; }
	virtual size_t getAllocationCount(hxMemoryAllocator id) const HX_OVERRIDE { (void)id; return m_allocationCount; }
	virtual size_t getBytesAllocated(hxMemoryAllocator id) const HX_OVERRIDE { (void)id; return m_current - m_begin_; }
	virtual size_t getHighWater(hxMemoryAllocator id) HX_OVERRIDE { (void)id; return m_current - m_begin_; }

	void* release() {
		void* t = (void*)m_begin_;
		m_begin_ = 0;
		return t;
	}

	void* allocateNonVirtual(size_t size, size_t alignment) {
		--alignment; // use as a mask.
		uintptr_t aligned = (m_current + alignment) & ~alignment;
		if ((aligned + size) > m_end_) {
			return hxnull;
		}

		++m_allocationCount;
		m_current = aligned + size;
		return (void*)aligned;
	}

	void onFreeNonVirtual(void* ptr) {
		hxAssertMsg(m_allocationCount > 0 && (uintptr_t)ptr >= m_begin_
			&& (uintptr_t)ptr < m_current, "unexpected free: %s", m_label_);
		--m_allocationCount; (void)ptr;
		return;
	}

protected:
	virtual void* onAlloc(size_t size, size_t alignment) HX_OVERRIDE {
		return allocateNonVirtual(size, alignment);
	}

protected:
	uintptr_t m_begin_;
	uintptr_t m_end_;
	uintptr_t m_current;
	size_t m_allocationCount;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorTempStack: Resets after a scope closes.

class hxMemoryAllocatorTempStack : public hxMemoryAllocatorStack {
public:
	void construct(void* ptr, size_t size, const char* label) {
		hxMemoryAllocatorStack::construct(ptr, size, label);
		m_highWater = 0u;
	}

	virtual void endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator oldId) HX_OVERRIDE {
		(void)oldId;
		hxAssertMsg(m_allocationCount <= scope->getPreviousAllocationCount(),
			"%s leaked %d allocations", m_label_, (int)(m_allocationCount - scope->getPreviousAllocationCount()));

		m_highWater = hxmax(m_highWater, m_current);

		uintptr_t previousCurrent = m_begin_ + scope->getPreviousBytesAllocated();
		if ((HX_RELEASE) < 1) {
			::memset((void*)previousCurrent, 0xdd, (size_t)(m_current - previousCurrent));
		}
		m_current = previousCurrent;

		// This assert indicates overwriting the stack trashing *scope.
		hxAssertRelease(m_current <= m_end_, "error resetting temp stack");
	}

	virtual size_t getHighWater(hxMemoryAllocator id) HX_OVERRIDE {
		(void)id;
		m_highWater = hxmax(m_highWater, m_current);
		return m_highWater - m_begin_;
	}

protected:
	uintptr_t m_highWater;
};

// ----------------------------------------------------------------------------
// hxMemoryManager

class hxMemoryManager {
public:
	void construct();
	void destruct();
	size_t leakCount();

	hxMemoryAllocator beginAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator newId);
	void endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator previousId);

	hxMemoryAllocatorBase& getAllocator(hxMemoryAllocator id) {
		hxAssert(id >= 0 && id < hxMemoryAllocator_Current);
		return *m_memoryAllocators[id];
	}

	void* allocate(size_t size, hxMemoryAllocator id, size_t alignment);
	void free(void* ptr);

private:
	friend class hxMemoryAllocatorScope;

	// Nota bene:  The current allocator is a thread local attribute.
	static HX_THREAD_LOCAL hxMemoryAllocator s_hxCurrentMemoryAllocator;

	hxMemoryAllocatorBase* m_memoryAllocators[hxMemoryAllocator_Current];

	hxMemoryAllocatorOsHeap     m_memoryAllocatorHeap;
	hxMemoryAllocatorStack      m_memoryAllocatorPermanent;
	hxMemoryAllocatorTempStack  m_memoryAllocatorTemporaryStack;
};

HX_THREAD_LOCAL hxMemoryAllocator hxMemoryManager::s_hxCurrentMemoryAllocator = hxMemoryAllocator_Heap;

void hxMemoryManager::construct() {
	s_hxCurrentMemoryAllocator = hxMemoryAllocator_Heap;

	m_memoryAllocators[hxMemoryAllocator_Heap] = &m_memoryAllocatorHeap;
	m_memoryAllocators[hxMemoryAllocator_Permanent] = &m_memoryAllocatorPermanent;
	m_memoryAllocators[hxMemoryAllocator_TemporaryStack] = &m_memoryAllocatorTemporaryStack;

	::new (&m_memoryAllocatorHeap) hxMemoryAllocatorOsHeap(); // set vtable ptr.
	::new (&m_memoryAllocatorPermanent) hxMemoryAllocatorStack();
	::new (&m_memoryAllocatorTemporaryStack) hxMemoryAllocatorTempStack();

	m_memoryAllocatorHeap.construct("heap");
	m_memoryAllocatorPermanent.construct(hxMallocChecked(HX_MEMORY_BUDGET_PERMANENT),
		(HX_MEMORY_BUDGET_PERMANENT), "perm");
	m_memoryAllocatorTemporaryStack.construct(hxMallocChecked(HX_MEMORY_BUDGET_TEMPORARY_STACK),
		(HX_MEMORY_BUDGET_TEMPORARY_STACK), "temp");
}

void hxMemoryManager::destruct() {
	hxAssertMsg(m_memoryAllocatorPermanent.getAllocationCount(hxMemoryAllocator_Permanent) == 0,
		"leaked permanent allocation");
	hxAssertMsg(m_memoryAllocatorTemporaryStack.getAllocationCount(hxMemoryAllocator_TemporaryStack) == 0,
		"leaked temporary allocation");

	::free(m_memoryAllocatorPermanent.release());
	::free(m_memoryAllocatorTemporaryStack.release());
}

size_t hxMemoryManager::leakCount() {
	size_t leakCount = 0;
	HX_MEMORY_MANAGER_LOCK_();
	for (int32_t i = 0; i != hxMemoryAllocator_Current; ++i) {
		hxMemoryAllocatorBase& al = *m_memoryAllocators[i];
		if(al.getAllocationCount((hxMemoryAllocator)i)) {
			hxLogWarning("LEAK IN ALLOCATOR %s count %u size %u high_water %u", al.label(),
				(unsigned int)al.getAllocationCount((hxMemoryAllocator)i),
				(unsigned int)al.getBytesAllocated((hxMemoryAllocator)i),
				(unsigned int)al.getHighWater((hxMemoryAllocator)i));
		}
		leakCount += (size_t)al.getAllocationCount((hxMemoryAllocator)i);
	}
	return leakCount;
}

hxMemoryAllocator hxMemoryManager::beginAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator newId) {
	hxAssert((unsigned int)newId < (unsigned int)hxMemoryAllocator_Current);

	HX_MEMORY_MANAGER_LOCK_();
	hxMemoryAllocator previousId = s_hxCurrentMemoryAllocator;
	s_hxCurrentMemoryAllocator = newId;
	m_memoryAllocators[s_hxCurrentMemoryAllocator]->beginAllocationScope(scope, s_hxCurrentMemoryAllocator);
	return previousId;
}

void hxMemoryManager::endAllocationScope(hxMemoryAllocatorScope* scope, hxMemoryAllocator previousId) {
	hxAssert((unsigned int)previousId < (unsigned int)hxMemoryAllocator_Current);

	HX_MEMORY_MANAGER_LOCK_();
	m_memoryAllocators[s_hxCurrentMemoryAllocator]->endAllocationScope(scope, s_hxCurrentMemoryAllocator);
	s_hxCurrentMemoryAllocator = previousId;
}

void* hxMemoryManager::allocate(size_t size, hxMemoryAllocator id, size_t alignment) {
	if (id == hxMemoryAllocator_Current) {
		id = s_hxCurrentMemoryAllocator;
	}

	if (size == 0u) {
		size = 1u; // Enforce unique pointer values.
	}

	// following code assumes that "alignment-1" is a valid mask of unused bits.
	if (alignment == 0) {
		alignment = 1u;
	}

	hxAssertMsg(((alignment - 1) & (alignment)) == 0u, "alignment %d is not power of 2", (int)alignment);
	hxAssertMsg((unsigned int)id < (unsigned int)hxMemoryAllocator_Current, "bad allocator: %ud", id);

	HX_MEMORY_MANAGER_LOCK_();
	void* ptr = m_memoryAllocators[id]->allocate(size, alignment);
	hxAssertMsg(((uintptr_t)ptr & (alignment - (uintptr_t)1)) == 0, "alignment wrong %x from %d",
		(unsigned int)(uintptr_t)ptr, (int)id);
	if (ptr) { return ptr; }
	hxLogWarning("%s is overflowing to heap, size %d", m_memoryAllocators[id]->label(), (int)size);
	return m_memoryAllocatorHeap.allocate(size, alignment);
}

void hxMemoryManager::free(void* ptr) {
	// this path is hard-coded for efficiency.
	HX_MEMORY_MANAGER_LOCK_();

	if (m_memoryAllocatorTemporaryStack.contains(ptr)) {
		m_memoryAllocatorTemporaryStack.onFreeNonVirtual(ptr);
		return;
	}

	if (m_memoryAllocatorPermanent.contains(ptr)) {
		hxWarnMsg(g_hxSettings.deallocatePermanent, "ERROR: free from permanent");
		m_memoryAllocatorPermanent.onFreeNonVirtual(ptr);
		return;
	}

	m_memoryAllocatorHeap.onFreeNonVirtual(ptr);
}

} // namespace

// ----------------------------------------------------------------------------
// hxMemoryAllocatorScope

hxMemoryAllocatorScope::hxMemoryAllocatorScope(hxMemoryAllocator id)
{
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return;
	}
#endif

	// sets CurrentAllocator():
	m_thisAllocator_ = id;
	m_previousAllocator_ = s_hxMemoryManager->beginAllocationScope(this, id);
	hxMemoryAllocatorBase& al = s_hxMemoryManager->getAllocator(id);
	m_previousAllocationCount_ = al.getAllocationCount(id);
	m_previousBytesAllocated_ = al.getBytesAllocated(id);
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 2
	hxLog(" => %s, count %d, size %d\n", al.label(), (int)getTotalAllocationCount(), (int)getTotalBytesAllocated());
#endif
}

hxMemoryAllocatorScope::~hxMemoryAllocatorScope() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return;
	}
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 2
	hxLog(" <= %s, count %d/%d, size %d/%d\n", s_hxMemoryManager->getAllocator(m_thisAllocator_).label(),
		(int)getScopeAllocationCount(), (int)getTotalAllocationCount(), (int)getScopeBytesAllocated(),
		(int)getTotalBytesAllocated());
#endif
#endif
	s_hxMemoryManager->endAllocationScope(this, m_previousAllocator_);
}

size_t hxMemoryAllocatorScope::getTotalAllocationCount() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisAllocator_).getAllocationCount(m_thisAllocator_);
}

size_t hxMemoryAllocatorScope::getTotalBytesAllocated() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisAllocator_).getBytesAllocated(m_thisAllocator_);
}

size_t hxMemoryAllocatorScope::getScopeAllocationCount() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisAllocator_).getAllocationCount(m_thisAllocator_) - m_previousAllocationCount_;
}

size_t hxMemoryAllocatorScope::getScopeBytesAllocated() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisAllocator_).getBytesAllocated(m_thisAllocator_) - m_previousBytesAllocated_;
}

// ----------------------------------------------------------------------------
// C API

extern "C"
void* hxMalloc(size_t size) {
	hxInit();
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return hxMallocChecked(size);
	}
#endif
	void* ptr = s_hxMemoryManager->allocate(size, hxMemoryAllocator_Current, HX_ALIGNMENT);
	if ((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

#include <stdio.h>

extern "C"
void* hxMallocExt(size_t size, hxMemoryAllocator id, size_t alignment) {
	hxInit();
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		hxAssert(alignment <= HX_ALIGNMENT); // No support for alignment when disabled.
		return hxMallocChecked(size);
	}
#endif
	void* ptr = s_hxMemoryManager->allocate(size, (hxMemoryAllocator)id, alignment);
	if ((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
void hxFree(void *ptr) {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		::free(ptr);
	}
	else
#endif
	{
		// Nothing allocated from the OS memory manager can be freed here.  Not even
		// from hxMemoryAllocatorOsHeap.
		s_hxMemoryManager->free(ptr);
	}
}

void hxMemoryManagerInit() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
	hxAssert(!s_hxMemoryManager);
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	if (g_hxSettings.disableMemoryManager) {
		return;
	}
#endif
	s_hxMemoryManager = (hxMemoryManager*)hxMallocChecked(sizeof(hxMemoryManager));
	::memset((void*)s_hxMemoryManager, 0x00, sizeof(hxMemoryManager));

	// Contains disableMemoryManager checks
	s_hxMemoryManager->construct();
}

void hxMemoryManagerShutDown() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return;
	}
#endif
	// Any allocations made while active will crash when free'd.
	size_t leakCount = hxMemoryManagerLeakCount();
	hxAssertRelease(leakCount == 0, "memory leaks: %d", (int)leakCount); (void)leakCount;

	s_hxMemoryManager->destruct();

	// All subsequent calls to hxFree had best come from the heap.
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
	::free(s_hxMemoryManager);
	s_hxMemoryManager = hxnull;
}

size_t hxMemoryManagerLeakCount() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager,
		"disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0u;
	}
#endif
	return s_hxMemoryManager->leakCount();
}

// ----------------------------------------------------------------------------
#else // HX_MEM_DIAGNOSTIC_LEVEL == -1

extern "C"
void* hxMalloc(size_t size) {
	return hxMallocChecked(size);
}

// No support for alignment when disabled. This makes sense for WASM.
extern "C"
void* hxMallocExt(size_t size, hxMemoryAllocator id, size_t alignment) {
	(void)id; (void)alignment;
	return hxMallocChecked(size);
}

extern "C"
void hxFree(void *ptr) {
	::free(ptr);
}

void hxMemoryManagerInit() { }

void hxMemoryManagerShutDown() { }

size_t hxMemoryManagerLeakCount() { return 0; }

hxMemoryAllocatorScope::hxMemoryAllocatorScope(hxMemoryAllocator id)
{
	(void)id;
	m_previousAllocationCount_ = 0;
	m_previousBytesAllocated_ = 0;
}

hxMemoryAllocatorScope::~hxMemoryAllocatorScope() { }

size_t hxMemoryAllocatorScope::getTotalAllocationCount() const { return 0; }

size_t hxMemoryAllocatorScope::getTotalBytesAllocated() const { return 0; }

size_t hxMemoryAllocatorScope::getScopeAllocationCount() const { return 0; }

size_t hxMemoryAllocatorScope::getScopeBytesAllocated() const { return 0; }

#endif // HX_MEM_DIAGNOSTIC_LEVEL == -1
