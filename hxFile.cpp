// Copyright 2017 Adrian Johnston

#include "hxFile.h"
#include <stdio.h>

HX_REGISTER_FILENAME_HASH;

// Target will require an implementation of fopen(), fclose(), fread(), fwrite(),
// fgets() and feof().

hxFile::hxFile() {
	m_filePImpl = null;
	m_openMode = 0u;
	m_good = false;
	m_eof = false;
}

hxFile::hxFile(uint16_t mode, const char* filename, ...) {
	m_filePImpl = null;
	va_list args;
	va_start(args, filename);
	openV(mode, filename, args);
	va_end(args);
}

hxFile::~hxFile() {
	close();
}

bool hxFile::open(uint16_t mode, const char* filename, ...) {
	va_list args;
	va_start(args, filename);
	bool rv = openV(mode, filename, args);
	va_end(args);
	return rv;
}

#if HX_HAS_C_FILE

bool hxFile::openV(uint16_t mode, const char* filename, va_list args) {
	hxAssertMsg((mode & hxFile::reserved_) == 0, "using reserved file mode");
	close();

	char buf[HX_MAX_LINE] = "";
	int sz = filename ? ::vsnprintf(buf, HX_MAX_LINE, filename, args) : 0; // C99
	hxAssertMsg(sz > 0, "hxFile filename error: %s", filename ? filename : "<null>"); (void)sz;

	const char* m = "";
	switch (mode & (hxFile::in | hxFile::out)) {
	case hxFile::in:
		m = "rb";
		break;
	case hxFile::out:
		m = "wb";
		break;
	default:
		hxAssertMsg(false, "file mode invalid, %d for %s", (int)mode, buf);
	}

	m_openMode = mode;
	m_filePImpl = (char*)::fopen(buf, m);
	hxAssertRelease(m_filePImpl || (mode & hxFile::fallible), "failed to open file: %s", buf);
	m_good = m_filePImpl != null;
	return m_good;
}

void hxFile::close() {
	if (m_filePImpl) {
		::fclose((FILE*)m_filePImpl);
		m_filePImpl = null;
	}
	m_openMode = 0u;
	m_good = false;
	m_eof = false;
}

size_t hxFile::read(void* bytes, size_t byteCount) {
	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not readable");
	size_t bytesRead = (bytes && m_filePImpl) ? ::fread(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesRead) || (m_openMode & hxFile::fallible), "read bytes %d != actual %d", (int)byteCount, (int)bytesRead); (void)bytesRead;
	if (byteCount != bytesRead) {
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : false;
	}
	return bytesRead;
}

size_t hxFile::write(const void* bytes, size_t byteCount) {
	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::out) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not writable");
	size_t bytesWritten = (bytes && m_filePImpl) ? ::fwrite(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesWritten) || (m_openMode & hxFile::fallible), "write bytes %d != actual %d", (int)byteCount, (int)bytesWritten); (void)bytesWritten;
	m_good = m_good && byteCount == bytesWritten;
	return bytesWritten;
}

bool hxFile::getline(char* buffer, size_t bufferSize) {
	hxAssertMsg(buffer, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "invalid file");
	char* result = (buffer && m_filePImpl) ? ::fgets(buffer, (int)bufferSize, (FILE*)m_filePImpl) : null;
	if (!result) {
		buffer[0] = '\0';
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : false;
		hxAssertRelease(m_eof || (m_openMode & hxFile::fallible), "getline error");
		return false; // EOF or error
	}
	// This is a potential data corruption issue, do not mask with fallible.
	hxAssertRelease(::strlen(result) < bufferSize, "getline overflow"); // Using the last available byte is bad.
	return true;
}

#else // !HX_HAS_C_FILE
#error "TODO: file I/O"
#endif

bool hxFile::print(const char* format, ...) {
	char buf[HX_MAX_LINE] = "";

	va_list args;
	va_start(args, format);
	int sz = ::vsnprintf(buf, HX_MAX_LINE, format ? format : "<null>", args); // C99
	va_end(args);

	// These are potential data corruption issues, not fallible I/O.
	hxAssertRelease(sz >= 0 && sz < (int)HX_MAX_LINE, "file print error: %s", format);

	return (sz >= 0) ? write(buf, (sz < (int)HX_MAX_LINE) ? (size_t)sz : (size_t)HX_MAX_LINE) : false;
}
