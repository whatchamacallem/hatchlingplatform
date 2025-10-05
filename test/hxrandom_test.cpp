// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxrandom.hpp>
#include <hx/hxtest.hpp>
#include <hx/hxarray.hpp>

HX_REGISTER_FILENAME_HASH

TEST(hxrandom_test, generation) {
	hxrandom rng(1u);

	// implicit casts and assignment constructors
	uint8_t uint8 = rng;
	uint16_t uint16 = rng;
	uint32_t uint32 = rng;
	uint64_t uint64 = rng;

	// functor calls and assignment operators
	uint8 = rng();
	uint16 = rng();
	uint32 = rng();
	uint64 = rng();

	(void)uint8; (void)uint16; (void)uint32; (void)uint64;

	for(int s=10; s--;) {

		// implicit casts and assignment constructors
		float f = rng;
		float d = rng;

		// functor calls and assignment operators
		f = rng();
		d = rng();

		(void)uint8; (void)uint16; (void)uint32; (void)uint64;

		// The odds of hitting zero in the first 20 numbers should be zero.
		EXPECT_TRUE(f > 0.0f && f < 1.0f);
		EXPECT_TRUE(d > 0.0 && d < 1.0);
	}
}

TEST(hxrandom_test, ops) {
	hxrandom rng(20000);
	for(int s=100; s--;) {
		// T &= rng
		int i = 255; i &= rng;
		EXPECT_TRUE(i >= 0 && i < 256);

		unsigned int u = 255; u &= rng;
		EXPECT_TRUE(u < 256);

		char c = 'x'; c &= rng;
		EXPECT_TRUE((c&~(unsigned char)'x') == (unsigned char)'\0');

		// floating point mod.
		float f = rng % 255.0f;
		EXPECT_TRUE(f >= 0.0f && f < 255.0f);
		double d = rng % 255.0;
		EXPECT_TRUE(d >= 0.0 && d < 255.0);

		{
			int r = 255 & rng;
			EXPECT_TRUE(r >= 0 && r < 256);

			int l = rng & 255;
			EXPECT_TRUE(l >= 0 && l < 256);

			int m = rng % 255;
			EXPECT_TRUE(m >= 0 && m < 255);
		}
		{
			unsigned short r = (unsigned short)255 & rng;
			EXPECT_TRUE(r < (unsigned short)256);

			unsigned short l = rng & (unsigned short)255;
			EXPECT_TRUE(l < (unsigned short)256);

			unsigned short m = rng % (unsigned short)255;
			EXPECT_TRUE(m < (unsigned short)255);
		}
		{
			int64_t r = 255ll & rng;
			EXPECT_TRUE(r >= 0ll && r < 256ll);

			int64_t l = rng & 255ll;
			EXPECT_TRUE(l >= 0ll && l < 256ll);

			int64_t m = rng % 255ll;
			EXPECT_TRUE(m >= 0ll && m < 255ll);
		}
		{
			uint64_t r = 255ull & rng;
			EXPECT_TRUE(r < 256ull);

			uint64_t l = rng & 255ull;
			EXPECT_TRUE(l < 256ull);

			uint64_t m = rng % 255ull;
			EXPECT_TRUE(m < 255ull);
		}

		// Check a different modulo.
		EXPECT_TRUE((rng%100) >= 0 && (rng()%100) < 100);
		EXPECT_TRUE((rng%100.0f) >= 0.0f && (rng()%100.0f) < 100.0f);
		EXPECT_TRUE((rng%100.0) >= 0.0 && (rng()%100.0) < 100.0);
		EXPECT_TRUE((rng()%100u) < 100u);
		EXPECT_TRUE((rng%100l) >= 0l && (rng()%100l) < 100l);
		EXPECT_TRUE((rng()%100ul) < 100ul);
		EXPECT_TRUE((rng%100ll) >= 0ll && (rng()%100ll) < 100ll);
		EXPECT_TRUE((rng()%100ull) < 100ull);

		// Check that the rng isn't just spitting out zeros.
		EXPECT_TRUE((uint32_t)rng() | (uint32_t)rng());
	}
}

TEST(hxrandom_test, read_populates_buffer) {
	hxrandom rng(0x654321u);

	uint8_t buffer[] = {
		0xefu, 0xefu, 0xefu, 0xefu,
		0xefu, 0xefu, 0xefu, 0xefu,
		0xefu
	};
	const size_t size = sizeof buffer / sizeof *buffer;
	const size_t read_count = size - 2; // 7. Intentionally odd.

	rng.read(buffer, read_count);

	hxrandom verifier(0x654321u);
	size_t remaining = read_count;
	const uint8_t* expected = buffer;

	// This just documents an expected interface and sequence.
	while(remaining >= 4) {
		const uint32_t x = verifier.generate32();
		EXPECT_EQ(*expected++, (uint8_t)x);
		EXPECT_EQ(*expected++, (uint8_t)(x >> 8));
		EXPECT_EQ(*expected++, (uint8_t)(x >> 16));
		EXPECT_EQ(*expected++, (uint8_t)(x >> 24));
		remaining -= 4;
	}
	if(remaining) {
		uint32_t x = verifier.generate32();
		do {
			EXPECT_EQ(*expected++, (uint8_t)x);
			x >>= 8;
		} while(--remaining)
			/**/;
	}

	// Check for a tail overwrite.
	for(size_t i = read_count; i < size; ++i) {
		EXPECT_EQ(buffer[i], 0xefu);
	}
}

TEST(hxrandom_test, range) {
	hxrandom rng(30000);
	for(int s=100; s--;) {
		EXPECT_TRUE(rng.range('a', (char)10) >= 'a' && rng.range('a', (char)10) < (char)('a' + 10));
		EXPECT_TRUE(rng.range(1000,100) >= 1000 && rng.range(1000,100) < 1100);
		EXPECT_TRUE(rng.range(1000u,100u) >= 1000u && rng.range(1000u,100u) < 1100u);
		EXPECT_TRUE(rng.range(1000l,100l) >= 1000l && rng.range(1000l,100l) < 1100l);
		EXPECT_TRUE(rng.range(1000ul,100ul) >= 1000ul && rng.range(1000ul,100ul) < 1100ul);
		EXPECT_TRUE(rng.range(1000ll,100ll) >= 1000ll && rng.range(1000ll,100ll) < 1100ll);
		EXPECT_TRUE(rng.range(1000ull,100ull) >= 1000ull && rng.range(1000ull,100ull) < 1100ull);
		EXPECT_TRUE(rng.range(1000.0f,100.0f) >= 1000.0f && rng.range(1000.0f,100.0f) < 1100.0f);
		EXPECT_TRUE(rng.range(1000.0,100.0) >= 1000.0 && rng.range(1000.0,100.0) < 1100.0);

		// Check that the rng isn't just spitting out zeros.
		EXPECT_TRUE((uint32_t)rng() | (uint32_t)rng());
	}
}

TEST(hxrandom_test, histogram) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(40000);
	const int buckets = 1 << 10; // 1k buckets
	const int iters = 1000;
	const int max = 1100; // 10% above average max.
	hxarray<int> hist(buckets, 0);

	for(int i=(buckets*iters); i--;) {
		// Doesn't require an unsigned type for %. No floating point used.
		++hist[(size_t)(rng() % (buckets - 1))];
	}
	for(size_t i=buckets; i--;) {
		EXPECT_LE(hist[i], max);
	}
}

TEST(hxrandom_test, histogram_f) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxrandom rng(40000);
	const int buckets = 1000; // 1k buckets
	const int iters = 1000;
	const int max = 1150; // 15% above average max.
	hxarray<int> hist(buckets, 0);

	for(int i=(buckets*iters); i--;) {
		// Run the full 64-bit bit double pipeline.
		++hist[(size_t)(rng() % (double)buckets)];
	}
	for(size_t i=buckets; i--;) {
		EXPECT_LE(hist[i], max);
	}
}
