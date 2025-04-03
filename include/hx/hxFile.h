#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxFile;

// ----------------------------------------------------------------------------
// hxout

// hxout is the system log.  Returns an hxFile for use as both stdout and stderr.
// If you don't want a written file set g_hxSettings.logFile to hxnull.
#define hxout hxOut()

// Implements hxout.
hxFile& hxOut();

// Closes the system log while still allowing assert messages to be written to stdout.
void hxCloseOut();

// ----------------------------------------------------------------------------
// hxFile: RAII wrapper for file I/O.  A mixture of unformatted std::basic_fstream
// operations and formatted C-style text printing.

class hxFile {
public:
	// openmode is a subset of std::ios_base::openmode with fallible and echo added.
	// fallible skips asserts and is similar to setting std::basic_ios::exceptions(0). 
	enum openmode {
		in = 1u << 0,       // Open for binary reading.
		out = 1u << 1,      // Open for binary writing.
		fallible = 1u << 2, // Skip asserts.
		echo = 1u << 3,     // Echo to stdout if available.
	};

	// Stream is closed by default.
	hxFile(uint16_t mode_=0u);

	// Opens a stream using a formatted filename.  Non-standard arg order.
	hxFile(uint16_t mode_, const char* filename_, ...) HX_ATTR_FORMAT(3, 4);

	// Closes stream.
	~hxFile();

	// Opens a stream using a formatted filename.
	bool open(uint16_t mode_, const char* filename_, ...) HX_ATTR_FORMAT(3, 4);

	// Closes stream.
	void close();

	HX_INLINE bool is_open() const { return m_filePImpl != hxnull; }

	HX_INLINE bool good() const { return m_good; }

	HX_INLINE bool eof() const { return m_eof; }   // Check for EOF only.

	HX_INLINE void clear() {
		m_good = m_filePImpl != hxnull;
		m_eof = false;
	}

	// Returns whether operations may fail without asserting.  Non-standard, similar
	// to checking if exceptions are enabled.
	HX_INLINE bool is_fallible() const { return (m_openMode & fallible) != 0; }

	// Returns whether all writes will be echoed to stdout.  Non-standard.
	HX_INLINE bool is_echo() const { return (m_openMode & echo) != 0; }

	size_t read(void* bytes_, size_t count_);

	size_t write(const void* bytes_, size_t count_);

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on
	// EOF without needing to be hxFile::fallible.  Automatically determines
	// the size of the provided char array.  buffer is a reference to a char
	// array.
	template<size_t BufferSize_>
	HX_INLINE bool getline(char(&buffer_)[BufferSize_]) { return getline(buffer_, BufferSize_); }

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on EOF
	// without needing to be hxFile::fallible.
	bool getline(char* buffer_, size_t bufferSize_);

	// Formatted string write.  Must be less than HX_MAX_LINE characters.  gcc
	// considers "this" to be argument 1.
	bool print(const char* format_, ...) HX_ATTR_FORMAT(2, 3);

	// Read a single unformatted native endian object.
	template<typename T_>
	HX_INLINE bool read1(T_& t_) { return read(&t_, sizeof t_) == sizeof t_; }

	// Write a single unformatted native endian object.
	template<typename T_>
	HX_INLINE bool write1(const T_& t_) { return write(&t_, sizeof t_) == sizeof t_; }

	// Read a single unformatted native endian object from a stream.  The operator
	// >= is being used instead of >> to indicate there is no formatting.
	template<typename T_>
	HX_INLINE hxFile& operator>=(T_& t_) {
		read(&t_, sizeof t_);
		return *this;
	}

	// Write a single unformatted native endian object to a stream.  The operator
	// <= is being used instead of << to indicate there is no formatting.
	template<typename T_>
	HX_INLINE hxFile& operator<=(const T_& t_) {
		write(&t_, sizeof t_);
		return *this;
	}

	// Write a string literal.  Supports Google Test style diagnostic messages.
	template<size_t StringLength_>
	HX_INLINE hxFile& operator<<(const char(&str_)[StringLength_]) {
		hxAssert(::strnlen(str_, StringLength_) == (StringLength_-1));
		write(str_, StringLength_-1);
		return *this;
	}

	// Write a null terminated string.  Supports Google Test style diagnostic
	// messages.
	template<size_t StringLength_>
	HX_INLINE hxFile& operator<<(char(&str_)[StringLength_]) {
		write(str_, ::strnlen(str_, StringLength_));
		return *this;
	}

private:
	hxFile(const hxFile&); // = delete
	void operator=(const hxFile&); // = delete
	template<typename T_> HX_INLINE hxFile& operator>>(const T_* t_); // = delete

	bool openv_(uint16_t mode_, const char* format_, va_list args_);

	char* m_filePImpl;
	uint16_t m_openMode;
	bool m_good;
	bool m_eof;
};
