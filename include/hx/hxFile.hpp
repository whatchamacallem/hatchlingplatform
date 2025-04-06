#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxFile;

// ----------------------------------------------------------------------------
// hxFile: RAII wrapper for file I/O.  A mixture of unformatted std::basic_fstream
// operations and formatted C-style text printing.

class hxFile {
public:
	// openMode is a subset of std::ios_base::openmode with fallible added.
	// fallible skips asserts and is similar to setting std::basic_ios::exceptions(0). 
	enum openMode {
		in = 1u << 0,       // Open for binary reading.
		out = 1u << 1,      // Open for binary writing.
		fallible = 1u << 2  // Skip asserts.
	};

	// Constructor to initialize the file object with a specific mode.
	hxFile(uint16_t mode_=0u);

	// Constructor to initialize and open a file with a formatted filename.
	// Opens a stream using a formatted filename.  Non-standard arg order.
	hxFile(uint16_t mode_, const char* filename_, ...) HX_ATTR_FORMAT(3, 4);

	// Destructor to ensure the file is closed when the object goes out of scope.
	~hxFile();

	// Opens a file with the specified mode and formatted filename.
	bool open(uint16_t mode_, const char* filename_, ...) HX_ATTR_FORMAT(3, 4);

	// Closes the currently open file.
	void close();

	// Checks if the file is open.
	HX_INLINE bool isOpen() const { return m_filePImpl != hxnull; }

	// Checks if the file stream is in a good state.
	HX_INLINE bool good() const { return m_good; }

	// Checks if the end of the file has been reached.
	HX_INLINE bool eof() const { return m_eof; }

	// Clears the state of the file stream.
	HX_INLINE void clear() {
		m_good = m_filePImpl != hxnull;
		m_eof = false;
	}

	// Returns the current open mode of the file.
	HX_INLINE uint16_t mode() const { return m_openMode; }

	// Reads a specified number of bytes from the file into the provided buffer.
	size_t read(void* bytes_, size_t count_);

	// Writes a specified number of bytes from the provided buffer to the file.
	size_t write(const void* bytes_, size_t count_);

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on
	// EOF without needing to be hxFile::fallible.  Automatically determines
	// the size of the provided char array.  buffer is a reference to a char
	// array.
	template<size_t BufferSize_>
	HX_INLINE bool getLine(char(&buffer_)[BufferSize_]) { return getLine(buffer_, BufferSize_); }

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on EOF
	// without needing to be hxFile::fallible.
	bool getLine(char* buffer_, size_t bufferSize_);

	// Writes a formatted string to the file. Must be less than HX_MAX_LINE characters.
	bool print(const char* format_, ...) HX_ATTR_FORMAT(2, 3);

	// Reads a single unformatted native endian object from the file.
	template<typename T_>
	HX_INLINE bool read1(T_& t_) { return read(&t_, sizeof t_) == sizeof t_; }

	// Writes a single unformatted native endian object to the file.
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

	// Writes a string literal to the file. Supports Google Test style diagnostic messages.
	template<size_t StringLength_>
	HX_INLINE hxFile& operator<<(const char(&str_)[StringLength_]) {
		hxAssert(::strnlen(str_, StringLength_) == (StringLength_-1));
		write(str_, StringLength_-1);
		return *this;
	}

private:
	hxFile(const hxFile&); // = delete
	void operator=(const hxFile&); // = delete
	template<typename T_> HX_INLINE hxFile& operator>>(const T_* t_); // = delete

	// Internal function to open a file with a formatted filename and variable arguments.
	bool openv_(uint16_t mode_, const char* format_, va_list args_);

	char* m_filePImpl;   // Pointer to the file implementation.
	uint16_t m_openMode; // Current open mode of the file.
	bool m_good;         // Indicates if the file stream is in a good state.
	bool m_eof;	         // Indicates if the end of the file has been reached.
};
