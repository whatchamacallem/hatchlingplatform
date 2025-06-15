// Copyright 2017-2025 Adrian Johnston

// No DMA in a web browser.  The DMA code is just scaffolding.
#ifdef __EMSCRIPTEN__

#include <hx/hxDma.hpp>
#include <hx/hxArray.hpp>
#include <hx/hxProfiler.hpp>
#if HX_USE_CPP_THREADS
#include <mutex>
#endif

HX_REGISTER_FILENAME_HASH

namespace {

#if HX_DEBUG_DMA
struct hxDmaDebugRecord_ {
	hxDmaDebugRecord_(const void* d, const void* s, size_t b, size_t c, const char* l) :
		dst(d), src(s), bytes(b), barrierCounter(c), labelStringLiteral(l) { }
	const void* dst;
	const void* src;
	size_t bytes;
	size_t barrierCounter;
	const char* labelStringLiteral;
};
hxArray<hxDmaDebugRecord_, HX_DEBUG_DMA_RECORDS> s_hxDmaDebugRecords;
size_t s_hxDmaBarrierCounter = 0u;

#if HX_USE_CPP_THREADS
std::mutex s_hxDmaDebugMutex;
#define HX_DMA_DEBUG_MUTEX_LOCK std::lock_guard<std::mutex> debugGuard_(s_hxDmaDebugMutex)
#else
#define HX_DMA_DEBUG_MUTEX_LOCK ((void)0)
#endif
#endif

} // namespace

void hxDmaInit() {
	HX_STATIC_ASSERT(!HX_USE_DMA_HARDWARE, "TODO: Configure for target.");
}

void hxDmaEndFrame() {
	hxDmaAwaitAll("end frame");
	HX_STATIC_ASSERT(!HX_USE_DMA_HARDWARE, "TODO: Configure for target.");
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	s_hxDmaBarrierCounter = 0u;
#endif
}

void hxDmaAddSyncPoint(struct hxDmaSyncPoint& syncPoint) {
	(void)syncPoint;
	HX_STATIC_ASSERT(!HX_USE_DMA_HARDWARE, "TODO: Configure for target.");
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	syncPoint.debugOnly = s_hxDmaBarrierCounter++;
	hxAssertMsg(syncPoint.debugOnly < (1u << 10), "calls to hxDmaEndFrame() required");
#endif
}

void hxDmaStartLabeled(void* dst, const void* src, size_t bytes, const char* labelStringLiteral) {
	hxAssertMsg(src != hxnull && dst != hxnull && bytes != 0, "dma illegal args: %s 0x%x, 0x%x, 0x%x",
		(labelStringLiteral ? labelStringLiteral : "dma start"), (unsigned int)(uintptr_t)dst,
		(unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)bytes); (void)labelStringLiteral;
#if HX_USE_DMA_HARDWARE
	HX_STATIC_ASSERT(!HX_USE_DMA_HARDWARE, "TODO: Configure for target.");
#else
	::memcpy(dst, src, bytes);
#endif
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssert(!s_hxDmaDebugRecords.full());
	if (!s_hxDmaDebugRecords.full()) {
		s_hxDmaDebugRecords.pushBack(hxDmaDebugRecord_(dst, src, bytes, s_hxDmaBarrierCounter,
			(labelStringLiteral ? labelStringLiteral : "dma start")));
	}
#endif
}

void hxDmaAwaitSyncPointLabeled(struct hxDmaSyncPoint& syncPoint, const char* labelStringLiteral) {
	(void)syncPoint;
	hxProfileScopeMin((labelStringLiteral ? labelStringLiteral : "dma await"),
		c_hxDefaultCyclesCutoff); (void)labelStringLiteral;
	HX_STATIC_ASSERT(!HX_USE_DMA_HARDWARE, "TODO: Configure for target.");

#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssertRelease(syncPoint.debugOnly < s_hxDmaBarrierCounter, "dma sync point unexpected: %s",
		(labelStringLiteral ? labelStringLiteral : "dma await"));
	for (hxDmaDebugRecord_* it = (s_hxDmaDebugRecords.end() - 1); it >= s_hxDmaDebugRecords.begin(); --it) {
		// syncPoint.debugOnly is the value of s_hxDmaBarrierCounter for proceeding dma.
		if (it->barrierCounter <= syncPoint.debugOnly) {
			bool isOk = ::memcmp(it->dst, it->src, it->bytes) == 0;
			hxAssertRelease(isOk, "dma corrupt %s, %s", it->labelStringLiteral, (labelStringLiteral ? labelStringLiteral : "dma await"));
			s_hxDmaDebugRecords.eraseUnordered(it);
		}
	}
#endif
}

void hxDmaAwaitAllLabeled(const char* labelStringLiteral) {
	hxDmaSyncPoint b;
	hxDmaAddSyncPoint(b);
	hxDmaAwaitSyncPointLabeled(b, labelStringLiteral);
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssertRelease(s_hxDmaDebugRecords.empty(), "dma await failed %s",
		(labelStringLiteral ? labelStringLiteral : ""));
#endif
}

#endif // __EMSCRIPTEN__
