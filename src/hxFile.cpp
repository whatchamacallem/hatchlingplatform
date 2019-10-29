// Copyright 2017-2019 Adrian Johnston

#include <hx/hxFile.h>

#if HX_HAS_C_FILE
#include <stdio.h>
#endif

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------------
// hxFile
//
// Target will require an implementation of fopen(), fclose(), fread(), fwrite(),
// fgets() and feof().

// Wrapped to ensure correct construction order.
hxFile& hxLogFile() {
	static hxFile f(hxFile::out | hxFile::fallible | hxFile::echo, "%s", g_hxSettings.logFile ? g_hxSettings.logFile : "");
	return f;
}

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
	hxAssertMsg((mode & ~(uint16_t)((1u << 5) - 1u)) == 0, "using reserved file mode");
	close();

	m_openMode = mode;

	if (mode == 0 || filename == hxnull) {
		return false;
	}

	char buf[HX_MAX_LINE] = "";
	::vsnprintf(buf, HX_MAX_LINE, filename, args); // C99

	if (buf[0] == '\0') {
		return false;
	}

	const char* m = "";
	switch (mode & (hxFile::in | hxFile::out)) {
	case hxFile::in:
		m = "rb";
		break;
	case hxFile::out:
		m = "wb";
		break;
	default:
		m = "rb+";
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
#if HX_HAS_C_FILE
	if (m_openMode & echo) {
		::fwrite(bytes, 1, byteCount, stdout);
	}
#endif

	hxAssertMsg(bytes, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::out) && (m_filePImpl || (m_openMode & hxFile::fallible)), "file not writable");
	size_t bytesWritten = (bytes && m_filePImpl) ? ::fwrite(bytes, 1, byteCount, (FILE*)m_filePImpl) : 0u;
	hxAssertRelease((byteCount == bytesWritten) || (m_openMode & hxFile::fallible), "write bytes %d != actual %d", (int)byteCount, (int)bytesWritten); (void)bytesWritten;
	m_good = byteCount == bytesWritten; // Can restore goodness.
	return bytesWritten;
}

bool hxFile::getline(char* buffer, size_t bufferSize) {
	hxAssertMsg(buffer, "null i/o buffer");
	hxAssertMsg((m_openMode & hxFile::in) && (m_filePImpl || (m_openMode & hxFile::fallible)), "invalid file");
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

#else // !HX_HAS_C_FILE
#error "TODO: file I/O"
#endif

bool hxFile::print(const char* format, ...) {
	hxAssert(format);

	char str[HX_MAX_LINE] = "";
	va_list args;
	va_start(args, format);
	int len = ::vsnprintf(str, HX_MAX_LINE, format, args); // C99
	va_end(args);

	// These are potential data corruption issues, not fallible I/O.
	hxAssertRelease(len >= 0 && len < (int)HX_MAX_LINE, "file print error: %s", format);
	return write(str, len);
}

