#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"

// ----------------------------------------------------------------------------
// DMA API.  Internally profiling and validating.

// These versions only label themselves when HX_PROFILE != 0.
#if HX_PROFILE
#define hxDmaStart(dst, src, bytes) hxDmaStartLabeled(dst, src, bytes, __FILE__ "(" HX_QUOTE(__LINE__) ") start dma")
#define hxDmaAwaitSyncPoint(barrier) hxDmaAwaitSyncPointLabeled(barrier, __FILE__ "(" HX_QUOTE(__LINE__) ") wait dma")
#define hxDmaAwaitAll() hxDmaAwaitAllLabeled(__FILE__ "(" HX_QUOTE(__LINE__) ") wait dma")
#else // !HX_PROFILE
#define hxDmaStart(dst, src, bytes) hxDmaStartLabeled(dst, src, bytes, hx_null)
#define hxDmaAwaitSyncPoint(barrier) hxDmaAwaitSyncPointLabeled(barrier, hx_null)
#define hxDmaAwaitAll() hxDmaAwaitAllLabeled(hx_null)
#endif

struct hxDmaSyncPoint {
#if HX_DEBUG_DMA
	hxDmaSyncPoint() : debug(~(uint32_t)0u), pImpl(hx_null) { }
	uint32_t debug;
#else
	hxDmaSyncPoint() : pImpl(hx_null) { }
#endif
	// TODO: Target specific handle.
	char* pImpl;
};

void hxDmaInit();
void hxDmaShutDown();

// Waits for all DMA and invalidates all barriers.  _Must_ be called at regular intervals.
void hxDmaEndFrame();

// Introduces a barrier in the DMA command stream.  The hxDmaSyncPoint object itself
// will not be modified when that barrier is reached.
void hxDmaAddSyncPoint(hxDmaSyncPoint& barrier);

// Initiates a DMA transfer from src to dst of bytes length.
void hxDmaStartLabeled(void* dst, const void* src, size_t bytes, const char* labelStaticString);

// Waits until all DMA proceeding the corrisponding call to hxDmaAddSyncPoint is completed.
void hxDmaAwaitSyncPointLabeled(hxDmaSyncPoint& barrier, const char* label);

// Waits until all DMA is completed.
void hxDmaAwaitAllLabeled(const char* label);
