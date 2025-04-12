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

// hxMallocChecked.  Always check malloc and halt on failure.  This is extremely
// important with hardware where 0 is a valid address and can be written to with
// disastrous results.
static HX_INLINE void* hxMallocChecked(size_t size) {
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

// HX_MEM_DIAGNOSTIC_LEVEL.  See hxSettings.h.
#if (HX_MEM_DIAGNOSTIC_LEVEL) != -1

namespace {

// Needs to be a pointer to prevent a constructor running at a bad time.
class hxMemoryManager* s_hxMemoryManager = hxnull;

// ----------------------------------------------------------------------------
// hxScratchpad

#if HX_USE_MEMORY_SCRATCH
template<size_t Bytes>
struct hxScratchpad {
	HX_INLINE void* data() { return &*m_storage; }
	HX_INLINE bool contains(void* ptr) {
		return ptr >= m_storage && ptr < (m_storage + (Bytes / sizeof(uintptr_t)));
	}
	uintptr_t m_storage[Bytes / sizeof(uintptr_t)]; // C++98 hack to align to pointer size.
};

// TODO: This needs a special linker section.
HX_LINK_SCRATCHPAD hxScratchpad<((HX_MEMORY_BUDGET_SCRATCH_PAGE) * 3u
	+ (HX_MEMORY_BUDGET_SCRATCH_TEMP))> g_hxScratchpadObject;

extern "C"
int hxIsScratchpad(void * ptr) {
	return g_hxScratchpadObject.contains(ptr) ? 1 : 0;
}
#else
extern "C"
int hxIsScratchpad(void * ptr) { (void)ptr; return 0; }
#endif // !HX_USE_MEMORY_SCRATCH

// ----------------------------------------------------------------------------
// hxMemoryAllocationHeader

struct hxMemoryAllocationHeader {
	uintptr_t size;
	uintptr_t actual; // address actually returned by malloc.

#if (HX_RELEASE) < 2
	static const size_t c_guard = 0xc811b135u;
	size_t guard;
#endif
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorBase

class hxMemoryAllocatorBase {
public:
	hxMemoryAllocatorBase() : m_label(hxnull) { }
	void* allocate(size_t size, uintptr_t alignmentMask) {
		if (size == 0u) {
			size = 1u; // Enforce unique pointer values.
		}
		return onAlloc(size, alignmentMask);
	}

	virtual void beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId) = 0;
	virtual void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId oldId) = 0;
	virtual uintptr_t getAllocationCount(hxMemoryManagerId id) const = 0;
	virtual uintptr_t getBytesAllocated(hxMemoryManagerId id) const = 0;
	virtual uintptr_t getHighWater(hxMemoryManagerId id) = 0;
	const char* label() const { return m_label; }

protected:
	virtual void* onAlloc(size_t size, uintptr_t alignmentMask) = 0;
	const char* m_label;
private:
	void operator=(const hxMemoryAllocatorBase&); // = delete
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorOsHeap
//
// Wraps heap allocations with a header and adds padding to obtain required
// alignment.  This is only intended for large or debug allocations.  For lots
// of small allocations use a small block allocator or check to see if C++17's
// (or C11's) aligned_alloc() is available and efficient on your target.

class hxMemoryAllocatorOsHeap : public hxMemoryAllocatorBase {
public:
	void construct(const char* label) {
		m_label = label;
		m_allocationCount = 0u;
		m_bytesAllocated = 0u;
		m_highWater = 0u;
	}

	virtual void beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId) HX_OVERRIDE { (void)scope; (void)newId; }
	virtual void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId oldId) HX_OVERRIDE { (void)scope; (void)oldId; }
	virtual uintptr_t getAllocationCount(hxMemoryManagerId id) const HX_OVERRIDE { (void)id; return m_allocationCount; }
	virtual uintptr_t getBytesAllocated(hxMemoryManagerId id) const HX_OVERRIDE { (void)id; return m_bytesAllocated; }
	virtual uintptr_t getHighWater(hxMemoryManagerId id) HX_OVERRIDE { (void)id; return m_highWater; }

	virtual void* onAlloc(size_t size, uintptr_t alignmentMask) HX_OVERRIDE {
		hxAssert(size != 0u); // hxMemoryAllocatorBase::allocate
		++m_allocationCount;
		m_bytesAllocated += size; // ignore overhead
		if (m_highWater < m_bytesAllocated) {
			m_highWater = m_bytesAllocated;
		}

		// hxMemoryAllocationHeader has an HX_ALIGNMENT_MASK alignment mask as well.
		if (alignmentMask < HX_ALIGNMENT_MASK) {
			alignmentMask = HX_ALIGNMENT_MASK;
		}

		// Place header immediately before aligned allocation.
		uintptr_t actual = (uintptr_t)hxMallocChecked(size + sizeof(hxMemoryAllocationHeader) + alignmentMask);
		uintptr_t aligned = (actual + sizeof(hxMemoryAllocationHeader) + alignmentMask) & ~alignmentMask;
		hxMemoryAllocationHeader& hdr = ((hxMemoryAllocationHeader*)aligned)[-1];
		hdr.size = size;
		hdr.actual = actual;
#if (HX_RELEASE) < 2
		hdr.guard = hxMemoryAllocationHeader::c_guard;
#endif
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 3
		// Record the size of the allocation in debug.  Cast via (uintptr_t)
		// because %p is not portable.
		hxLog("%s: %d at %x  (count %d, bytes %d)\n", m_label, (int)size, (unsigned int)aligned,
			(int)m_allocationCount, (int)m_bytesAllocated);
#endif
		return (void*)aligned;
	}

	void onFreeNonVirtual(void* p) {
		if (p == hxnull) {
			return;
		}

		hxAssert(m_allocationCount > 0u);
		--m_allocationCount;
		hxMemoryAllocationHeader& hdr = ((hxMemoryAllocationHeader*)p)[-1];
#if (HX_RELEASE) < 2
		hxAssertRelease(hdr.guard == hxMemoryAllocationHeader::c_guard, "heap free corrupt");
		hdr.guard = 0u;
#endif
		hxAssert(hdr.size != 0); // see hxMemoryAllocatorBase::allocate
		m_bytesAllocated -= hdr.size;
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 3
		// Record the size of the allocation in debug.  Cast via (uintptr_t) because
		// Mac and supporting %p.
		hxLog("%s: -%d at %x  (count %d, bytes %d)\n", m_label, (int)hdr.size,
			(unsigned int)(uintptr_t)p, (int)m_allocationCount, (int)m_bytesAllocated);
#endif
		uintptr_t actual = hdr.actual;
		if ((HX_RELEASE) < 1) {
			::memset((void*)&hdr, 0xee, hdr.size + sizeof(hxMemoryAllocationHeader));
		}
		::free((void*)actual);
	}

private:
	uintptr_t m_allocationCount;
	uintptr_t m_bytesAllocated;
	uintptr_t m_highWater;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorStack: Nothing can be freed.

class hxMemoryAllocatorStack : public hxMemoryAllocatorBase {
public:
	void construct(void* ptr, size_t size, const char* label) {
		m_label = label;

		m_allocationCount = 0u;
		m_begin = ((uintptr_t)ptr);
		m_end = ((uintptr_t)ptr + size);
		m_current = ((uintptr_t)ptr);

		if ((HX_RELEASE) < 1) {
			::memset(ptr, 0xdd, size);
		}
	}

	virtual void beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId) HX_OVERRIDE { (void)scope; (void)newId; }
	virtual void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId oldId) HX_OVERRIDE { (void)scope; (void)oldId; }
	bool contains(void* ptr) { return (uintptr_t)ptr >= m_begin && (uintptr_t)ptr < m_end; }
	virtual uintptr_t getAllocationCount(hxMemoryManagerId id) const HX_OVERRIDE { (void)id; return m_allocationCount; }
	virtual uintptr_t getBytesAllocated(hxMemoryManagerId id) const HX_OVERRIDE { (void)id; return m_current - m_begin; }
	virtual uintptr_t getHighWater(hxMemoryManagerId id) HX_OVERRIDE { (void)id; return m_current - m_begin; }

	void* release() {
		void* t = (void*)m_begin;
		m_begin = 0;
		return t;
	}

	void* allocateNonVirtual(size_t size, uintptr_t alignmentMask) {
		uintptr_t aligned = (m_current + alignmentMask) & ~alignmentMask;
		if ((aligned + size) > m_end) {
			return hxnull;
		}

		++m_allocationCount;
		m_current = aligned + size;
		return (void*)aligned;
	}

	void onFreeNonVirtual(void* ptr) {
		hxAssertMsg(m_allocationCount > 0 && (uintptr_t)ptr >= m_begin
			&& (uintptr_t)ptr < m_current, "unexpected free: %s", m_label);
		if ((uintptr_t)ptr < m_current) {
			--m_allocationCount;
		}
		return;
	}

protected:
	virtual void* onAlloc(size_t size, uintptr_t alignmentMask) HX_OVERRIDE {
		return allocateNonVirtual(size, alignmentMask);
	}

protected:
	uintptr_t m_begin;
	uintptr_t m_end;
	uintptr_t m_current;
	uintptr_t m_allocationCount;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorTempStack: Resets after a scope closes.

class hxMemoryAllocatorTempStack : public hxMemoryAllocatorStack {
public:
	void construct(void* ptr, size_t size, const char* label) {
		m_label = label;

		m_allocationCount = 0u;
		m_begin = ((uintptr_t)ptr);
		m_end = ((uintptr_t)ptr + size);
		m_current = ((uintptr_t)ptr);
		m_highWater = 0u;

		if ((HX_RELEASE) < 1) {
			::memset(ptr, 0xdd, size);
		}
	}

	virtual void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId oldId) HX_OVERRIDE {
		(void)oldId;
		getHighWater(hxMemoryManagerId_Current);

		hxAssertMsg(m_allocationCount == scope->getPreviousAllocationCount(),
			"%s leaked %d allocations", m_label, (int)(m_allocationCount - scope->getPreviousAllocationCount()));
		uintptr_t previousCurrent = m_begin + scope->getPreviousBytesAllocated();
		if ((HX_RELEASE) < 1) {
			::memset((void*)previousCurrent, 0xdd, (size_t)(m_current - previousCurrent));
		}
		m_allocationCount = scope->getPreviousAllocationCount();
		m_current = previousCurrent;

		// This assert indicates overwriting the stack trashing *scope.
		hxAssertRelease(m_current <= m_end, "error resetting temp stack");
	}

	virtual uintptr_t getHighWater(hxMemoryManagerId id) HX_OVERRIDE {
		(void)id;
		if (m_highWater < (m_current - m_begin)) {
			m_highWater = (m_current - m_begin);
		}
		return m_highWater;
	}

protected:
	uintptr_t m_highWater;
};

// ----------------------------------------------------------------------------
// hxMemoryAllocatorScratchpad: A stack allocator where allocations are expected
// to leak.  This is a system for assigning intermediate locations in algorithms
// that are aware of their temporary nature.
#if HX_USE_MEMORY_SCRATCH
class hxMemoryAllocatorScratchpad : public hxMemoryAllocatorBase {
private:
	struct Section {
		uintptr_t m_begin;
		uintptr_t m_end;
		uintptr_t m_current;
		uintptr_t m_allocationCount;
		uintptr_t m_highWater;
	};

	static const size_t c_allSection = hxMemoryManagerId_ScratchAll - hxMemoryManagerId_ScratchPage0;
	static const size_t c_nSections = c_allSection + 1u;

public:
	void construct(void* ptr, size_t size, const char* label) {
		m_label = label;

		uintptr_t current = (uintptr_t)ptr;

		// This could be made custom per-algorithm.
		uintptr_t sizes[c_nSections] = {
			(HX_MEMORY_BUDGET_SCRATCH_PAGE), // hxMemoryManagerId_ScratchPage0
			(HX_MEMORY_BUDGET_SCRATCH_PAGE), // hxMemoryManagerId_ScratchPage1
			(HX_MEMORY_BUDGET_SCRATCH_PAGE), // hxMemoryManagerId_ScratchPage2
			(HX_MEMORY_BUDGET_SCRATCH_TEMP)  // hxMemoryManagerId_ScratchTemp
		};

		for (size_t i = 0; i < (size_t)c_allSection; ++i) {
			m_sections[i].m_begin = current;
			m_sections[i].m_current = 0u;
			m_sections[i].m_allocationCount = 0u;
			m_sections[i].m_highWater = current;

			current += sizes[i];

			m_sections[i].m_end = current;
		}

		// Last "all" Section is set up manually.
		Section& sectionAll = m_sections[c_allSection];
		sectionAll.m_begin = (uintptr_t)ptr;
		sectionAll.m_current = 0u;
		sectionAll.m_allocationCount = 0u;
		sectionAll.m_highWater = (uintptr_t)ptr;
		sectionAll.m_end = (uintptr_t)ptr + (((HX_MEMORY_BUDGET_SCRATCH_PAGE) * 3u
			+ (HX_MEMORY_BUDGET_SCRATCH_TEMP)));

		m_currentSection = (size_t)0;
		hxAssert((current - (uintptr_t)ptr) == (uintptr_t)size);

		if ((HX_RELEASE) < 1) {
			::memset(ptr, 0xdd, size);
		}
	}

	virtual void beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId) HX_OVERRIDE {
		(void)scope;
		m_currentSection = (size_t)newId - (size_t)hxMemoryManagerId_ScratchPage0;
		hxAssert(m_currentSection < c_nSections);
		Section& section = m_sections[m_currentSection];

		// Reopening is prohibited.
		if ((HX_RELEASE) < 1) {
			hxAssertMsg(section.m_current == 0u, "reopening scratchpad allocator");
			if (newId == hxMemoryManagerId_ScratchAll) {
				// Everything but hxMemoryManagerId_ScratchAll must be closed.
				for (size_t i = 0; i < (size_t)c_allSection; ++i) {
					hxAssertMsg(m_sections[i].m_current == 0u, "scratchpad all is exclusive");
				}
			}
			else {
				// hxMemoryManagerId_ScratchAll must be closed.
				hxAssertMsg(m_sections[c_allSection].m_current == 0u, "scratchpad all is exclusive");
			}
		}

		section.m_current = section.m_begin;
		section.m_allocationCount = 0;
	}

	virtual void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId oldId) HX_OVERRIDE {
		(void)scope;
		hxAssert(m_currentSection < c_nSections);
		Section& section = m_sections[m_currentSection];
		hxAssert(section.m_current != 0u);
		section.m_highWater = (section.m_highWater > section.m_current) ? section.m_highWater : section.m_current;

		if ((HX_RELEASE) < 1) {
			::memset((void*)section.m_begin, 0xdd, (size_t)(section.m_end - section.m_begin));
		}

		section.m_current = 0u;
		section.m_allocationCount = 0u;

		// May not be valid.
		m_currentSection = (size_t)oldId - (size_t)hxMemoryManagerId_ScratchPage0;
	}

	bool contains(void* ptr) {
		return (uintptr_t)ptr >= m_sections[0].m_begin && (uintptr_t)ptr < m_sections[c_nSections - 1u].m_end;
	}

	virtual uintptr_t getAllocationCount(hxMemoryManagerId id) const HX_OVERRIDE {
		const Section& section = m_sections[calculateSection_(id)];
		return section.m_allocationCount;
	}

	virtual uintptr_t getBytesAllocated(hxMemoryManagerId id) const HX_OVERRIDE {
		const Section& section = m_sections[calculateSection_(id)];
		return section.m_current ? (section.m_current - section.m_begin) : 0u;
	}

	virtual void* onAlloc(size_t size, uintptr_t alignmentMask) HX_OVERRIDE {
		hxAssert(m_currentSection < c_nSections);
		Section& section = m_sections[m_currentSection];
		hxAssertMsg(section.m_current != 0u, "no open scope for scratchpad allocator %d",
			(int)m_currentSection);
		uintptr_t aligned = (section.m_current + alignmentMask) & ~alignmentMask;

		if ((aligned + size) > section.m_end) {
			hxWarn("%s overflow allocating %d bytes in section %d with %d bytes available",
				m_label, (int)size, (int)m_currentSection, (int)(section.m_end - section.m_current));
			return 0;
		}

		++section.m_allocationCount;
		section.m_current = aligned + size;
		return (void*)aligned;
	}

	virtual uintptr_t getHighWater(hxMemoryManagerId id) HX_OVERRIDE {
		const Section& section = m_sections[calculateSection_(id)];
		return section.m_highWater - section.m_begin;
	}

private:
	size_t calculateSection_(hxMemoryManagerId id) const {
		size_t section = (size_t)id - (size_t)hxMemoryManagerId_ScratchPage0;
		hxAssert(section < c_nSections);
		return section;
	}

	size_t m_currentSection;
	Section m_sections[c_nSections];
};
#endif // HX_USE_MEMORY_SCRATCH

// ----------------------------------------------------------------------------
// hxMemoryManager

class hxMemoryManager {
public:
	void construct();
	void destruct();
	size_t allocationCount();

	hxMemoryManagerId beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId);
	void endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId previousId);

	hxMemoryAllocatorBase& getAllocator(hxMemoryManagerId id) {
		hxAssert(id >= 0 && id < hxMemoryManagerId_MAX);
		return *m_memoryAllocators[id];
	}

	void* allocate(size_t size);
	void* AllocateExtended(size_t size, hxMemoryManagerId id, uintptr_t alignmentMask);
	void free(void* ptr);

private:
	friend class hxMemoryManagerScope;

	// Nota bene:  While the current allocator is a thread local attribute, the
	// memory manager does not support concurrent access to the same allocator.
	// Either preallocate working buffers or arrange for locking around shared
	// allocators.  
	static HX_THREAD_LOCAL hxMemoryManagerId s_hxCurrentMemoryAllocator;

	hxMemoryAllocatorBase* m_memoryAllocators[hxMemoryManagerId_MAX];

	hxMemoryAllocatorOsHeap     m_memoryAllocatorHeap;
	hxMemoryAllocatorStack      m_memoryAllocatorPermanent;
	hxMemoryAllocatorTempStack  m_memoryAllocatorTemporaryStack;
#if HX_USE_MEMORY_SCRATCH
	hxMemoryAllocatorScratchpad m_memoryAllocatorScratch;
#endif // HX_USE_MEMORY_SCRATCH
};

HX_THREAD_LOCAL hxMemoryManagerId hxMemoryManager::s_hxCurrentMemoryAllocator = hxMemoryManagerId_Heap;

void hxMemoryManager::construct() {
	hxLog("memory manager init.\n");

	s_hxCurrentMemoryAllocator = hxMemoryManagerId_Heap;

	m_memoryAllocators[hxMemoryManagerId_Heap] = &m_memoryAllocatorHeap;
	m_memoryAllocators[hxMemoryManagerId_Permanent] = &m_memoryAllocatorPermanent;
	m_memoryAllocators[hxMemoryManagerId_TemporaryStack] = &m_memoryAllocatorTemporaryStack;

	::new (&m_memoryAllocatorHeap) hxMemoryAllocatorOsHeap(); // set vtable ptr.
	::new (&m_memoryAllocatorPermanent) hxMemoryAllocatorStack();
	::new (&m_memoryAllocatorTemporaryStack) hxMemoryAllocatorTempStack();

	m_memoryAllocatorHeap.construct("heap");
	m_memoryAllocatorPermanent.construct(hxMallocChecked(HX_MEMORY_BUDGET_PERMANENT),
		(HX_MEMORY_BUDGET_PERMANENT), "perm");
	m_memoryAllocatorTemporaryStack.construct(hxMallocChecked(HX_MEMORY_BUDGET_TEMPORARY_STACK),
		(HX_MEMORY_BUDGET_TEMPORARY_STACK), "temp");

#if HX_USE_MEMORY_SCRATCH
	for (int32_t i = hxMemoryManagerId_ScratchPage0; i <= hxMemoryManagerId_ScratchAll; ++i) {
		m_memoryAllocators[i] = &m_memoryAllocatorScratch;
	}

	::new (&m_memoryAllocatorScratch) hxMemoryAllocatorScratchpad();

	m_memoryAllocatorScratch.construct(g_hxScratchpadObject.data(), sizeof g_hxScratchpadObject, "scratchpad");
#endif // HX_USE_MEMORY_SCRATCH
}

void hxMemoryManager::destruct() {
	hxAssertMsg(m_memoryAllocatorPermanent.getAllocationCount(hxMemoryManagerId_Permanent) == 0,
		"leaked permanent allocation");
	hxAssertMsg(m_memoryAllocatorTemporaryStack.getAllocationCount(hxMemoryManagerId_TemporaryStack) == 0,
		"leaked temporary allocation");

	::free(m_memoryAllocatorPermanent.release());
	::free(m_memoryAllocatorTemporaryStack.release());
}

size_t hxMemoryManager::allocationCount() {
	size_t allocationCount = 0;
	hxLog("memory manager allocation count:\n");
	for (int32_t i = 0; i != hxMemoryManagerId_MAX; ++i) {
		hxMemoryAllocatorBase& al = *m_memoryAllocators[i];
		hxLog("  %s count %u size %u high_water %u\n", al.label(),
			(unsigned int)al.getAllocationCount((hxMemoryManagerId)i),
			(unsigned int)al.getBytesAllocated((hxMemoryManagerId)i),
			(unsigned int)al.getHighWater((hxMemoryManagerId)i));
		allocationCount += (size_t)al.getAllocationCount((hxMemoryManagerId)i);
	}
	return allocationCount;
}

hxMemoryManagerId hxMemoryManager::beginAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId newId) {
	hxAssert((unsigned int)newId < (unsigned int)hxMemoryManagerId_MAX);

	hxMemoryManagerId previousId = s_hxCurrentMemoryAllocator;
	s_hxCurrentMemoryAllocator = newId;
	m_memoryAllocators[s_hxCurrentMemoryAllocator]->beginAllocationScope(scope, newId);
	return previousId;
}

void hxMemoryManager::endAllocationScope(hxMemoryManagerScope* scope, hxMemoryManagerId previousId) {
	hxAssert((unsigned int)previousId < (unsigned int)hxMemoryManagerId_MAX);

	m_memoryAllocators[s_hxCurrentMemoryAllocator]->endAllocationScope(scope, previousId);
	s_hxCurrentMemoryAllocator = previousId;
}

void* hxMemoryManager::allocate(size_t size) {
	hxAssert(s_hxCurrentMemoryAllocator >= 0 && s_hxCurrentMemoryAllocator < hxMemoryManagerId_MAX);
	hxAssert(m_memoryAllocators[s_hxCurrentMemoryAllocator]->label());
	void* ptr = m_memoryAllocators[s_hxCurrentMemoryAllocator]->allocate(size, HX_ALIGNMENT_MASK);
	hxAssertMsg(((uintptr_t)ptr & HX_ALIGNMENT_MASK) == 0, "alignment wrong %x, %s",
		(unsigned int)(uintptr_t)ptr, m_memoryAllocators[s_hxCurrentMemoryAllocator]->label());
	if (ptr) { return ptr; }
	hxWarn("%s is overflowing to heap, size %d", m_memoryAllocators[s_hxCurrentMemoryAllocator]->label(), (int)size);
	return m_memoryAllocatorHeap.allocate(size, HX_ALIGNMENT_MASK);
}

void* hxMemoryManager::AllocateExtended(size_t size, hxMemoryManagerId id, uintptr_t alignmentMask) {
	if (id == hxMemoryManagerId_Current) {
		id = s_hxCurrentMemoryAllocator;
	}

	hxAssert(((alignmentMask + 1) & (alignmentMask)) == 0u); // alignmentMask is ((1 << bits) - 1).
	hxAssert((unsigned int)id < (unsigned int)hxMemoryManagerId_MAX);

	void* ptr = m_memoryAllocators[id]->allocate(size, alignmentMask);
	hxAssertMsg(((uintptr_t)ptr & alignmentMask) == 0, "alignment wrong %x from %d",
		(unsigned int)(uintptr_t)ptr, (int)id);
	if (ptr) { return ptr; }
	hxWarn("%s is overflowing to heap, size %d", m_memoryAllocators[id]->label(), (int)size);
	return m_memoryAllocatorHeap.allocate(size, alignmentMask);
}

void hxMemoryManager::free(void* ptr) {
	// this path is hard-coded for efficiency.

	if (m_memoryAllocatorTemporaryStack.contains(ptr)) {
		m_memoryAllocatorTemporaryStack.onFreeNonVirtual(ptr);
		return;
	}
#if HX_USE_MEMORY_SCRATCH
	if (m_memoryAllocatorScratch.contains(ptr)) {
		return;
	}
#endif // HX_USE_MEMORY_SCRATCH

	if (m_memoryAllocatorPermanent.contains(ptr)) {
		hxWarnCheck(g_hxSettings.deallocatePermanent, "ERROR: free from permanent");
		m_memoryAllocatorPermanent.onFreeNonVirtual(ptr);
		return;
	}

	m_memoryAllocatorHeap.onFreeNonVirtual(ptr);
}

} // namespace

// ----------------------------------------------------------------------------
// hxMemoryManagerScope

hxMemoryManagerScope::hxMemoryManagerScope(hxMemoryManagerId id)
{
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return;
	}
#endif

	// sets CurrentAllocator():
	m_thisId = id;
	m_previousId = s_hxMemoryManager->beginAllocationScope(this, id);
	hxMemoryAllocatorBase& al = s_hxMemoryManager->getAllocator(id);
	m_previousAllocationCount = al.getAllocationCount(id);
	m_previousBytesAllocated = al.getBytesAllocated(id);
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 2
	hxLog(" => %s, count %d, size %d\n", al.label(), (int)getTotalAllocationCount(), (int)getTotalBytesAllocated());
#endif
}

hxMemoryManagerScope::~hxMemoryManagerScope() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager || m_previousId == hxMemoryManagerId_Current) {
		return;
	}
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 2
	hxLog(" <= %s, count %d/%d, size %d/%d\n", s_hxMemoryManager->getAllocator(m_thisId).label(),
		(int)getScopeAllocationCount(), (int)getTotalAllocationCount(), (int)getScopeBytesAllocated(),
		(int)getTotalBytesAllocated());
#endif
#endif
	s_hxMemoryManager->endAllocationScope(this, m_previousId);
}

uintptr_t hxMemoryManagerScope::getTotalAllocationCount() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisId).getAllocationCount(m_thisId);
}

uintptr_t hxMemoryManagerScope::getTotalBytesAllocated() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisId).getBytesAllocated(m_thisId);
}

uintptr_t hxMemoryManagerScope::getScopeAllocationCount() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisId).getAllocationCount(m_thisId) - m_previousAllocationCount;
}

uintptr_t hxMemoryManagerScope::getScopeBytesAllocated() const {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0;
	}
#endif
	return s_hxMemoryManager->getAllocator(m_thisId).getBytesAllocated(m_thisId) - m_previousBytesAllocated;
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
	void* ptr = s_hxMemoryManager->allocate(size);
	if ((HX_RELEASE) < 1) {
		::memset(ptr, 0xab, size);
	}
	return ptr;
}

extern "C"
void* hxMallocExt(size_t size, hxMemoryManagerId id, uintptr_t alignmentMask) {
	hxInit();
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager, "disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		hxAssert(alignmentMask <= HX_ALIGNMENT_MASK); // No support for alignment when disabled.
		return hxMallocChecked(size);
	}
#endif
	void* ptr = s_hxMemoryManager->AllocateExtended(size, (hxMemoryManagerId)id, alignmentMask);
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
		// Nothing allocated from the OS memory manager can be freed here.   Not even
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
	size_t allocationCount = hxMemoryManagerAllocationCount();
	hxAssertRelease(allocationCount == 0, "memory leaks: %d", (int)allocationCount); (void)allocationCount;

	s_hxMemoryManager->destruct();

	// All subsequent calls to hxFree had best come from the heap.
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
	::free(s_hxMemoryManager);
	s_hxMemoryManager = hxnull;
}

size_t hxMemoryManagerAllocationCount() {
	hxAssertRelease(g_hxIsInit, "call hxInit");
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	hxAssertMsg(!s_hxMemoryManager == !!g_hxSettings.disableMemoryManager,
		"disableMemoryManager inconsistent");
	if (!s_hxMemoryManager) {
		return 0u;
	}
#endif
	return s_hxMemoryManager->allocationCount();
}

// ----------------------------------------------------------------------------
#else // HX_MEM_DIAGNOSTIC_LEVEL == -1

extern "C"
void* hxMalloc(size_t size) {
	return hxMallocChecked(size);
}

extern "C"
void* hxMallocExt(size_t size, hxMemoryManagerId id, uintptr_t alignmentMask) {
	(void)id;
	hxAssert(alignmentMask <= HX_ALIGNMENT_MASK); (void)alignmentMask; // No support for alignment when disabled.
	return hxMallocChecked(size);
}

extern "C"
void hxFree(void *ptr) {
	::free(ptr);
}

extern "C"
int hxIsScratchpad(void * ptr) { (void)ptr; return 0; }

void hxMemoryManagerInit() { }

void hxMemoryManagerShutDown() { }

size_t hxMemoryManagerAllocationCount() { return 0; }

hxMemoryManagerScope::hxMemoryManagerScope(hxMemoryManagerId id)
{
	(void)id;
	m_previousAllocationCount = 0;
	m_previousBytesAllocated = 0;
}

hxMemoryManagerScope::~hxMemoryManagerScope() { }

uintptr_t hxMemoryManagerScope::getTotalAllocationCount() const { return 0; }

uintptr_t hxMemoryManagerScope::getTotalBytesAllocated() const { return 0; }

uintptr_t hxMemoryManagerScope::getScopeAllocationCount() const { return 0; }

uintptr_t hxMemoryManagerScope::getScopeBytesAllocated() const { return 0; }

#endif // HX_MEM_DIAGNOSTIC_LEVEL == -1
