// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// This is the C <stdio.h> version using FILE*. Use an alternate .cpp file for
// alternate implementations.

#include <hx/hxfile.hpp>

#include <stdio.h>
#include <errno.h>

#if defined _MSC_VER
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// hxfile - Target will require an implementation of fopen(), fclose(), fread(),
// fwrite(), fgets() and feof().

hxfile hxin(stdin, hxfile::in);
hxfile hxout(stdout, hxfile::out);
hxfile hxerr(stderr, hxfile::out);
hxfile hxdev_null(hxnull, hxfile::out);

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

	m_open_mode_ = mode; // Record mode regardless.
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
	hxassertrelease(m_file_pimpl_ || (mode & hxfile::skip_asserts), \
		"fopen %s %s: %s", buf, m, ::strerror(errno));

	m_owns_ = m_file_pimpl_ != hxnull;
	m_good_ = m_owns_;
	return m_good_;
}

void hxfile::close(void) {
	if (m_owns_) {
		::fclose((FILE*)m_file_pimpl_);
	}
	::memset((void*)this, 0x00, sizeof *this);
}

size_t hxfile::read(void* bytes, size_t byte_count) {
	hxassertmsg((m_open_mode_ & hxfile::in) && m_file_pimpl_ && bytes, "invalid_parameter");

	size_t bytes_read = ::fread(bytes, 1, byte_count, (FILE*)m_file_pimpl_);

	hxassertmsg((byte_count == bytes_read) || (m_open_mode_ & hxfile::skip_asserts),
		"fread expected %zu != actual %zu: %s", byte_count, bytes_read, ::strerror(errno));

	if (byte_count != bytes_read) {
		m_good_ = false;
		m_eof_ = ::feof((FILE*)m_file_pimpl_);
	}
	return bytes_read;
}

size_t hxfile::write(const void* bytes, size_t byte_count) {
	hxassertmsg((m_open_mode_ & hxfile::out) && bytes, "invalid_parameter");

	if(m_file_pimpl_ == hxnull) {
		// Writing to null as /dev/null supported.
		return 0u;
	}
	size_t bytes_written = ::fwrite(bytes, 1, byte_count, (FILE*)m_file_pimpl_);

	hxassertmsg((byte_count == bytes_written) || (m_open_mode_ & hxfile::skip_asserts),
		"fwrite expected %zu != actual %zu: %s", byte_count, bytes_written, ::strerror(errno));

	m_good_ = byte_count == bytes_written; // Can restore goodness.
	return bytes_written;
}

bool hxfile::get_line(char* buffer, size_t buffer_size) {
	hxassertmsg((m_open_mode_ & hxfile::in) && m_file_pimpl_ && buffer, "invalid_parameter");

	char* result = ::fgets(buffer, (int)buffer_size, (FILE*)m_file_pimpl_);

	hxassertmsg(!::ferror((FILE*)m_file_pimpl_), "fgets %s", ::strerror(errno));

	if (!result) {
		m_good_ = false;
		m_eof_ = (bool)::feof((FILE*)m_file_pimpl_); // 0: not past end.
		return false; // EOF or error
	}
	return true;
}

// See vsnprintf to reimplement without FILE* support.
bool hxfile::print(const char* format, ...) {
	hxassertmsg((m_open_mode_ & hxfile::out) && format, "invalid_parameter");

	if(m_file_pimpl_ == hxnull) {
		// Writing to null as /dev/null supported.
		return 0u;
	}

	va_list args;
	va_start(args, format);
	int len = ::vfprintf((FILE*)m_file_pimpl_, format, args);
	va_end(args);

	hxassertrelease(len >= 0, "vfprintf %s", ::strerror(errno));
	return len >= 0;
}

// See vscanf to reimplement without FILE* support.
bool hxfile::scan(const char* format, ...) {
	hxassertmsg((m_open_mode_ & hxfile::in) && m_file_pimpl_ && format, "invalid_parameter");

	va_list args;
	va_start(args, format);
	int len = ::vfscanf((FILE*)m_file_pimpl_, format, args);
	va_end(args);

	hxassertrelease(len >= 0 || (m_open_mode_ & hxfile::skip_asserts), "vfscanf %s", ::strerror(errno));
	return len >= 0;
}
