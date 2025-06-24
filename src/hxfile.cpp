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
	m_file_pimpl_ = hxnull;
	open(mode, hxnull);
}

hxfile::hxfile(uint16_t mode, const char* filename, ...) {
	m_file_pimpl_ = hxnull;
	close(); // openv_ assumes initialized and closed

	va_list args;
	va_start(args, filename);
	openv_(mode, filename, args);
	va_end(args);
}

hxfile::~hxfile(void) {
	close();
}

bool hxfile::open(uint16_t mode, const char* filename, ...) {
	close(); // openv_ assumes closed

	va_list args;
	va_start(args, filename);
	bool rv = openv_(mode, filename, args);
	va_end(args);
	return rv;
}

bool hxfile::openv_(uint16_t mode, const char* filename, va_list args) {
	hxinit(); // Needed to write out asserts before main().

	hxassertmsg(m_file_pimpl_ == hxnull, "internal_error");
	hxassertmsg((mode & ~(uint16_t)((1u << 5) - 1u)) == 0, "invalid_parameter reserved bits");

	uint16_t stdio_mode = mode & (hxfile::stdio|hxfile::in|hxfile::out);
	if(stdio_mode == (hxfile::stdio|hxfile::in)) {
		hxassertmsg(!filename, "invalid_parameter stdio+filename");
		m_file_pimpl_ = (char*)stdin;
		m_open_mode_ = mode;
		m_good_ = true;
		return true;
	}
	else if(stdio_mode == (hxfile::stdio|hxfile::out)) {
		hxassertmsg(!filename, "invalid_parameter stdio+filename");
		m_file_pimpl_ = (char*)stdout;
		m_open_mode_ = mode;
		m_good_ = true;
		return true;
	}
	else if(filename == hxnull) {
		m_open_mode_ = mode; // Record failable mode.
		return false;
	}

	// Avoid a file handle leak.
	hxassertmsg((mode & hxfile::stdio) == 0, "invalid_parameter stdio in+out");
	mode &= ~hxfile::stdio;
	m_open_mode_ = mode;

	char buf[HX_MAX_LINE] = "";
	::vsnprintf(buf, HX_MAX_LINE, filename, args);

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

	m_file_pimpl_ = (char*)::fopen(buf, m);
	hxassertrelease(m_file_pimpl_ || (mode & hxfile::failable), "fopen %s", buf);
	m_good_ = m_file_pimpl_ != hxnull;
	return m_good_;
}

void hxfile::close(void) {
	if (m_file_pimpl_ && (m_open_mode_ & hxfile::stdio) == 0) {
		::fclose((FILE*)m_file_pimpl_);
	}
	m_file_pimpl_ = hxnull;
	m_open_mode_ = (uint16_t)0u;
	m_good_ = false;
	m_eof_ = false;
}

size_t hxfile::read(void* bytes, size_t byte_count) {
	hxassertmsg(bytes, "invalid_parameter null");
	hxassertmsg((m_open_mode_ & hxfile::in) && (m_file_pimpl_ || (m_open_mode_ & hxfile::failable)),
		"file_not_readable");
	size_t bytes_read = (bytes && m_file_pimpl_) ? ::fread(bytes, 1, byte_count, (FILE*)m_file_pimpl_) : 0u;
	hxassertrelease((byte_count == bytes_read) || (m_open_mode_ & hxfile::failable),
		"file_read_wrong bytes %zu != actual %zu", byte_count, bytes_read);
	if (byte_count != bytes_read) {
		m_good_ = false;
		m_eof_ = m_file_pimpl_ ? ::feof((FILE*)m_file_pimpl_) : false;
	}
	return bytes_read;
}

size_t hxfile::write(const void* bytes, size_t byte_count) {
	hxassertmsg(bytes, "invalid_parameter null");
	hxassertmsg((m_open_mode_ & hxfile::out) && (m_file_pimpl_ || (m_open_mode_ & hxfile::failable)),
		"file_not_writable");
	size_t bytes_written = (bytes && m_file_pimpl_) ? ::fwrite(bytes, 1, byte_count, (FILE*)m_file_pimpl_) : 0u;
	hxassertrelease((byte_count == bytes_written) || (m_open_mode_ & hxfile::failable),
		"file_read_wrong bytes %zu != actual %zu", byte_count, bytes_written);
	m_good_ = byte_count == bytes_written; // Can restore goodness.
	return bytes_written;
}

bool hxfile::get_line(char* buffer, size_t buffer_size) {
	hxassertmsg(buffer, "invalid_parameter null");
	hxassertmsg((m_open_mode_ & hxfile::in) && (m_file_pimpl_ || (m_open_mode_ & hxfile::failable)), "file not readable");
	char* result = (buffer && buffer_size && m_file_pimpl_) ? ::fgets(buffer, (int)buffer_size, (FILE*)m_file_pimpl_) : hxnull;
	if (!result) {
		if (buffer && buffer_size) {
			buffer[0] = '\0';
		}
		m_good_ = false;
		m_eof_ = m_file_pimpl_ ? ::feof((FILE*)m_file_pimpl_) : false;
		hxassertrelease(m_eof_ || (m_open_mode_ & hxfile::failable), "fgets");
		return false; // EOF or error
	}
	return true;
}

bool hxfile::print(const char* format, ...) {
	hxassertmsg(format, "invalid_parameter null");

	char str[HX_MAX_LINE] = "";
	va_list args;
	va_start(args, format);
	int len = ::vsnprintf(str, HX_MAX_LINE, format, args);
	va_end(args);

	// These are potential data corruption issues, not failable I/O.
	// Don't try and print the format string as it may be bad.
	hxassertrelease(len >= 0 && len < (int)HX_MAX_LINE, "vsnprintf");
	return write(str, len);
}
