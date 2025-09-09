// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxfile.hpp>

#include <stdio.h>

#if defined _MSC_VER
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// hxfile - Target will require an implementation of fopen(), fclose(), fread(),
// fwrite(), fgets() and feof().

hxfile hxin(stdin, hxfile::in);
hxfile hxout(stdout, hxfile::out);
hxfile hxerr(stderr, hxfile::out);
hxfile hxdev_null(hxnull, hxfile::in | hxfile::out | hxfile::skip_asserts);

hxfile::hxfile(void) {
	m_owns_ = false;
	close();
}

// In this version the file is a FILE*.
hxfile::hxfile(void* file_, uint8_t mode) : hxfile() {
	m_open_mode_ = mode;
	if(file_) {
		m_file_pimpl_ = file_; // does not own.
		m_good_ = true;
	}
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

bool hxfile::open(uint8_t mode, const char* filename, ...) {
	close(); // openv_ assumes closed

	va_list args;
	va_start(args, filename);
	bool rv = openv_(mode, filename, args);
	va_end(args);
	return rv;
}

bool hxfile::openv_(uint8_t mode, const char* filename, va_list args) {
	hxassertmsg(m_file_pimpl_ == hxnull, "internal_error");

	m_open_mode_ = mode; // Record skip_asserts mode regardless.
	if(filename == hxnull) {
		return false;
	}

	char buf[HX_MAX_LINE];
	int len = ::vsnprintf(buf, HX_MAX_LINE, filename, args);
	hxassertmsg(len >= 0 && len < HX_MAX_LINE, "vsnprintf"); (void)len;

	const char* m = hxnull;
	switch (mode & (hxfile::in | hxfile::out)) {
	case hxfile::in:
		m = "rb";
		break;
	case hxfile::out:
		m = "wb";
		break;
	default:
		m = "w+b";
	}

	m_file_pimpl_ = ::fopen(buf, m);
	hxassertrelease(m_file_pimpl_ || (mode & hxfile::skip_asserts), "fopen %s", buf);
	m_owns_ = m_file_pimpl_ != hxnull;
	m_good_ = m_owns_;
	return m_good_;
}

void hxfile::close(void) {
	if (m_owns_) {
		::fclose((FILE*)m_file_pimpl_);
	}
	m_file_pimpl_ = hxnull;
	m_owns_	= false;
	m_open_mode_ = (uint8_t)0u;
	m_good_ = false;
	m_eof_ = false;
}

size_t hxfile::read(void* bytes, size_t byte_count) {
	hxassertmsg((m_open_mode_ & hxfile::in) && m_file_pimpl_ && bytes, "invalid_parameter");
	size_t bytes_read = ::fread(bytes, 1, byte_count, (FILE*)m_file_pimpl_);
	hxassertmsg((byte_count == bytes_read) || (m_open_mode_ & hxfile::skip_asserts),
		"file_read_wrong bytes %zu != actual %zu", byte_count, bytes_read);
	if (byte_count != bytes_read) {
		m_good_ = false;
		m_eof_ = m_file_pimpl_ ? ::feof((FILE*)m_file_pimpl_) : false;
	}
	return bytes_read;
}

size_t hxfile::write(const void* bytes, size_t byte_count) {
	hxassertmsg((m_open_mode_ & hxfile::out) && bytes, "invalid_parameter");
	if(m_file_pimpl_ == hxnull) {
		return 0; // /dev/null.
	}
	size_t bytes_written = ::fwrite(bytes, 1, byte_count, (FILE*)m_file_pimpl_);
	hxassertmsg((byte_count == bytes_written) || (m_open_mode_ & hxfile::skip_asserts),
		"file_write_wrong bytes %zu != actual %zu", byte_count, bytes_written);
	m_good_ = byte_count == bytes_written; // Can restore goodness.
	return bytes_written;
}

bool hxfile::get_line(char* buffer, size_t buffer_size) {
	hxassertmsg((m_open_mode_ & hxfile::in) && m_file_pimpl_ && buffer, "invalid_parameter");
	char* result = ::fgets(buffer, (int)buffer_size, (FILE*)m_file_pimpl_);
	if (!result) {
		m_good_ = false;
		m_eof_ = ::feof((FILE*)m_file_pimpl_); // 0 is not past end.
		hxassertmsg(m_eof_ || (m_open_mode_ & hxfile::skip_asserts), "fgets");
		return false; // EOF or error
	}
	return true;
}

bool hxfile::print(const char* format, ...) {
	hxassertmsg(format, "invalid_parameter null");

	char str[HX_MAX_LINE];
	va_list args;
	va_start(args, format);
	int len = ::vsnprintf(str, HX_MAX_LINE, format, args);
	va_end(args);

	// These are potential data corruption issues, not skip_asserts I/O.
	// Don't try and print the format string as it may be bad.
	hxassertrelease(len >= 0 && len < (int)HX_MAX_LINE, "vsnprintf");
	return write(str, len);
}
