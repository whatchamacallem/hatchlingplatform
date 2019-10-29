// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxDma.h>
#include <hx/hxArray.h>
#include <hx/hxProfiler.h>
#if HX_HAS_CPP11_THREADS
#include <mutex>
#endif

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------------
// dma

// TODO: Every instance of HX_HAS_DMA has to be hooked up for your target.
#ifndef HX_HAS_DMA
#define HX_HAS_DMA 0
#endif

#if HX_DEBUG_DMA
struct hxDmaDebugRecord {
	hxDmaDebugRecord(const void* d, const void* s, size_t b, uint32_t c, const char* l) :
		dst(d), src(s), bytes(b), barrierCounter(c), labelStaticString(l) { }
	const void* dst;
	const void* src;
	size_t bytes;
	uint32_t barrierCounter;
	const char* labelStaticString;
};
static hxArray<hxDmaDebugRecord, HX_DEBUG_DMA_RECORDS> s_hxDmaDebugRecords;
static uint32_t s_hxDmaBarrierCounter = 0u;

#if HX_HAS_CPP11_THREADS
static std::mutex s_hxDmaDebugMutex;
#define HX_DMA_DEBUG_MUTEX_LOCK std::lock_guard<std::mutex> debugGuard(s_hxDmaDebugMutex)
#else
#define HX_DMA_DEBUG_MUTEX_LOCK ((void)0)
#endif
#endif

void hxDmaInit() {
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");
}

#if (HX_RELEASE) < 3
void hxDmaShutDown() {
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");
}
#endif

void hxDmaEndFrame() {
	hxDmaAwaitAll("end frame");
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	s_hxDmaBarrierCounter = 0u;
#endif
}

void hxDmaAddSyncPoint(struct hxDmaSyncPoint& syncPoint) {
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	syncPoint.debugOnly = s_hxDmaBarrierCounter++;
	hxAssertMsg(syncPoint.debugOnly < (1u << 10), "calls to hxDmaEndFrame() required");
#endif
}

void hxDmaStartLabeled(void* dst, const void* src, size_t bytes, const char* labelStaticString) {
	hxAssertMsg(src != hxnull && dst != hxnull && bytes != 0, "dma illegal args: %s 0x%x, 0x%x, 0x%x", (labelStaticString ? labelStaticString : "dma start"), (unsigned int)(uintptr_t)dst, (unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)bytes);
#if HX_HAS_DMA
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");
#else
	::memcpy(dst, src, bytes);
#endif
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssert(!s_hxDmaDebugRecords.full());
	if (!s_hxDmaDebugRecords.full()) {
		s_hxDmaDebugRecords.push_back(hxDmaDebugRecord(dst, src, bytes, s_hxDmaBarrierCounter, (labelStaticString ? labelStaticString : "dma start")));
	}
#endif
}

void hxDmaAwaitSyncPointLabeled(struct hxDmaSyncPoint& syncPoint, const char* labelStaticString) {
	hxProfileScopeMin((labelStaticString ? labelStaticString : "dma await"), c_hxProfilerDefaultSamplingCutoff);
	HX_STATIC_ASSERT(!HX_HAS_DMA, "TODO");

#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssertRelease(syncPoint.debugOnly < s_hxDmaBarrierCounter, "dma sync point unexpected: %s", (labelStaticString ? labelStaticString : "dma await"));
	for (hxDmaDebugRecord* it = (s_hxDmaDebugRecords.end() - 1); it >= s_hxDmaDebugRecords.begin(); --it) {
		// syncPoint.debugOnly is the value of s_hxDmaBarrierCounter for proceeding dma.
		if (it->barrierCounter <= syncPoint.debugOnly) {
			bool isOk = ::memcmp(it->dst, it->src, it->bytes) == 0;
			hxAssertRelease(isOk, "dma corrupt %s, %s", it->labelStaticString, (labelStaticString ? labelStaticString : "dma await"));
			s_hxDmaDebugRecords.erase_unordered(it);
		}
	}
#endif
}

void hxDmaAwaitAllLabeled(const char* labelStaticString) {
	hxDmaSyncPoint b;
	hxDmaAddSyncPoint(b);
	hxDmaAwaitSyncPointLabeled(b, labelStaticString);
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxAssertRelease(s_hxDmaDebugRecords.empty(), "dma await failed %s", (labelStaticString ? labelStaticString : ""));
#endif
}
