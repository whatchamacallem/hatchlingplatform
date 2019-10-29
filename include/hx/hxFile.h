#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>

// hxout is the hxFile for both stdout and the system log.
#define hxout hxLogFile()

// ----------------------------------------------------------------------------
// hxFile: RAII wrapper for stream I/O.  A mixture of unformatted std::basic_fstream
// operations and formatted C-style text printing.

class hxFile {
public:
	// openmode is a subset of std::ios_base::openmode with fallible and echo added.
	// fallible is similar to setting std::basic_ios::exceptions(0). 
	enum openmode {
		in = 1u << 0,       // Open for binary reading.
		out = 1u << 1,      // Open for binary writing.
		fallible = 1u << 2, // Skip asserts.
		echo = 1u << 3,     // Echo to stdout if available.
		binary = 1u << 4,   // This flag is ignored.
	};

	// Stream is closed by default.
	hxFile(uint16_t mode=0u);

	// Opens a stream using a formatted filename.  Non-standard arg order.
	hxFile(uint16_t mode, const char* filename, ...) HX_ATTR_FORMAT(3, 4);

	// Closes stream.
	~hxFile();

	// Opens a stream using a formatted filename.  Non-standard arg order.
	bool open(uint16_t mode, const char* filename, ...) HX_ATTR_FORMAT(3, 4);

	void close();

	HX_INLINE bool is_open() const { return m_filePImpl != hxnull; }

	HX_INLINE bool good() const { return m_good; }

	HX_INLINE bool eof() const { return m_eof; }   // Check for EOF only.

	HX_INLINE void clear() {
		m_good = m_filePImpl != hxnull;
		m_eof = false;
	}

	// Returns whether operations may fail without asserting.  Non-standard, similar to
	// checking if exceptions are enabled.
	HX_INLINE bool is_fallible() const { return (m_openMode & fallible) != 0; }

	size_t read(void* bytes, size_t count);

	size_t write(const void* bytes, size_t count);

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on
	// EOF without needing to be hxFile::fallible.  Automatically determines
	// the size of the provided char array.
	template<size_t BufferSize>
	HX_INLINE bool getline(char(&buffer)[BufferSize]) { return getline(buffer, BufferSize); }

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on
	// EOF without needing to be hxFile::fallible.
	bool getline(char* buffer, size_t bufferSize);

	// Formatted string write.  Must be less than HX_MAX_LINE characters.
	bool print(const char* format, ...) HX_ATTR_FORMAT(2, 3); // gcc considers "this" to be argument 1.

	// Read a single unformatted native endian object.
	template<typename T>
	HX_INLINE bool read1(T& t) { return read(&t, sizeof t) == sizeof t; }

	// Write a single unformatted native endian object.
	template<typename T>
	HX_INLINE bool write1(const T& t) { return write(&t, sizeof t) == sizeof t; }

	// Read a single unformatted native endian object from a stream.
	template<typename T>
	HX_INLINE hxFile& operator>>(T& t) {
		read(&t, sizeof t);
		return *this;
	}

	// Write a single unformatted native endian object to a stream.
	template<typename T>
	HX_INLINE hxFile& operator<<(const T& t) {
		write(&t, sizeof t);
		return *this;
	}

	// Supports GoogleTest style diagnostic messages.
	template<size_t StringLength>
	HX_INLINE hxFile& operator<<(const char(&str)[StringLength]) {
		write(str, StringLength-1);
		return *this;
	}

private:
	hxFile(const hxFile&); // = delete
	void operator=(const hxFile&); // = delete
	template<typename T> HX_INLINE hxFile& operator>>(const T* t); // = delete

	bool openV(uint16_t mode, const char* format, va_list args);

	char* m_filePImpl;
	uint16_t m_openMode;
	bool m_good;
	bool m_eof;
};

// See hxout.
hxFile& hxLogFile();