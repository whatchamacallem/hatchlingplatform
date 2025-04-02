// Copyright 2017-2019 Adrian Johnston

#include <hx/hxFile.h>

#if HX_USE_STDIO_H
#include <stdio.h>
#endif

#if defined(_MSC_VER)
#pragma warning(disable: 4996) // Allow use of fopen as fopen_s is not C99.
#endif

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// This is hxout.

// Wrapped to ensure correct construction order.
hxFile& hxOut() {
	static hxFile f(hxFile::out | hxFile::echo | hxFile::fallible, "%s",
		(g_hxIsInit && g_hxSettings.logFile) ? g_hxSettings.logFile : "");
	return f;
}

void hxCloseOut() {
	// Allow assert messages to be written to stdout.
	hxout.open(hxFile::out | hxFile::echo | hxFile::fallible, "%s", "");
}

// ----------------------------------------------------------------------------
// hxFile
//
// With HX_USE_STDIO_H target will require an implementation of fopen(), fclose(),
// fread(), fwrite(), fgets() and feof().

hxFile::hxFile(uint16_t mode) {
	m_filePImpl = hxnull;
	m_openMode = mode;
	m_good = false;
	m_eof = false;
}

hxFile::hxFile(uint16_t mode, const char* filename, ...) {
	m_filePImpl = hxnull;
	
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

HX_STATIC_ASSERT(HX_USE_STDIO_H, "TODO: File I/O");
#if HX_USE_STDIO_H

bool hxFile::openv_(uint16_t mode, const char* filename, va_list args) {
	hxInit(); // Needed to write out asserts at startup.
	close(); // clears all state
	m_openMode = mode; 

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

	m_filePImpl = (char*)::fopen(buf, m);
	hxAssertRelease(m_filePImpl || (mode & hxFile::fallible), "failed to open file: %s", buf);
	m_good = m_filePImpl != hxnull;
	return m_good;
}

void hxFile::close() {
	if (m_filePImpl) {
		::fclose((FILE*)m_filePImpl);
		m_filePImpl = hxnull;
	}
	m_openMode = (uint16_t)0u;
	m_good = false;
	m_eof = false;
}

size_t hxFile::read(void* bytes, size_t byteCount) {
	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)),
		"file not readable");
	size_t bytesRead = (bytes && m_filePImpl) ? ::fread(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesRead) || (m_openMode & hxFile::fallible),
		"read bytes %zu != actual %zu", byteCount, bytesRead);
	if (byteCount != bytesRead) {
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : false;
	}
	return bytesRead;
}

size_t hxFile::write(const void* bytes, size_t byteCount) {
	if (m_openMode & echo) {
		::fwrite(bytes, 1, byteCount, stdout);
	}

	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode & (hxFile::out | hxFile::echo)) && (m_filePImpl || (m_openMode & hxFile::fallible)),
		"file not writable");
	size_t bytesWritten = (bytes && m_filePImpl) ? ::fwrite(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesWritten) || (m_openMode & hxFile::fallible),
		"write bytes %zu != actual %zu", byteCount, bytesWritten);
	m_good = byteCount == bytesWritten; // Can restore goodness.
	return bytesWritten;
}

bool hxFile::getline(char* buffer, size_t bufferSize) {
	hxAssertMsg(buffer, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not readable");
	char* result = (buffer && bufferSize && m_filePImpl) ? ::fgets(buffer, (int)bufferSize, (FILE*)m_filePImpl) : hxnull;
	if (!result) {
		if (buffer && bufferSize) {
			buffer[0] = '\0';
		}
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : false;
		hxAssertRelease(m_eof || (m_openMode & hxFile::fallible), "getline error");
		return false; // EOF or error
	}
	return true;
}
#endif

bool hxFile::print(const char* format, ...) {
	hxAssert(format);

	char str[HX_MAX_LINE] = "";
	va_list args;
	va_start(args, format);
	int len = vsnprintf(str, HX_MAX_LINE, format, args);
	va_end(args);

	// These are potential data corruption issues, not fallible I/O.
	hxAssertRelease(len >= 0 && len < (int)HX_MAX_LINE, "file print error: %s", format);
	return write(str, len);
}
