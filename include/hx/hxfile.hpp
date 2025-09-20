#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

class hxfile;

/// Global reference to stdin or equivalent.
extern hxfile hxin;

/// Global reference to stdout or equivalent.
extern hxfile hxout;

/// Global reference to stderr or equivalent.
extern hxfile hxerr;

/// Global equivalent to /dev/null. May be written to but not read from.
extern hxfile hxdev_null;

/// Equivalent to `std::endl` without the flush. Does not change per-platform.
#define hxendl "\n"

/// `hxfile` - Single ownership C++ RAII abstraction for C-style `FILE*` I/O.
/// Provides a mixture of unformatted binary stream operations and formatted
/// `printf`/`scanf` style I/O. Provides optional error handling. `gcc` is
/// useful for validating `printf`/`scanf` style arguments. However, memory
/// imaged data structres are still recommended. Formatted I/O is intended to
/// use UTF-8 with no carrige return.
///
/// To switch to using a different implementation just use an alternate .cpp
/// file for your target. Allows for `hxerr` to be a serial port and file I/O
/// to use a DMA controller.
///
/// NOTA BENE: `get_position`/`set_position` over 2 GiB is not supported on
/// Windows. As well, `size_t` is limited to 4 GiB on all 32-bit platforms.
class hxfile {
public:
	/// `open_mode` - Flags indicating how the file is to be used. Modifying or
	/// appending to an existing file is not supported.
	enum open_mode : uint8_t {
		/// No flags.
		none = 0u,
		/// Open for binary reading. E.g. "rb".
		in = 1u,
		/// Open for binary writing. Replaces any existing file with an empty
		/// one even if `in` is used at the same time. E.g. "wb".
		out = 2u,
		/// By default, any unexpected failure results in an assert. To allow
		/// reasonably unforeseen asserts to be skipped, set skip_asserts. Bad
		/// parameters (e.g., writing to a file that is not open, was not opened
		/// to be written to, or providing a null buffer) will still result in
		/// assertions. E.g. "w+b".
		skip_asserts = 4u
	};

	/// Default constructs as a closed file.
	hxfile(void) {
		::memset((void*)this, 0x00, sizeof *this);
	}

	/// Constructor to initialize and open a file with a formatted filename.
	/// Opens a stream using a formatted filename. Non-standard arg order.
	hxfile(uint8_t mode_, const char* filename_, ...) hxattr_format_printf(3, 4);

	/// The constructor to initialize the file object with an unowned
	/// implementation object and a specific mode. No checks. Use `hxin`,
	/// `hxout`, `hxerr` and `hxdev_null` instead.
	hxfile(void* file_, uint8_t mode_);

	// Move constructor. No copy constructor is provided.
	hxfile(hxfile&& file_) {
		::memcpy((void*)this, &file_, sizeof file_);
		::memset((void*)&file_, 0x00, sizeof file_);
	}

	/// Destructor to ensure the file is closed when the object goes out of
	/// scope.
	~hxfile();

	// Move operator=. No assignment operator is provided.
	void operator=(hxfile&& file_);

	/// Opens a file with the specified mode and formatted filename.
	bool open(uint8_t mode_, const char* filename_, ...) hxattr_format_printf(3, 4);

	/// Closes the currently open file.
	void close(void);

	/// Checks if the file is open.
	bool is_open(void) const { return m_file_pimpl_ != hxnull; }

	/// Checks if the file is open, EOF has not been reached and no error
	/// encountered.
	bool good(void) const { return m_good_; }

	/// Checks if EOF has been reached.
	bool eof(void) const { return m_eof_; }

	/// Resets the goodness and EOF flags.
	void clear(void) {
		m_good_ = m_file_pimpl_ != hxnull;
		m_eof_ = false;
	}

	/// Returns the current open mode of the file.
	uint8_t mode(void) const { return m_open_mode_; }

	/// Returns current position in file if open, 0 otherwise. FILE*
	/// implementation requires a 64-bit long to support 64-bit files.
	size_t get_pos(void) const;

	/// Sets current position in file. Returns true on success. FILE*
	/// implementation requires a 64-bit long to support 64-bit files.
	bool set_pos(size_t position_);

	/// Reads a specified number of bytes from the file into the provided
	/// buffer.
	/// - `bytes` : Pointer to the buffer where the read bytes will be stored.
	/// - `count` : Number of bytes to read from the file.
	size_t read(void* bytes_, size_t count_);

	/// Writes a specified number of bytes from the provided buffer to the file.
	/// Writing will be skipped when using hxdev_null.
	/// - `bytes` : Pointer to the buffer containing the bytes to write.
	/// - `count` : Number of bytes to write to the file.
	size_t write(const void* bytes_, size_t count_);

	/// Reads an `\n` or `EOF` terminated character sequence. Allowed to fail on
	/// `EOF` without needing to be `hxfile::skip_asserts`. Automatically
	/// determines the size of the provided char array.
	/// - `buffer` : Reference to a char array where the line will be stored.
	template<size_t buffer_size_>
	bool get_line(char(&buffer_)[buffer_size_]) {
		return this->get_line(buffer_, buffer_size_);
	}

	/// Reads an `\n` or `EOF` terminated character sequence. Allowed to fail on
	/// `EOF` without needing to be `hxfile::skip_asserts`.
	/// - `buffer` : Pointer to a char array where the line will be stored.
	/// - `buffer_size` : Size of the buffer array.
	bool get_line(char* buffer_, int buffer_size_);

	/// Writes a formatted UTF-8 string to the file. Uses printf conventions.
	/// Formatting and writing will be skipped when using hxdev_null.
	/// - `format` : Format string, similar to printf.
	/// - `...` Additional arguments for the format string.
	bool print(const char* format_, ...) hxattr_format_printf(2, 3);

	/// Reads a formatted UTF-8 string from the file. Uses scanf conventions. Returns
	/// the number of items matched or a negative value on EOF or failure. Use
	/// hxfile::skip_asserts to read until EOF.
	/// - `format` : Format string, similar to scanf.
	/// - `...` Additional arguments for the format string.
	int scan(const char* format_, ...) hxattr_format_scanf(2, 3);

	/// Reads a single unformatted native endian object from the file.
	/// - `t` : Reference to the object where the data will be stored.
	template<typename T_>
	bool read1(T_& t_) { return this->read(&t_, sizeof t_) == sizeof t_; }

	/// Writes a single unformatted native endian object to the file.
	/// - `t` : Reference to the object containing the data to write.
	template<typename T_>
	bool write1(const T_& t_) { return this->write(&t_, sizeof t_) == sizeof t_; }

	/// Read a single unformatted native endian object from a stream. The
	/// operator `>=` is being used instead of `>>` to indicate there is no
	/// formatting.
	/// - `t` : Reference to the object where the data will be stored.
	template<typename T_>
	hxfile& operator>=(T_& t_) {
		this->read(&t_, sizeof t_);
		return *this;
	}

	/// Write a single unformatted native endian object to a stream. The
	/// operator `<=` is being used instead of `<<` to indicate there is no
	/// formatting.
	/// - `t` : Reference to the object containing the data to write.
	template<typename T_>
	hxfile& operator<=(const T_& t_) {
		this->write(&t_, sizeof t_);
		return *this;
	}

	/// Writes a string literal to the file. Supports Google Test style
	/// diagnostic messages in hxtest.
	/// - `str` : Reference to a string literal to write to the file.
	template<size_t string_length_>
	hxfile& operator<<(const char(&str_)[string_length_]) {
		hxassertmsg(::strlen(str_) == (string_length_-1), "bad_string_literal");
		this->write(str_, string_length_-1);
		return *this;
	}

	hxfile(const hxfile&) = delete;
	void operator=(const hxfile&) = delete;
	template<typename T_> hxfile& operator>>(const T_* t_) = delete; // use >=

private:
	// Internal function to open a file with a formatted filename and variable
	// arguments.
	bool openv_(uint8_t mode_, const char* format_, va_list args_);

	void* m_file_pimpl_;   // FILE* file pointer.
	uint8_t m_open_mode_;  // Current open_mode flags.
	bool m_owns_;  		   // Indicates if the FILE* is owned.
	bool m_good_; 		   // Indicates is open, not EOF nor errors.
	bool m_eof_;  		   // Indicates EOF.
};
