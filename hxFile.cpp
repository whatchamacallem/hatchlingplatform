// Copyright 2017 Adrian Johnston

#include "hxFile.h"
#include <stdio.h>

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
	hxAssertRelease(hxFile::fallible || filename, "null filename");
	close();
	if (!filename) {
		return false; // Note fallible mode not stored.
	}

	char buf[hxFile::c_MaxLine];
	int sz = ::vsnprintf(buf, hxFile::c_MaxLine, filename, args);
	hxAssertRelease(0 < sz, "hxFile filename error: %s", filename); // C99

	const char* m = "";
	switch (mode & (hxFile::in | hxFile::out)) {
	case hxFile::in:
		m = "rb";
		break;
	case hxFile::out:
		m = "wb";
		break;
	default:
		hxAssertRelease(false, "file mode invalid, %d for %s", (int)mode, buf);
		break;
	}

	m_openMode = mode;
	m_filePImpl = (char*)::fopen(buf, m);
	hxAssertRelease(m_filePImpl || (mode & hxFile::fallible), "failed to open file: %s", buf);
	hxWarnCheck(m_filePImpl, "failed to open file: %s", buf);
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
	hxAssertRelease(bytes, "null i/o buffer");
	hxAssertRelease((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not readable");
	size_t bytesRead = m_filePImpl ? ::fread(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesRead) || (m_openMode & hxFile::fallible), "read bytes %d != actual %d", (int)byteCount, (int)bytesRead); (void)bytesRead;
	if (byteCount != bytesRead) {
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : true;
	}
	return bytesRead;
}

size_t hxFile::write(const void* bytes, size_t byteCount) {
	hxAssertRelease(bytes, "null i/o buffer");
	hxAssertRelease((m_openMode & hxFile::out) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not writable");
	size_t bytesWritten = m_filePImpl ? ::fwrite(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesWritten) || (m_openMode & hxFile::fallible), "write bytes %d != actual %d", (int)byteCount, (int)bytesWritten); (void)bytesWritten;
	m_good = m_good && byteCount == bytesWritten;
	return bytesWritten;
}

bool hxFile::getline(char* buffer, size_t size) {
	hxAssertRelease(buffer, "null i/o buffer");
	hxAssertRelease((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "invalid file");
	char* result = m_filePImpl ? ::fgets(buffer, (int)size, (FILE*)m_filePImpl) : null;
	if (!result) {
		buffer[0] = '\0';
		m_good = false;
		m_eof = m_filePImpl ? ::feof((FILE*)m_filePImpl) : true;
		hxAssertRelease(m_eof || (m_openMode & hxFile::fallible), "getline error");
		return false; // EOF or error
	}
	// This is a potential data corruption issue, do not mask with fallible.
	hxAssertRelease(::strlen(result) < size, "read line overflow"); // Using the last available byte is bad.
	return true;
}

#else // !HX_HAS_C_FILE
#error "TODO: file I/O"
#endif

bool hxFile::print(const char* format, ...) {
	char buf[hxFile::c_MaxLine];

	va_list args;
	va_start(args, format);
	int sz = ::vsnprintf(buf, hxFile::c_MaxLine, format, args); // C99
	va_end(args);

	// These are potential data corruption issues, do not mask with fallible.
	hxAssertRelease(sz >= 0, "file print error: %s", format);
	hxAssertRelease(sz < (int)hxFile::c_MaxLine, "file print overflow: %s", format);

	return write(buf, (size_t)sz);
}
