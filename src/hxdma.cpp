// Copyright 2017-2025 Adrian Johnston

// No DMA in a web browser.  The DMA code is just scaffolding.
#ifndef __EMSCRIPTEN__

#include <hx/hxdma.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxprofiler.hpp>
#if HX_USE_CPP_THREADS
#include <mutex>
#endif

HX_REGISTER_FILENAME_HASH

namespace {

#if HX_DEBUG_DMA
class hxdma_debug_record_ {
public:
	hxdma_debug_record_(const void* d, const void* s, size_t b, size_t c, const char* l) :
		dst(d), src(s), bytes(b), barrier_counter(c), label_string_literal(l) { }
	const void* dst;
	const void* src;
	size_t bytes;
	size_t barrier_counter;
	const char* label_string_literal;
};
hxarray<hxdma_debug_record_, HX_DEBUG_DMA_RECORDS> s_hxdma_debug_records;
size_t s_hxdma_barrier_counter = 0u;

#if HX_USE_CPP_THREADS
std::mutex s_hxdma_debug_mutex;
#define HX_DMA_DEBUG_MUTEX_LOCK std::lock_guard<std::mutex> debug_guard_(s_hxdma_debug_mutex)
#else
#define HX_DMA_DEBUG_MUTEX_LOCK ((void)0)
#endif
#endif

} // namespace

void hxdma_init() {
	// TODO: Configure for target.
}

void hxdma_end_frame() {
	hxdma_await_all("end frame");
	// TODO: Configure for target.
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	s_hxdma_barrier_counter = 0u;
#endif
}

void hxdma_add_sync_point(class hxdma_sync_point& sync_point) {
	(void)sync_point;
	// TODO: Configure for target.
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	sync_point.debug_only = s_hxdma_barrier_counter++;
	hxassertmsg(sync_point.debug_only < (1u << 10), "calls to hxdma_end_frame() required");
#endif
}

void hxdma_start_labeled(void* dst, const void* src, size_t bytes, const char* label_string_literal) {
	hxassertmsg(src != hxnull && dst != hxnull && bytes != 0, "dma illegal args: %s 0x%x, 0x%x, 0x%x",
		(label_string_literal ? label_string_literal : "dma start"), (unsigned int)(uintptr_t)dst,
		(unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)bytes); (void)label_string_literal;

	// TODO: Configure for target.
	::memcpy(dst, src, bytes);

#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxassert(!s_hxdma_debug_records.full());
	if (!s_hxdma_debug_records.full()) {
		s_hxdma_debug_records.push_back(hxdma_debug_record_(dst, src, bytes, s_hxdma_barrier_counter,
			(label_string_literal ? label_string_literal : "dma start")));
	}
#endif
}

void hxdma_await_sync_point_labeled(class hxdma_sync_point& sync_point, const char* label_string_literal) {
	(void)sync_point;
	hxprofile_scope_min((label_string_literal ? label_string_literal : "dma await"),
		hxdefault_cycles_cutoff); (void)label_string_literal;
	// TODO: Configure for target.

#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxassertrelease(sync_point.debug_only < s_hxdma_barrier_counter, "dma sync point unexpected: %s",
		(label_string_literal ? label_string_literal : "dma await"));
	for (hxdma_debug_record_* it = (s_hxdma_debug_records.end() - 1); it >= s_hxdma_debug_records.begin(); --it) {
		// sync_point.debug_only is the value of s_hxdma_barrier_counter for proceeding dma.
		if (it->barrier_counter <= sync_point.debug_only) {
			bool is_ok = ::memcmp(it->dst, it->src, it->bytes) == 0;
			hxassertrelease(is_ok, "dma corrupt %s, %s", it->label_string_literal, (label_string_literal ? label_string_literal : "dma await"));
			s_hxdma_debug_records.erase_unordered(it);
		}
	}
#endif
}

void hxdma_await_all_labeled(const char* label_string_literal) {
	hxdma_sync_point b;
	hxdma_add_sync_point(b);
	hxdma_await_sync_point_labeled(b, label_string_literal);
#if HX_DEBUG_DMA
	HX_DMA_DEBUG_MUTEX_LOCK;
	hxassertrelease(s_hxdma_debug_records.empty(), "dma await failed %s",
		(label_string_literal ? label_string_literal : ""));
#endif
}

#endif // __EMSCRIPTEN__
