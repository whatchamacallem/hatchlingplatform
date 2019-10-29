// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxDma.h"
#include "hxProfiler.h"
#include "hxArray.h"

HX_REGISTER_FILENAME_HASH;

// TODO
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
static hxArray<hxDmaDebugRecord, 16> s_hxDmaDebugRecords;
static uint32_t s_hxDmaBarrierCounter = 0u;
#endif

void hxDmaInit() {
	static_assert(!HX_HAS_DMA, "TODO");
}

void hxDmaShutDown() {
	static_assert(!HX_HAS_DMA, "TODO");
}

void hxDmaEndFrame() {
	hxDmaAwaitAll();
	static_assert(!HX_HAS_DMA, "TODO");
#if HX_DEBUG_DMA
	s_hxDmaBarrierCounter = 0u;
#endif
}

void hxDmaAddSyncPoint(struct hxDmaSyncPoint& barrier) {
	static_assert(!HX_HAS_DMA, "TODO");
#if HX_DEBUG_DMA
	barrier.debug = s_hxDmaBarrierCounter++;
	hxAssertMsg(barrier.debug < 1000u, "calls to hxDmaEndFrame() required");
#endif
}

void hxDmaStartLabeled(void* dst, const void* src, size_t bytes, const char* labelStaticString) {
	hxAssertMsg(src != null && dst != null && bytes != 0, "dma illegal args: %s 0x%x, 0x%x, 0x%x", (labelStaticString ? hxBasename(labelStaticString) : "hxDmaStart"), (unsigned int)(ptrdiff_t)dst, (unsigned int)(ptrdiff_t)src, (unsigned int)(ptrdiff_t)bytes);
#if HX_HAS_DMA
	static_assert(!HX_HAS_DMA, "TODO");
#else
	::memcpy(dst, src, bytes);
#endif
#if HX_DEBUG_DMA
	s_hxDmaDebugRecords.push_back(hxDmaDebugRecord(dst, src, bytes, s_hxDmaBarrierCounter, (labelStaticString ? hxBasename(labelStaticString) : "hxDmaStart")));
#endif
}

void hxDmaAwaitSyncPointLabeled(struct hxDmaSyncPoint& barrier, const char* label) {
	hxProfileScope((label ? label : "hxDmaAwait"), c_hxProfilerDefaultSamplingCutoff);
	static_assert(!HX_HAS_DMA, "TODO");
#if HX_DEBUG_DMA
	hxAssertRelease(barrier.debug < s_hxDmaBarrierCounter, "dma barrier unexpected: %s", hxBasename(label));
	for (hxDmaDebugRecord* it = (s_hxDmaDebugRecords.end() - 1); it >= s_hxDmaDebugRecords.begin(); --it) {
		// barrier.debug is the value of s_hxDmaBarrierCounter for preceeding dma.
		if (it->barrierCounter <= barrier.debug) {
			bool isOk = ::memcmp(it->dst, it->src, it->bytes) == 0;
			hxAssertRelease(isOk, "dma corrupt %s, %s", hxBasename(it->labelStaticString), hxBasename(label));
			s_hxDmaDebugRecords.erase_unordered(it);
		}
	}
#endif
}

void hxDmaAwaitAllLabeled(const char* label) {
	hxDmaSyncPoint b;
	hxDmaAddSyncPoint(b);
	hxDmaAwaitSyncPointLabeled(b, label);
#if HX_DEBUG_DMA
	hxAssertRelease(s_hxDmaDebugRecords.empty(), "dma await failed: %s", (label ? hxBasename(label) : "hxDmaAwaitAll"));
#endif
}
