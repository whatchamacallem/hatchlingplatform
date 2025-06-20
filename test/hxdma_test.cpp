// Copyright 2017-2025 Adrian Johnston

// No DMA in a web browser.  The DMA code is just scaffolding.
#ifndef __EMSCRIPTEN__

#include <hx/hxdma.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

class hxdma_test :
	public testing::Test
{
public:
    hxdma_test() {
        ::memset(m_buf_ + 0, 0x00, sizeof m_buf_);
        set_buf();
    }

	~hxdma_test() {
		hxdma_await_all("end test");
		hxdma_end_frame();
		check_buf(); // Don't trash test buffer
	}

	void set_buf(uint8_t* buf=hxnull) {
		buf = buf ? buf : m_buf_;
		for (size_t i = Buf_size; i--;) {
			buf[i] = (uint8_t)i;
		}
	}

	void check_buf(const uint8_t* buf = 0) const {
		buf = buf ? buf : m_buf_;
		for (size_t i = Buf_size; i--;) {
			ASSERT_EQ(buf[i], (uint8_t)i);
		}
	}

    static const size_t Buf_size = 100;
    uint8_t m_buf_[Buf_size];
};

TEST_F(hxdma_test, Single) {
	uint8_t dst[Buf_size];
	::memset(dst, 0x33, sizeof dst);
	hxdma_start(dst, m_buf_, Buf_size, "start");
	hxdma_await_all("await");
	check_buf(dst);
}

TEST_F(hxdma_test, Multiple) {
	static const size_t OPS = 3u;
	uint8_t dst[OPS][Buf_size];

	::memset(dst, 0x33, sizeof dst);
	for (size_t i = OPS; i--;) {
		hxdma_start(dst[i], m_buf_, Buf_size, "start");
	}
	hxdma_await_all("await");
	for (size_t i = OPS; i--;) {
		check_buf(dst[i]);
	}
}

TEST_F(hxdma_test, Simultaneous) {
	static const size_t OPS = 3u;
	static const size_t REPS = 4u;
	uint8_t dst[OPS][Buf_size];
	hxdma_sync_point sp[OPS];

	for (size_t j = REPS; j--;) {
		::memset(dst, 0x33, sizeof dst);
		for (size_t i = OPS; i--;) {
			hxdma_start(dst[i], m_buf_, Buf_size, "start");
			hxdma_add_sync_point(sp[i]);
		}
		for (size_t i = OPS; i--;) {
			hxdma_await_sync_point(sp[i], "sync point");
			check_buf(dst[i]);
		}
	}
}

#endif // __EMSCRIPTEN__
