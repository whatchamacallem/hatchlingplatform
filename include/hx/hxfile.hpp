#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

/// hxfile - RAII wrapper for file I/O. A mixture of unformatted std::basic_fstream
/// operations and formatted C-style text printing.
class hxfile {
public:
	/// open_mode::in/out are from std::ios_base::openmode and indicate I/O mode.
	/// open_mode::stdio provides access to stdio. open_mode::failable skips asserts
	/// and is similar to setting std::basic_ios::exceptions(0).
	enum open_mode {
		/// Open for binary reading.
		in = 1u,
		/// Open for binary writing.
		out = 2u,
		/// Access stdio as in or out but not both.
		stdio = 4u,
		/// Skip asserts.
		failable = 8u
	};

	/// Constructor to initialize the file object with a specific mode.
	/// For an unopened file use 0. For stdio use (stdio|in), (stdio|out).
	/// stdio may be failble.
	hxfile(uint16_t mode_=0u);

	/// Constructor to initialize and open a file with a formatted filename.
	/// Opens a stream using a formatted filename. Non-standard arg order.
	hxfile(uint16_t mode_, const char* filename_, ...) hxattr_format(3, 4);

	/// Destructor to ensure the file is closed when the object goes out of scope.
	~hxfile(void);

	/// Opens a file with the specified mode and formatted filename.
	bool open(uint16_t mode_, const char* filename_, ...) hxattr_format(3, 4);

	/// Closes the currently open file.
	void close(void);

	/// Checks if the file is open.
	hxconstexpr_fn bool is_open(void) const { return m_file_pimpl_ != hxnull; }

	/// Checks if the file stream is in a good state.
	hxconstexpr_fn bool good(void) const { return m_good_; }

	/// Checks if the end of the file has been reached.
	hxconstexpr_fn bool eof(void) const { return m_eof_; }

	/// Clears the state of the file stream.
	hxconstexpr_fn void clear(void) {
		m_good_ = m_file_pimpl_ != hxnull;
		m_eof_ = false;
	}

	/// Returns the current open mode of the file.
	hxconstexpr_fn uint16_t mode(void) const { return m_open_mode_; }

	/// Reads a specified number of bytes from the file into the provided buffer.
	/// - bytes: Pointer to the buffer where the read bytes will be stored.
	/// - count: Number of bytes to read from the file.
	size_t read(void* bytes_, size_t count_);

	/// Writes a specified number of bytes from the provided buffer to the file.
	/// - bytes: Pointer to the buffer containing the bytes to write.
	/// - count: Number of bytes to write to the file.
	size_t write(const void* bytes_, size_t count_);

	/// Reads an \n or EOF terminated character sequence. Allowed to fail on
	/// EOF without needing to be hxfile::failable. Automatically determines
	/// the size of the provided char array.
	/// - buffer: Reference to a char array where the line will be stored.
	template<size_t Buffer_size_>
	hxconstexpr_fn bool get_line(char(&buffer_)[Buffer_size_]) { return get_line(buffer_, Buffer_size_); }

	/// Reads an \n or EOF terminated character sequence. Allowed to fail on EOF
	/// without needing to be hxfile::failable.
	/// - buffer: Pointer to a char array where the line will be stored.
	/// - buffer_size: Size of the buffer array.
	bool get_line(char* buffer_, size_t buffer_size_);

	/// Writes a formatted string to the file. Must be less than HX_MAX_LINE characters.
	/// - format: Format string, similar to printf.
	/// - ...: Additional arguments for the format string.
	bool print(const char* format_, ...) hxattr_format(2, 3);

	/// Reads a single unformatted native endian object from the file.
	/// - t: Reference to the object where the data will be stored.
	template<typename T_>
	hxconstexpr_fn bool read1(T_& t_) { return read(&t_, sizeof t_) == sizeof t_; }

	/// Writes a single unformatted native endian object to the file.
	/// - t: Reference to the object containing the data to write.
	template<typename T_>
	hxconstexpr_fn bool write1(const T_& t_) { return write(&t_, sizeof t_) == sizeof t_; }

	/// Read a single unformatted native endian object from a stream. The operator
	/// >= is being used instead of >> to indicate there is no formatting.
	/// - t: Reference to the object where the data will be stored.
	template<typename T_>
	hxconstexpr_fn hxfile& operator>=(T_& t_) {
		read(&t_, sizeof t_);
		return *this;
	}

	/// Write a single unformatted native endian object to a stream. The operator
	/// <= is being used instead of << to indicate there is no formatting.
	/// - t: Reference to the object containing the data to write.
	template<typename T_>
	hxconstexpr_fn hxfile& operator<=(const T_& t_) {
		write(&t_, sizeof t_);
		return *this;
	}

	/// Writes a string literal to the file. Supports Google Test style diagnostic messages.
	/// - str: Reference to a string literal to write to the file.
	template<size_t string_length_>
	hxconstexpr_fn hxfile& operator<<(const char(&str_)[string_length_]) {
		hxassertmsg(::strlen(str_) == (string_length_-1), "bad_string");
		write(str_, string_length_-1);
		return *this;
	}

private:
	hxfile(const hxfile&) hxdelete_fn;
	void operator=(const hxfile&) hxdelete_fn;
	template<typename T_> hxconstexpr_fn hxfile& operator>>(const T_* t_) hxdelete_fn;

	// Internal function to open a file with a formatted filename and variable arguments.
	bool openv_(uint16_t mode_, const char* format_, va_list args_);

    char* m_file_pimpl_;   // Pointer to the file implementation.
    uint16_t m_open_mode_; // Current open mode of the file.
    bool m_good_; 		 // Indicates if the file stream is in a good state.
    bool m_eof_;  		 // Indicates if the end of the file has been reached.
};
