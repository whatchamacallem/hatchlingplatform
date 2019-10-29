#pragma once
// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxFile.h"

// ** EXPERIMENTAL **  This design has not seen use.  Feedback please.

// ----------------------------------------------------------------------------
// Mechanism for testing that different code runs identically.  It is not a
// long term testing solution, but a hack for debugging a reimplementation.
#ifndef HX_DETERMINISTIC_REPLAY
#define HX_DETERMINISTIC_REPLAY 1
#endif

#if HX_DETERMINISTIC_REPLAY

struct hxDetermineHeader {
	// First 4 bytes of little endian file are "epdr"
	hxDetermineHeader() {
		version = ('e' << 24) | ('p' << 16) | ('d' << 8) | 'r';
	}
	int32_t version;
	int32_t tick;
};

class hxDetermine {
public:
	hxDetermine() {
		m_enabled = false;
	}

	void reset() {
		m_enabled = false; // next tick will restart/reconfigure.
	}

	// Enable and step the deterministic replay system.  filename should include a single "%d"
	// format specifier to encode the frame number in the filename. 
	bool tick(const char* filename, bool replaying, int32_t warm_up = 0, int32_t max_ = 1) {
		if (m_enabled == false) {
			m_enabled = true;
			m_replaying = replaying;
			m_counter = -warm_up;
			m_max = max_;
		}

		m_log.close();

		if (m_counter >= m_max) {
			return false;
		}
		if (m_counter++ < 0) {
			return false;
		}

		hxLog((m_replaying ? "Deterministic Replay %d...\n" : "Deterministic Recording %d...\n"), (int)m_counter);
		m_log.open((m_replaying ? hxFile::in : hxFile::out), filename, (int)m_counter);

		hxDetermineHeader h;
		h.tick = m_counter;
		data(&h, sizeof h);

		return true;
	}

	// Records or restores inputs to system being tested.  Allows for use of matching test data that
	// could not easily be made avialable on the target platform.
	void input(void* data, int32_t size) {
		if (!m_log.good() || size == 0) {
			return;
		}
		if (m_replaying) {
			m_log.read(data, size);
		}
		else {
			m_log.write(data, size);
		}
	}

	// Encode or check a buffer in the replay.
	void data(const void* data, uint32_t size) {
		if (!m_log.good() || size == 0) {
			return;
		}
		if (m_replaying) {
			const char* d2 = (const char*)data;
			int8_t buf[hxDetermine::c_bufSize];
			while (size > 0u) {
				int32_t chunk = (size < hxDetermine::c_bufSize) ? size : hxDetermine::c_bufSize;

				m_log.read(buf, chunk);

				int32_t cmp = ::memcmp(d2, buf, chunk);
				hxAssertRelease(cmp == 0, "replay wrong"); (void)cmp;

				d2 += chunk;
				size -= chunk;
			}
		}
		else {
			m_log.write(data, size);
		}
	}

	// Encode or check a string literal in the replay.
	void label(const char* label) {
		data(label, (uint32_t)::strlen(label));
	}

	// Encode or check a fundamental type in the replay.
	template<typename T>
	void number(T val) {
		data(&val, sizeof val);
	}

	static hxDetermine& get() {
		static hxDetermine data;
		return data;
	}

private:
	static const int32_t c_bufSize = 1024;

	hxDetermine(const hxDetermine&); // = delete
	void operator=(const hxDetermine&); // = delete

	bool m_enabled;
	bool m_replaying;
	int32_t m_counter;
	int32_t m_max;
	hxFile m_log;
};

#define hxDetermineTick(...) hxDetermine::get().tick(__VA_ARGS__)
#define hxDetermineInput(...) hxDetermine::get().input(__VA_ARGS__)
#define hxDetermineData(...) hxDetermine::get().data(__VA_ARGS__)
#define hxDetermineLabel(...) hxDetermine::get().label(__VA_ARGS__)
#define hxDetermineNumber(...) hxDetermine::get().number(__VA_ARGS__)

#else // !HX_DETERMINISTIC_REPLAY

#define hxDetermineTick(...)
#define hxDetermineInput(...)
#define hxDetermineData(...)
#define hxDetermineLabel(...)
#define hxDetermineNumber(...)

#endif // !HX_DETERMINISTIC_REPLAY


