// Copyright 2017-2025 Adrian Johnston

#include <hx/hxRandom.hpp>
#include <hx/hxTest.hpp>
#include <hx/hxArray.hpp>

HX_REGISTER_FILENAME_HASH

TEST(hxRandomTest, Generation) {
	hxRandom rng;
	for(int s=100; s--;) {

		// implicit casts and copy constructors
		uint8_t uint8 = rng;
		uint16_t uint16 = rng;
		uint32_t uint32 = rng;
		uint64_t uint64 = rng;
		float f = rng;
		float d = rng;

		// functor calls and assignment operators
		uint8 = rng();
		uint16 = rng();
		uint32 = rng();
		uint64 = rng();

		f = rng();
		d = rng();

		(void)uint8; (void)uint16; (void)uint32; (void)uint64;

		ASSERT_TRUE(f >= 0.0f && f < 1.0f);
		ASSERT_TRUE(d >= 0.0 && d < 1.0);

		// Check that the rng isn't just spitting out zeros.
		ASSERT_TRUE((uint32_t)rng() | (uint32_t)rng());
	}
}

TEST(hxRandomTest, Ops) {
	hxRandom rng(20000);
	for(int s=100; s--;) {
		// T &= rng
		int i = 255; i &= rng;
		ASSERT_TRUE(i >= 0 && i < 256);

		unsigned int u = 255; u &= rng;
		ASSERT_TRUE(u < 256);

		char c = 'x'; c &= rng;
		ASSERT_TRUE((c&~(unsigned char)'x') == (unsigned char)'\0');

		// floating point mod.
		float f = rng % 255.0f;
		ASSERT_TRUE(f >= 0.0f && f < 255.0f);
		float d = rng % 255.0;
		ASSERT_TRUE(d >= 0.0 && d < 255.0);

		{
			int r = 255 & rng;
			ASSERT_TRUE(r >= 0 && r < 256);

			int l = rng & 255;
			ASSERT_TRUE(l >= 0 && l < 256);

			int m = rng % 255;
			ASSERT_TRUE(m >= 0 && m < 255);
		}
		{
			unsigned short r = (unsigned short)255 & rng;
			ASSERT_TRUE(r < (unsigned short)256);

			unsigned short l = rng & (unsigned short)255;
			ASSERT_TRUE(l < (unsigned short)256);

			unsigned short m = rng % (unsigned short)255;
			ASSERT_TRUE(m < (unsigned short)255);
		}
		{
			int64_t r = 255ll & rng;
			ASSERT_TRUE(r >= 0ll && r < 256ll);

			int64_t l = rng & 255ll;
			ASSERT_TRUE(l >= 0ll && l < 256ll);

			int64_t m = rng % 255ll;
			ASSERT_TRUE(m >= 0ll && m < 255ll);
		}
		{
			uint64_t r = 255ull & rng;
			ASSERT_TRUE(r < 256ull);

			uint64_t l = rng & 255ull;
			ASSERT_TRUE(l < 256ull);

			uint64_t m = rng % 255ull;
			ASSERT_TRUE(m < 255ull);
		}

		// Check a different modulo.
		ASSERT_TRUE((rng%100) >= 0 && (rng()%100) < 100);
		ASSERT_TRUE((rng%100.0f) >= 0.0f && (rng()%100.0f) < 100.0f);
		ASSERT_TRUE((rng%100.0) >= 0.0 && (rng()%100.0) < 100.0);
		ASSERT_TRUE((rng()%100u) < 100u);
		ASSERT_TRUE((rng%100l) >= 0l && (rng()%100l) < 100l);
		ASSERT_TRUE((rng()%100ul) < 100ul);
		ASSERT_TRUE((rng%100ll) >= 0ll && (rng()%100ll) < 100ll);
		ASSERT_TRUE((rng()%100ull) < 100ull);

		// Check that the rng isn't just spitting out zeros.
		ASSERT_TRUE((uint32_t)rng() | (uint32_t)rng());
	}
}

TEST(hxRandomTest, Range) {
	hxRandom rng(30000);
	for(int s=100; s--;) {
		ASSERT_TRUE(rng.range('a', (char)10) >= 'a' && rng.range('a', (char)10) < (char)('a' + 10));
		ASSERT_TRUE(rng.range(1000,100) >= 1000 && rng.range(1000,100) < 1100);
		ASSERT_TRUE(rng.range(1000u,100u) >= 1000u && rng.range(1000u,100u) < 1100u);
		ASSERT_TRUE(rng.range(1000l,100l) >= 1000l && rng.range(1000l,100l) < 1100l);
		ASSERT_TRUE(rng.range(1000ul,100ul) >= 1000ul && rng.range(1000ul,100ul) < 1100ul);
		ASSERT_TRUE(rng.range(1000ll,100ll) >= 1000ll && rng.range(1000ll,100ll) < 1100ll);
		ASSERT_TRUE(rng.range(1000ull,100ull) >= 1000ull && rng.range(1000ull,100ull) < 1100ull);
		ASSERT_TRUE(rng.range(1000.0f,100.0f) >= 1000.0f && rng.range(1000.0f,100.0f) < 1100.0f);
		ASSERT_TRUE(rng.range(1000.0,100.0) >= 1000.0 && rng.range(1000.0,100.0) < 1100.0);

		// Check that the rng isn't just spitting out zeros.
		ASSERT_TRUE((uint32_t)rng() | (uint32_t)rng());
	}
}

TEST(hxRandomTest, Histogram) {
	hxRandom rng(40000);
	const int buckets = 1 << 10; // 1k buckets
	const int iters = 1000;
	const int max = 1100; // 10% above average max.
	hxArray<int> hist(buckets, 0);

	for(int i=(buckets*iters); i--;) {
		// Doesn't require an unsigned type here. No floating point used.
		++hist[rng() % (buckets - 1)];
	}
	int t=0;
	for(int i=buckets; i--;) {
		ASSERT_LE(hist[i], max);
		t = hxmax(t, hist[i]);
	}
}

TEST(hxRandomTest, HistogramF) {
	hxRandom rng(40000);
	const int buckets = 1000; // 1k buckets
	const int iters = 1000;
	const int max = 1150; // 15% above average max.
	hxArray<int> hist(buckets, 0);

	for(int i=(buckets*iters); i--;) {
		// Run the full 64-bit bit double pipeline.
		++hist[(int)(rng() % (double)buckets)];
	}
	int t=0;
	for(int i=buckets; i--;) {
		ASSERT_LE(hist[i], max);
		t = hxmax(t, hist[i]);
	}
}
