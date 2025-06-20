// Copyright 2017-2025 Adrian Johnston

#include <hx/hxfile.hpp>

#include <stdio.h>

#if defined _MSC_VER
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// hxfile - Target will require an implementation of fopen(), fclose(), fread(),
// fwrite(), fgets() and feof().

hxfile::hxfile(uint16_t mode) {
	m_open_mode_ = mode;
	m_good_ = false;
	m_eof_ = false;

	uint16_t stdio_mode = mode & (hxfile::stdio|hxfile::in|hxfile::out);
	if(stdio_mode == (hxfile::stdio|hxfile::in)) {
		m_file_pImpl_ = (char*)stdin;
	}
	else if(stdio_mode == (hxfile::stdio|hxfile::out)) {
		m_file_pImpl_ = (char*)stdout;
	}
	else {
		hxassertmsg((mode & hxfile::stdio) == 0, "stdio requires exactly one of in or out.");

		// failable I/O on a closed file will be ignored.
		m_file_pImpl_ = hxnull;
		m_open_mode_ = m_open_mode_ & ~hxfile::stdio;
	}
}

hxfile::hxfile(uint16_t mode, const char* filename, ...) {
	m_file_pImpl_ = hxnull;

	va_list args;
	va_start(args, filename);
	openv_(mode, filename, args);
	va_end(args);
}

hxfile::~hxfile() {
	close();
}

bool hxfile::open(uint16_t mode, const char* filename, ...) {
	va_list args;
	va_start(args, filename);
	bool rv = openv_(mode, filename, args);
	va_end(args);
	return rv;
}

bool hxfile::openv_(uint16_t mode, const char* filename, va_list args) {
	hxinit(); // Needed to write out asserts before main().
	close(); // clears all state

	hxassertmsg((mode & stdio) == 0, "both stdio and filename requested");
	mode &= ~(uint16_t)hxfile::stdio; // never free stdio.
	m_open_mode_ = mode;

	hxassertmsg((mode & ~(uint16_t)((1u << 5) - 1u)) == 0, "reserved file mode bits");
	hxassertrelease((mode & (hxfile::in | hxfile::out)) && filename, "missing file args");

	char buf[HX_MAX_LINE] = "";
	vsnprintf(buf, HX_MAX_LINE, filename, args);

	if (buf[0] == '\0') {
		return false;
	}

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

	m_file_pImpl_ = (char*)::fopen(buf, m);
	hxassertrelease(m_file_pImpl_ || (mode & hxfile::failable), "failed to open file: %s", buf);
	m_good_ = m_file_pImpl_ != hxnull;
	return m_good_;
}

void hxfile::close() {
	if (m_file_pImpl_ && (m_open_mode_ & hxfile::stdio) == 0) {
		::fclose((FILE*)m_file_pImpl_);
		m_file_pImpl_ = hxnull;
	}
	m_open_mode_ = (uint16_t)0u;
	m_good_ = false;
	m_eof_ = false;
}

size_t hxfile::read(void* bytes, size_t byte_count) {
	hxassertmsg(bytes, "null i/o buffer");
	hxassertmsg((m_open_mode_ & hxfile::in) && (m_file_pImpl_ || (m_open_mode_ & hxfile::failable)),
		"file not readable");
	size_t bytes_read = (bytes && m_file_pImpl_) ? ::fread(bytes, 1, byte_count, (FILE*)m_file_pImpl_) : 0u;
	hxassertrelease((byte_count == bytes_read) || (m_open_mode_ & hxfile::failable),
		"read bytes %zu != actual %zu", byte_count, bytes_read);
	if (byte_count != bytes_read) {
		m_good_ = false;
		m_eof_ = m_file_pImpl_ ? ::feof((FILE*)m_file_pImpl_) : false;
	}
	return bytes_read;
}

size_t hxfile::write(const void* bytes, size_t byte_count) {
	hxassertmsg(bytes, "null i/o buffer");
	hxassertmsg((m_open_mode_ & hxfile::out) && (m_file_pImpl_ || (m_open_mode_ & hxfile::failable)),
		"file not writable");
	size_t bytes_written = (bytes && m_file_pImpl_) ? ::fwrite(bytes, 1, byte_count, (FILE*)m_file_pImpl_) : 0u;
	hxassertrelease((byte_count == bytes_written) || (m_open_mode_ & hxfile::failable),
		"write bytes %zu != actual %zu", byte_count, bytes_written);
	m_good_ = byte_count == bytes_written; // Can restore goodness.
	return bytes_written;
}

bool hxfile::get_line(char* buffer, size_t buffer_size) {
	hxassertmsg(buffer, "null i/o buffer");
	hxassertmsg((m_open_mode_ & hxfile::in) && (m_file_pImpl_ || (m_open_mode_ & hxfile::failable)), "file not readable");
	char* result = (buffer && buffer_size && m_file_pImpl_) ? ::fgets(buffer, (int)buffer_size, (FILE*)m_file_pImpl_) : hxnull;
	if (!result) {
		if (buffer && buffer_size) {
			buffer[0] = '\0';
		}
		m_good_ = false;
		m_eof_ = m_file_pImpl_ ? ::feof((FILE*)m_file_pImpl_) : false;
		hxassertrelease(m_eof_ || (m_open_mode_ & hxfile::failable), "get_line error");
		return false; // EOF or error
	}
	return true;
}

bool hxfile::print(const char* format, ...) {
	hxassert(format);

	char str[HX_MAX_LINE] = "";
	va_list args;
	va_start(args, format);
	int len = vsnprintf(str, HX_MAX_LINE, format, args);
	va_end(args);

	// These are potential data corruption issues, not failable I/O.
	hxassertrelease(len >= 0 && len < (int)HX_MAX_LINE, "file print error: %s", format);
	return write(str, len);
}
