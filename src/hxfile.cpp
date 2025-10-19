// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// This is the C <stdio.h> version using FILE*. Use an alternate .cpp file for
// alternate implementations.

#include "../include/hx/hxfile.hpp"

// These are only dependencies of the Hatchling Platform here. This is to allow
// reimplementation.
#include <stdio.h>
#include <errno.h>

#if defined _MSC_VER
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// hxfile - Targets require an implementation of fopen(), fclose(), fread(),
// fwrite(), fgets(), and feof().

hxfile hxin(stdin, hxfile::in);
hxfile hxout(stdout, hxfile::out);
#ifndef __wasm__
hxfile hxerr(stderr, hxfile::out);
#else
// Don't use stdout with the default index.js provided by the emsdk.
hxfile hxerr(stdout, hxfile::out);
#endif
hxfile hxdev_null((void*)0, hxfile::out);

// In this version the file is a FILE*.
hxfile::hxfile(void* file_, uint8_t mode) : hxfile() {
	m_file_pimpl_ = file_; // does not own.
	m_open_mode_ = mode;
}

hxfile::hxfile(uint8_t mode, const char* filename, ...) : hxfile() {
	va_list args;
	va_start(args, filename);
	openv_(mode, filename, args);
	va_end(args);
}

hxfile::~hxfile(void) {
	close();
}

void hxfile::operator=(hxfile&& file_) {
	close();
	::memcpy((void*)this, &file_, sizeof file_);
	::memset((void*)&file_, 0x00, sizeof file_);
}

bool hxfile::open(uint8_t mode, const char* filename, ...) {
	close(); // openv_ assumes the file is closed.

	va_list args;
	va_start(args, filename);
	bool rv = openv_(mode, filename, args);
	va_end(args);
	return rv;
}

bool hxfile::openv_(uint8_t mode, const char* filename, va_list args) {
	hxassert(m_file_pimpl_ == hxnull);

	m_open_mode_ = mode; // Record mode regardless.

	const char* m = hxnull;
	switch ((int)mode & (hxfile::in | hxfile::out)) {
	case hxfile::none:
		return false; // May assert if used.
	case hxfile::in:
		m = "rb";
		break;
	case hxfile::out:
		m = "wb";
		break;
	default:
		m = "w+b";
	}

	char buf[HX_MAX_LINE];
	const int len = ::vsnprintf(buf, HX_MAX_LINE, filename, args);
	hxassertmsg(len >= 0 && len < HX_MAX_LINE, "vsnprintf"); (void)len;

	m_file_pimpl_ = ::fopen(buf, m);
	hxassertrelease((m_file_pimpl_ != hxnull) || ((mode & hxfile::skip_asserts) != 0u),
		"fopen %s %s: %s", buf, m, ::strerror(errno));

	m_fail_ = (m_file_pimpl_ == hxnull);
	m_owns_ = !m_fail_;
	return !m_fail_;
}

void hxfile::close(void) {
	if(m_owns_) {
		int code = ::fclose((FILE*)m_file_pimpl_);
		hxassertmsg(code == 0, "fclose"); (void)code;
	}
	::memset((void*)this, 0x00, sizeof *this);
}

void hxfile::clear(void) {
	m_fail_ = false;
	m_eof_ = false;
	if(m_file_pimpl_ != hxnull) {
		::clearerr((FILE*)m_file_pimpl_);
	}
}

size_t hxfile::get_pos(void) const {
	hxassertmsg(m_file_pimpl_ != hxnull, "invalid_file");
	// Requires a 64-bit long to support 64-bit files.
	return (size_t)::ftell((FILE*)m_file_pimpl_);
}

bool hxfile::set_pos(size_t position_) {
	hxassertmsg(m_file_pimpl_ != hxnull, "invalid_file");
	// Requires a 64-bit long to support 64-bit files.
	m_fail_ = ::fseek((FILE*)m_file_pimpl_, (long)position_, 0) != 0;
	m_eof_ = m_fail_;
	return !m_fail_;
}

size_t hxfile::read(void* bytes, size_t byte_count) {
	hxassertmsg(((m_open_mode_ & hxfile::in) != 0u) && (m_file_pimpl_ != hxnull), "invalid_file");

	const size_t bytes_read = ::fread(bytes, 1, byte_count, (FILE*)m_file_pimpl_);

	hxassertmsg((byte_count == bytes_read) || ((m_open_mode_ & hxfile::skip_asserts) != 0u),
		"fread expected %zu != actual %zu: %s", byte_count, bytes_read, ::strerror(errno));

	if(byte_count != bytes_read) {
		m_fail_ = true;
		m_eof_ = (::feof((FILE*)m_file_pimpl_) != 0);
	}
	return bytes_read;
}

size_t hxfile::write(const void* bytes, size_t byte_count) {
	hxassertmsg((m_open_mode_ & hxfile::out) != 0u, "invalid_file");

	if(m_file_pimpl_ == hxnull) {
		// Writing to null emulates /dev/null support.
		return byte_count;
	}
	const size_t bytes_written = ::fwrite(bytes, 1, byte_count, (FILE*)m_file_pimpl_);

	hxassertmsg((byte_count == bytes_written) || ((m_open_mode_ & hxfile::skip_asserts) != 0u),
		"fwrite expected %zu != actual %zu: %s", byte_count, bytes_written, ::strerror(errno));

	// Can restore goodness.
	m_fail_ = byte_count != bytes_written;
	return bytes_written;
}

bool hxfile::flush(void) {
	hxassertmsg((m_open_mode_ & hxfile::out) != 0u, "invalid_file");
	if(m_file_pimpl_ == hxnull) {
		return true;
	}

	const int result = ::fflush((FILE*)m_file_pimpl_);
	hxassertmsg((result == 0) || ((m_open_mode_ & hxfile::skip_asserts) != 0u),
		"fflush %s", ::strerror(errno));
	return result == 0;
}

bool hxfile::getline(char* buffer, int buffer_size) {
	hxassertmsg(((m_open_mode_ & hxfile::in) != 0u) && (m_file_pimpl_ != hxnull), "invalid_file");

	char* result = ::fgets(buffer, buffer_size, (FILE*)m_file_pimpl_);

	hxassertmsg(!::ferror((FILE*)m_file_pimpl_), "fgets %s", ::strerror(errno));

	if(result == hxnull) {
		m_fail_ = true;
		m_eof_ = (::feof((FILE*)m_file_pimpl_) != 0); // 0: not past end.
		return false; // EOF or error.
	}
	return true;
}

// See vsnprintf to reimplement this without FILE* support.
bool hxfile::print(const char* format, ...) {
	hxassertmsg((m_open_mode_ & hxfile::out) != 0u, "invalid_file");

	if(m_file_pimpl_ == hxnull) {
		// Writing to null emulates /dev/null support.
		return true;
	}

	va_list args;
	va_start(args, format);
	const int len = ::vfprintf((FILE*)m_file_pimpl_, format, args);
	va_end(args);

	hxassertrelease(len >= 0, "vfprintf %s", ::strerror(errno));
	return len >= 0;
}

// See vscanf to reimplement this without FILE* support.
int hxfile::scan(const char* format, ...) {
	hxassertmsg(((m_open_mode_ & hxfile::in) != 0u) && (m_file_pimpl_ != hxnull), "invalid_file");
	va_list args;
	va_start(args, format);
	const int items_scanned = ::vfscanf((FILE*)m_file_pimpl_, format, args);
	va_end(args);

	hxassertrelease(items_scanned != EOF || ((m_open_mode_ & hxfile::skip_asserts) != 0u), "vfscanf %s", ::strerror(errno));

	if(items_scanned == EOF) {
		m_fail_ = true;
		m_eof_ = (::feof((FILE*)m_file_pimpl_) != 0);
	}
	return items_scanned;
}
