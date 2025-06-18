// Copyright 2017-2025 Adrian Johnston

#include <hx/hxFile.hpp>

#include <stdio.h>

#if defined _MSC_VER
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// hxFile - Target will require an implementation of fopen(), fclose(), fread(),
// fwrite(), fgets() and feof().

hxFile::hxFile(uint16_t mode) {
	m_openMode_ = mode;
	m_good_ = false;
	m_eof_ = false;

	uint16_t stdioMode = mode & (hxFile::stdio|hxFile::in|hxFile::out);
	if(stdioMode == (hxFile::stdio|hxFile::in)) {
		m_filePImpl_ = (char*)stdin;
	}
	else if(stdioMode == (hxFile::stdio|hxFile::out)) {
		m_filePImpl_ = (char*)stdout;
	}
	else {
		hxAssertMsg((mode & hxFile::stdio) == 0, "stdio requires exactly one of in or out.");

		// failable I/O on a closed file will be ignored.
		m_filePImpl_ = hxnull;
		m_openMode_ = m_openMode_ & ~hxFile::stdio;
	}
}

hxFile::hxFile(uint16_t mode, const char* filename, ...) {
	m_filePImpl_ = hxnull;

	va_list args;
	va_start(args, filename);
	openv_(mode, filename, args);
	va_end(args);
}

hxFile::~hxFile() {
	close();
}

bool hxFile::open(uint16_t mode, const char* filename, ...) {
	va_list args;
	va_start(args, filename);
	bool rv = openv_(mode, filename, args);
	va_end(args);
	return rv;
}

bool hxFile::openv_(uint16_t mode, const char* filename, va_list args) {
	hxInit(); // Needed to write out asserts before main().
	close(); // clears all state

	hxAssertMsg((mode & stdio) == 0, "both stdio and filename requested");
	mode &= ~(uint16_t)hxFile::stdio; // never free stdio.
	m_openMode_ = mode;

	hxAssertMsg((mode & ~(uint16_t)((1u << 5) - 1u)) == 0, "reserved file mode bits");
	hxAssertRelease((mode & (hxFile::in | hxFile::out)) && filename, "missing file args");

	char buf[HX_MAX_LINE] = "";
	vsnprintf(buf, HX_MAX_LINE, filename, args);

	if (buf[0] == '\0') {
		return false;
	}

	const char* m = hxnull;
	switch (mode & (hxFile::in | hxFile::out)) {
	case hxFile::in:
		m = "rb";
		break;
	case hxFile::out:
		m = "wb";
		break;
	default:
		m = "w+b";
	}

	m_filePImpl_ = (char*)::fopen(buf, m);
	hxAssertRelease(m_filePImpl_ || (mode & hxFile::failable), "failed to open file: %s", buf);
	m_good_ = m_filePImpl_ != hxnull;
	return m_good_;
}

void hxFile::close() {
	if (m_filePImpl_ && (m_openMode_ & hxFile::stdio) == 0) {
		::fclose((FILE*)m_filePImpl_);
		m_filePImpl_ = hxnull;
	}
	m_openMode_ = (uint16_t)0u;
	m_good_ = false;
	m_eof_ = false;
}

size_t hxFile::read(void* bytes, size_t byteCount) {
	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode_ & hxFile::in) && (m_filePImpl_ || (m_openMode_ & hxFile::failable)),
		"file not readable");
	size_t bytesRead = (bytes && m_filePImpl_) ? ::fread(bytes, 1, byteCount, (FILE*)m_filePImpl_) : 0u;
	hxAssertRelease((byteCount == bytesRead) || (m_openMode_ & hxFile::failable),
		"read bytes %zu != actual %zu", byteCount, bytesRead);
	if (byteCount != bytesRead) {
		m_good_ = false;
		m_eof_ = m_filePImpl_ ? ::feof((FILE*)m_filePImpl_) : false;
	}
	return bytesRead;
}

size_t hxFile::write(const void* bytes, size_t byteCount) {
	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode_ & hxFile::out) && (m_filePImpl_ || (m_openMode_ & hxFile::failable)),
		"file not writable");
	size_t bytesWritten = (bytes && m_filePImpl_) ? ::fwrite(bytes, 1, byteCount, (FILE*)m_filePImpl_) : 0u;
	hxAssertRelease((byteCount == bytesWritten) || (m_openMode_ & hxFile::failable),
		"write bytes %zu != actual %zu", byteCount, bytesWritten);
	m_good_ = byteCount == bytesWritten; // Can restore goodness.
	return bytesWritten;
}

bool hxFile::getLine(char* buffer, size_t bufferSize) {
	hxAssertMsg(buffer, "null i/o buffer");
	hxAssertMsg((m_openMode_ & hxFile::in) && (m_filePImpl_ || (m_openMode_ & hxFile::failable)), "file not readable");
	char* result = (buffer && bufferSize && m_filePImpl_) ? ::fgets(buffer, (int)bufferSize, (FILE*)m_filePImpl_) : hxnull;
	if (!result) {
		if (buffer && bufferSize) {
			buffer[0] = '\0';
		}
		m_good_ = false;
		m_eof_ = m_filePImpl_ ? ::feof((FILE*)m_filePImpl_) : false;
		hxAssertRelease(m_eof_ || (m_openMode_ & hxFile::failable), "getLine error");
		return false; // EOF or error
	}
	return true;
}

bool hxFile::print(const char* format, ...) {
	hxAssert(format);

	char str[HX_MAX_LINE] = "";
	va_list args;
	va_start(args, format);
	int len = vsnprintf(str, HX_MAX_LINE, format, args);
	va_end(args);

	// These are potential data corruption issues, not failable I/O.
	hxAssertRelease(len >= 0 && len < (int)HX_MAX_LINE, "file print error: %s", format);
	return write(str, len);
}
