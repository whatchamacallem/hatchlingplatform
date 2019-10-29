// Copyright 2017-2019 Adrian Johnston

#include <hx/hxSort.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxRadixSortBase

HX_STATIC_ASSERT(HX_RADIX_SORT_BITS == 8 || HX_RADIX_SORT_BITS == 11, "Unsupported HX_RADIX_SORT_BITS");

void hxRadixSortBase::sort(hxMemoryManagerId tempMemory) {
	if (m_array.size() < HX_RADIX_SORT_MIN_SIZE) {
		hxInsertionSort(m_array.begin(), m_array.end());
		return;
	}

	hxMemoryManagerScope allocatorScope(tempMemory);

	if (HX_RADIX_SORT_BITS == 8) {
		// 2 Working buffers
		KeyValuePair* HX_RESTRICT buf0 = m_array.data();
		KeyValuePair* buf0End = buf0 + m_array.size();
		KeyValuePair* HX_RESTRICT buf1 = (KeyValuePair*)hxMalloc(m_array.size() * sizeof(KeyValuePair));
		KeyValuePair* buf1End = buf1 + m_array.size();

		uint32_t* histograms = (uint32_t*)hxMalloc(256u * 4u * sizeof(uint32_t));
		::memset(histograms, 0x00, 256u * 4u * sizeof(uint32_t)); // 4k

		// Build histograms
		uint32_t* HX_RESTRICT hist0 = histograms + (256 * 0);
		uint32_t* HX_RESTRICT hist1 = histograms + (256 * 1);
		uint32_t* HX_RESTRICT hist2 = histograms + (256 * 2);
		uint32_t* HX_RESTRICT hist3 = histograms + (256 * 3);

		for (const KeyValuePair* HX_RESTRICT it = buf0; it != buf0End; ++it) {
			uint32_t x = it->m_key;
			++hist0[x & 0xffu];
			++hist1[(x >> 8) & 0xffu];
			++hist2[(x >> 16) & 0xffu];
			++hist3[x >> 24];
		}

		// Convert histograms to start indices
		uint32_t sum0 = 0u, sum1 = 0u, sum2 = 0u, sum3 = 0u;
		for (uint32_t i = 0u; i < 256u; ++i) {
			uint32_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
			uint32_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
			uint32_t t2 = hist2[i] + sum2; hist2[i] = sum2; sum2 = t2;
			uint32_t t3 = hist3[i] + sum3; hist3[i] = sum3; sum3 = t3;
		}

		// 2 or 4 pass radix sort
		for (const KeyValuePair* HX_RESTRICT it = buf0; it != buf0End; ++it) {
			buf1[hist0[it->m_key & 0xffu]++] = *it;
		}
		for (const KeyValuePair* HX_RESTRICT it = buf1; it != buf1End; ++it) {
			buf0[hist1[(it->m_key >> 8) & 0xffu]++] = *it;
		}
		if (hist2[1] != m_array.size() || hist3[1] != m_array.size()) {
			for (const KeyValuePair* HX_RESTRICT it = buf0; it != buf0End; ++it) {
				buf1[hist2[(it->m_key >> 16) & 0xffu]++] = *it;
			}
			for (const KeyValuePair* HX_RESTRICT it = buf1; it != buf1End; ++it) {
				buf0[hist3[it->m_key >> 24]++] = *it;
			}
		}

		hxFree(histograms);
		hxFree(buf1);
	}
	else if (HX_RADIX_SORT_BITS == 11) {
		// 3 Working buffers
		KeyValuePair* HX_RESTRICT buf0 = m_array.data();
		KeyValuePair* buf0End = buf0 + m_array.size();
		KeyValuePair* HX_RESTRICT buf1 = (KeyValuePair*)hxMalloc(m_array.size() * sizeof(KeyValuePair) * 2u);
		KeyValuePair* buf1End = buf1 + m_array.size();
		KeyValuePair* buf2 = buf1End;
		KeyValuePair* buf2End = buf2 + m_array.size();

		uint32_t* histograms = (uint32_t*)hxMalloc(5120u * sizeof(uint32_t)); // 5120: 2048*2.5
		::memset(histograms, 0x00, 5120u * sizeof(uint32_t));

		uint32_t* HX_RESTRICT hist0 = histograms +    0u; // 2048 values
		uint32_t* HX_RESTRICT hist1 = histograms + 2048u; // 2048 values
		uint32_t* HX_RESTRICT hist2 = histograms + 4096u; // 1024 values

		for (const KeyValuePair* HX_RESTRICT it = buf0; it != buf0End; ++it) {
			uint32_t x = it->m_key;
			++hist0[x & 0x7ffu];
			++hist1[(x >> 11) & 0x7ffu];
			++hist2[x >> 22];
		}

		// Convert histograms to start indices
		uint32_t sum0 = 0u, sum1 = 0u, sum2 = 0u;
		for (uint32_t i = 0u; i < 1024u; ++i) {
			uint32_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
			uint32_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
			uint32_t t2 = hist2[i] + sum2; hist2[i] = sum2; sum2 = t2;
		}
		for (uint32_t i = 1024u; i < 2048u; ++i) {
			uint32_t t0 = hist0[i] + sum0; hist0[i] = sum0; sum0 = t0;
			uint32_t t1 = hist1[i] + sum1; hist1[i] = sum1; sum1 = t1;
		}

		// 2 or 3 pass radix sort
		for (const KeyValuePair* HX_RESTRICT it = buf0; it != buf0End; ++it) {
			buf1[hist0[it->m_key & 0x7ffu]++] = *it;
		}
		KeyValuePair* HX_RESTRICT buf20 = (hist2[1] != m_array.size()) ? buf2 : buf0;
		for (const KeyValuePair* HX_RESTRICT it = buf1; it != buf1End; ++it) {
			buf20[hist1[(it->m_key >> 11) & 0x7ffu]++] = *it;
		}
		if (hist2[1] != m_array.size()) {
			for (const KeyValuePair* HX_RESTRICT it = buf2; it != buf2End; ++it) {
				buf0[hist2[it->m_key >> 22]++] = *it;
			}
		}

		hxFree(histograms);
		hxFree(buf1);
	}
}
