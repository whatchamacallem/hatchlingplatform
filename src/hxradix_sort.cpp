// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hxradix_sort.hpp"
#include "../include/hx/hxsort.hpp"

HX_REGISTER_FILENAME_HASH

// An unsigned integer used for the histogram. uint32_t or size_t would be a
// reasonable choice here.
using hxhistogram_t = uint32_t;

void hxradix_sort_void(hxradix_sort_key_void* begin, hxradix_sort_key_void* end) {
	// Check for size overflowing hxhistogram_t.
	hxassertmsg((size_t)(end - begin) < ~(hxhistogram_t)0, "hxradix_sort_void Too big.");

	const hxhistogram_t size = (hxhistogram_t)(end - begin);
	if(size < HX_RADIX_SORT_MIN_SIZE) {
		hxinsertion_sort(begin, end);
		return;
	}

	hxsystem_allocator_scope allocator_scope(hxsystem_allocator_temporary_stack);

	// Two working buffers.
	hxradix_sort_key_void* hxrestrict buf0 = begin;
	hxradix_sort_key_void* buf0End = buf0 + size;
	hxradix_sort_key_void* hxrestrict buf1 = (hxradix_sort_key_void*)hxmalloc(size * sizeof(hxradix_sort_key_void));
	hxradix_sort_key_void* buf1End = buf1 + size;

	hxhistogram_t* histograms = (hxhistogram_t*)hxmalloc(256u * 4u * sizeof(hxhistogram_t));
	::memset(histograms, 0x00, 256u * 4u * sizeof(hxhistogram_t)); // 4-8k

	// Build histograms
	hxhistogram_t* hxrestrict hist0 = histograms + (256 * 0);
	hxhistogram_t* hxrestrict hist1 = histograms + (256 * 1);
	hxhistogram_t* hxrestrict hist2 = histograms + (256 * 2);
	hxhistogram_t* hxrestrict hist3 = histograms + (256 * 3);

	for(const hxradix_sort_key_void* hxrestrict it = buf0; it != buf0End; ++it) {
		const hxhistogram_t x = it->get_modified_key();
		++hist0[x & 0xffu];
		++hist1[(x >> 8) & 0xffu];
		++hist2[(x >> 16) & 0xffu];
		++hist3[x >> 24];
	}

	// Convert histograms to start indices
	hxhistogram_t sum0 = 0u, sum1 = 0u, sum2 = 0u, sum3 = 0u;
	for(hxhistogram_t i = 0u; i < 256u; ++i) {
		hxhistogram_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
		hxhistogram_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
		hxhistogram_t t2 = hist2[i] + sum2; hist2[i] = sum2; sum2 = t2;
		hxhistogram_t t3 = hist3[i] + sum3; hist3[i] = sum3; sum3 = t3;
	}

	// 2 or 4 pass radix sort.
	for(const hxradix_sort_key_void* hxrestrict it = buf0; it != buf0End; ++it) {
		buf1[hist0[it->get_modified_key() & 0xffu]++] = *it;
	}
	for(const hxradix_sort_key_void* hxrestrict it = buf1; it != buf1End; ++it) {
		buf0[hist1[(it->get_modified_key() >> 8) & 0xffu]++] = *it;
	}
	if(hist2[1] != size || hist3[1] != size) {
		for(const hxradix_sort_key_void* hxrestrict it = buf0; it != buf0End; ++it) {
			buf1[hist2[(it->get_modified_key() >> 16) & 0xffu]++] = *it;
		}
		for(const hxradix_sort_key_void* hxrestrict it = buf1; it != buf1End; ++it) {
			buf0[hist3[it->get_modified_key() >> 24]++] = *it;
		}
	}

	hxfree(histograms);
	hxfree(buf1);
}

void hxradix_sort_void11(hxradix_sort_key_void* begin, hxradix_sort_key_void* end) {
	// Check for size overflowing hxhistogram_t.
	hxassertmsg((size_t)(end - begin) < ~(hxhistogram_t)0, "hxradix_sort_void Too big.");

	const hxhistogram_t size = (hxhistogram_t)(end - begin);
	if(size < HX_RADIX_SORT_MIN_SIZE) {
		hxinsertion_sort(begin, end);
		return;
	}

	hxsystem_allocator_scope allocator_scope(hxsystem_allocator_temporary_stack);

	// Three working buffers for extremely large data sets.
	hxradix_sort_key_void* hxrestrict buf0 = begin;
	hxradix_sort_key_void* buf0End = buf0 + size;
	hxradix_sort_key_void* hxrestrict buf1 = (hxradix_sort_key_void*)hxmalloc(size * sizeof(hxradix_sort_key_void) * 2u);
	hxradix_sort_key_void* buf1End = buf1 + size;
	hxradix_sort_key_void* buf2 = buf1End;
	hxradix_sort_key_void* buf2End = buf2 + size;

	hxhistogram_t* histograms = (hxhistogram_t*)hxmalloc(5120u * sizeof(hxhistogram_t)); // 5120: 2048*2.5
	::memset(histograms, 0x00, 5120u * sizeof(hxhistogram_t));

	hxhistogram_t* hxrestrict hist0 = histograms +	0u; // 2048 values
	hxhistogram_t* hxrestrict hist1 = histograms + 2048u; // 2048 values
	hxhistogram_t* hxrestrict hist2 = histograms + 4096u; // 1024 values

	for(const hxradix_sort_key_void* hxrestrict it = buf0; it != buf0End; ++it) {
		const hxhistogram_t x = it->get_modified_key();
		++hist0[x & 0x7ffu];
		++hist1[(x >> 11) & 0x7ffu];
		++hist2[x >> 22];
	}

	// Convert histograms to start indices
	hxhistogram_t sum0 = 0u, sum1 = 0u, sum2 = 0u;
	for(hxhistogram_t i = 0u; i < 1024u; ++i) {
		hxhistogram_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
		hxhistogram_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
		hxhistogram_t t2 = hist2[i] + sum2; hist2[i] = sum2; sum2 = t2;
	}
	for(hxhistogram_t i = 1024u; i < 2048u; ++i) {
		hxhistogram_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
		hxhistogram_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
	}

	// 2 or 3 pass radix sort
	for(const hxradix_sort_key_void* hxrestrict it = buf0; it != buf0End; ++it) {
		buf1[hist0[it->get_modified_key() & 0x7ffu]++] = *it;
	}
	hxradix_sort_key_void* hxrestrict buf20 = (hist2[1] != size) ? buf2 : buf0;
	for(const hxradix_sort_key_void* hxrestrict it = buf1; it != buf1End; ++it) {
		buf20[hist1[(it->get_modified_key() >> 11) & 0x7ffu]++] = *it;
	}
	if(hist2[1] != size) {
		for(const hxradix_sort_key_void* hxrestrict it = buf2; it != buf2End; ++it) {
			buf0[hist2[it->get_modified_key() >> 22]++] = *it;
		}
	}

	hxfree(histograms);
	hxfree(buf1);
}
