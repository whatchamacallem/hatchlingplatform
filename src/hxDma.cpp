// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxDma.h>
#include <hx/hxArray.h>
#include <hx/hxProfiler.h>
#if HX_USE_CPP11_THREADS
#include <mutex>
#endif

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// dma

#if HX_DEBUG_DMA
struct hxDmaDebugRecord {
	hxDmaDebugRecord(const void* d, const void* s, size_t b, uint32_t c, const char* l) :
		dst(d), src(s), bytes(b), barrierCounter(c), labelStringLiteral(l) { }
	const void* dst;
	const void* src;
	size_t bytes;
	uint32_t barrierCounter;
	const char* labelStringLiteral;
};
static hxArray<hxDmaDebugRecord, HX_DEBUG_DMA_RECORDS> s_hxDmaDebugRecords;
static uint32_t s_hxDmaBarrierCounter = 0u;

#if HX_USE_CPP11_THREADS
static std::mutex s_hxDmaDebugMutex;
#define HX_DMA_DEBUG_MUTEX_LOCK std::lock_guard<std::mutex> debugGuard(s_hxDmaDebugMutex)
#else
#define HX_DMA_DEBUG_MUTEX_LOCK ((void)0)
#endif
#endif

void hxDmaInit() {
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");
}

#if (HX_RELEASE) < 3
void hxDmaShutDown() {
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");
}
#endif

void hxDmaEndFrame() {
	hxDmaAwaitAll("end frame");
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	s_hxDmaBarrierCounter = 0u;
#endif
}

void hxDmaAddSyncPoint(struct hxDmaSyncPoint& syncPoint) {
	(void)syncPoint;
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");
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
#if HX_USE_DMA
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");
#else
	::memcpy(dst, src, bytes);
#endif
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssert(!s_hxDmaDebugRecords.full());
	if (!s_hxDmaDebugRecords.full()) {
		s_hxDmaDebugRecords.push_back(hxDmaDebugRecord(dst, src, bytes, s_hxDmaBarrierCounter,
			(labelStringLiteral ? labelStringLiteral : "dma start")));
	}
#endif
}

void hxDmaAwaitSyncPointLabeled(struct hxDmaSyncPoint& syncPoint, const char* labelStringLiteral) {
	(void)syncPoint;
	hxProfileScopeMin((labelStringLiteral ? labelStringLiteral : "dma await"),
		c_hxTimeDefaultTimingCutoff); (void)labelStringLiteral;
	HX_STATIC_ASSERT(!HX_USE_DMA, "TODO: Configure for target.");

#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssertRelease(syncPoint.debugOnly < s_hxDmaBarrierCounter, "dma sync point unexpected: %s",
		(labelStringLiteral ? labelStringLiteral : "dma await"));
	for (hxDmaDebugRecord* it = (s_hxDmaDebugRecords.end() - 1); it >= s_hxDmaDebugRecords.begin(); --it) {
		// syncPoint.debugOnly is the value of s_hxDmaBarrierCounter for proceeding dma.
		if (it->barrierCounter <= syncPoint.debugOnly) {
			bool isOk = ::memcmp(it->dst, it->src, it->bytes) == 0;
			hxAssertRelease(isOk, "dma corrupt %s, %s", it->labelStringLiteral, (labelStringLiteral ? labelStringLiteral : "dma await"));
			s_hxDmaDebugRecords.erase_unordered(it);
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
