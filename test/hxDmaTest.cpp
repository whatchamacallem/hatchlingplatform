// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hxDma.h>
#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------

class hxDmaTest :
	public testing::Test
{
public:
	hxDmaTest() {
		::memset(m_buf + 0, 0x00, sizeof m_buf);
		setBuf();
	}

	~hxDmaTest() {
		hxDmaAwaitAll("end test");
		hxDmaEndFrame();
		checkBuf(); // Don't trash test buffer
	}

	void setBuf(uint8_t* buf=hxnull) {
		buf = buf ? buf : m_buf;
		for (size_t i = BufSize; i--;) {
			buf[i] = (uint8_t)i;
		}
	}

	void checkBuf(const uint8_t* buf = 0) const {
		buf = buf ? buf : m_buf;
		for (size_t i = BufSize; i--;) {
			ASSERT_EQ(buf[i], (uint8_t)i);
		}
	}

	static const size_t BufSize = 100;
	uint8_t m_buf[BufSize];
};

// ----------------------------------------------------------------------------

TEST_F(hxDmaTest, Single) {
	uint8_t dst[BufSize];
	::memset(dst, 0x33, sizeof dst);
	hxDmaStart(dst, m_buf, BufSize, "start");
	hxDmaAwaitAll("await");
	checkBuf(dst);
}

TEST_F(hxDmaTest, Multiple) {
	static const size_t OPS = 3u;
	uint8_t dst[OPS][BufSize];

	::memset(dst, 0x33, sizeof dst);
	for (size_t i = OPS; i--;) {
		hxDmaStart(dst[i], m_buf, BufSize, "start");
	}
	hxDmaAwaitAll("await");
	for (size_t i = OPS; i--;) {
		checkBuf(dst[i]);
	}
}

TEST_F(hxDmaTest, Simultaneous) {
	static const size_t OPS = 3u;
	static const size_t REPS = 4u;
	uint8_t dst[OPS][BufSize];
	hxDmaSyncPoint sp[OPS];

	for (size_t j = REPS; j--;) {
		::memset(dst, 0x33, sizeof dst);
		for (size_t i = OPS; i--;) {
			hxDmaStart(dst[i], m_buf, BufSize, "start");
			hxDmaAddSyncPoint(sp[i]);
		}
		for (size_t i = OPS; i--;) {
			hxDmaAwaitSyncPoint(sp[i], "sync point");
			checkBuf(dst[i]);
		}
	}
}




