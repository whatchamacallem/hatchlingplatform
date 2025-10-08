#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxarray.hpp"
#include "hxutility.h"

#include <stdio.h>

/// A `hxstringstream` with a non-zero capacity always contains a valid '\0'
/// terminated C-style string. The '\0' is not included in the size and is
/// pointed directly to by the `hxstringstream::end` iterator. All modification
/// is required to preserve the '\0' termination of the string.
/// However reads and writes are possible before the end of the string. A
/// successful read of the final character does not set `eof`.
template<size_t capacity_=hxallocator_dynamic_capacity>
class hxstringstream : public hxarray<char, capacity_> {
public:
	hxstringstream(void) {
		::memset((void*)this, 0, sizeof(*this));
	}

	hxstringstream(hxstringstream&& other_) {
		::memcpy((void*)this, &other_, sizeof(other_));
		::memset((void*)&other_, 0x00, sizeof(other_));
	}

	~hxstringstream(void) {
		// Use the last byte as a guard word.
		hxassert(!m_end_capacity_ || *(m_end_capacity_ - 1) == '\0');
	}

	void operator=(hxstringstream&& other_) {
		hxswap_memcpy(other_); // Asserts &other != this.
	}

	operator bool(void) const { return !m_failed_; }

	bool fail(void) const { return m_failed_; }

	bool eof(void) const { return m_eof_; }

	void clear(void) {
		m_position_ = this->begin();
		if(m_position_) {
			m_position_[0] = '\0';
		}
		m_failed_ = false;
		m_eof_ = false;
	}

	/// Returns the read/write position in the string.
	size_t get_pos(void) const {
		return (size_t)(m_position_ - this->data());
	}

	/// Unable to set position beyond the end of the string. Capacity is irrelevant.
	bool set_pos(size_t position_) {
		if(position_ > this->size()) {
			m_failed_ = true;
			return false;
		}
		m_position_ = this->data() + position_;
		m_eof_ = false;
		return true;
	}

	// This is for reading binary data and not scanning text data. It may be
	// used to read the trailing '\0' at the end of the string. Any other
	// embedded '\0' characters will be copied as well. Does not '\0'-terminate
	// the sequence of bytes read unless the `\0` at the end of the string is
	// requested. Does not perform a partial read.
	size_t read(char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(this->data(), "hxstringstream Not allocated.");

		// Allow reading size + 1 because of the '\0'.
		size_t available_ = (size_t)(this->m_end_ - m_position_) + 1u;
		if(count_ > available_) {
			m_failed_ = true;
			m_eof_ = true;
			return 0u;
		}
		::memcpy(bytes_, m_position_, count_);
		m_position_ += count_;
		return count_;
	}

	/// Writes an array of `count` characters of binary data. Any included '\0'
	/// characters will be embedded in the stream. A '\0' will be appended
	/// beyond the end of the string if the end of the string is written to or
	/// overwritten. Capacity is required for the trailing '\0', however it is
	/// not include in the size and is pointed to by the `hxstringstream::end`
	/// iterator and will be overwritten by the next write.
	size_t write(const char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(this->data(), "hxstringstream Not allocated.");

		size_t available_capacity_ = (size_t)(m_end_capacity_ - m_position_);
		// Equal length is a problem due to the '\0'.
		if(count_ >= available_capacity_) {
			m_failed_ = true;
			return 0u;
		}
		::memcpy(m_position_, bytes_, count_);
		m_position_ += count_;
		if(m_position_ > this->m_end_) {
			this->m_end_ = m_position_;
			*m_position_ = '\0';
		}
		return count_;
	}

	template<size_t buffer_size_>
	bool getline(char(&buffer_)[buffer_size_]);
	bool getline(char* buffer_, int buffer_size_) hxattr_nonnull(2) hxattr_hot;

	template<size_t string_length_>
	hxstringstream& operator<<(const char(&str_)[string_length_]) {
		this->write(str_, string_length_ - 1u);
		return *this;
	}

	hxstringstream& operator<<(bool value_) { return this->write_fundamental_("%d", value_); }
	hxstringstream& operator<<(char value_) { return this->write_fundamental_("%c", value_); }
	hxstringstream& operator<<(signed char value_) { return this->write_fundamental_("%hhd", value_); }
	hxstringstream& operator<<(unsigned char value_) { return this->write_fundamental_("%hhu", value_); }
	hxstringstream& operator<<(short value_) { return this->write_fundamental_("%hd", (int)value_); }
	hxstringstream& operator<<(unsigned short value_) { return this->write_fundamental_("%hu", value_); }
	hxstringstream& operator<<(int value_) { return this->write_fundamental_("%d", value_); }
	hxstringstream& operator<<(unsigned int value_) { return this->write_fundamental_("%u", value_); }
	hxstringstream& operator<<(long value_) { return this->write_fundamental_("%ld", value_); }
	hxstringstream& operator<<(unsigned long value_) { return this->write_fundamental_("%lu", value_); }
	hxstringstream& operator<<(long long value_) { return this->write_fundamental_("%lld", value_); }
	hxstringstream& operator<<(unsigned long long value_) { return this->write_fundamental_("%llu", value_); }
	hxstringstream& operator<<(float value_) { return this->write_fundamental_("%g", value_); }
	hxstringstream& operator<<(double value_) { return this->write_fundamental_("%g", value_); }
	hxstringstream& operator<<(long double value_) { return this->write_fundamental_("%Lg", value_); }

	void reserve(size_t size_, hxsystem_allocator_t allocator_=hxsystem_allocator_current) {
		hxarray<char, capacity_>::reserve(size_, allocator_, sizeof(char));
		if(m_position_ == hxnull) {
			// Reading and writing start at the beginning of a '\0'-terminated string.
			m_position_ = this->data();
			if(m_position_ != hxnull) {
				m_position_[0] = '\0';
				m_end_capacity_ = m_position_ + this->capacity();
				// This is in case vsnprintf stops printing right before the last
				// character and does not add a '\0'.
				*(m_end_capacity_ - 1) = '\0';
			}
		}
	}

	hxstringstream(const hxstringstream&) = delete;
	void operator=(const hxstringstream&) = delete;
	template<typename T_> hxstringstream& operator>>(const T_* t_) = delete;

private:
	hxstringstream& write_fundamental_(const char* format_, ...) {
		const size_t available_capacity_ = (size_t)(m_end_capacity_ - m_position_);
		hxassert(available_capacity_ > 0u); // Room for trailing '\0' remains.
		va_list args_;
		va_start(args_, format_);
		const int count_ = ::vsnprintf(m_position_, available_capacity_, format_, args_);
		va_end(args_);
		if((size_t)count_ >= available_capacity_) {
			m_failed_ = true;
			return *this;
		}
		m_position_ += (size_t)count_;
		hxassert(*m_position_ == '\0'); // vsnprintf added a NUL.
		this->m_end_ = m_position_;
		return *this;
	}

	char* m_position_;
	char* m_end_capacity_; // Points to the last byte which is '\0'.
	bool m_failed_; // Has any error been encountered.
	bool m_eof_;
};
