#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxarray.hpp"
#include "hxutility.h"

/// A valid `hxstring_stream` with a non-zero capacity always contains a valid
/// '\0' terminated UTF-8 C-style string. That means `*strstr.end() == '\0'` as a
/// pre-condition and a post-condition of every operation. However reads and
/// writes are possible before the end of the string. A successful read of the
/// final character does not set `eof`.
template<size_t capacity_=hxallocator_dynamic_capacity>
class hxstring_stream : public hxarray<char, capacity_> {
public:
	hxstring_stream(void) {
		::memset((void*)this, 0, sizeof(*this));
	}

	hxstring_stream(hxstring_stream&& other_) {
		::memcpy((void*)this, &other_, sizeof(other_));
		::memset((void*)&other_, 0x00, sizeof(other_));
	}

	~hxstring_stream(void) = default;

	void operator=(hxstring_stream&& other_) {
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

	// Does not '\0'-terminate the sequence of bytes read. This is for lower
	// level use and not reading strings. (TODO.)
	size_t read(char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg((size_t)(m_position_ - this->data()) < this->size(), "hxstring_stream unallocated");

		// May or may not read '\0'.
		size_t available_ = (size_t)(this->m_end_ - m_position_);
		if(count_ > available_) {
			m_failed_ = true;
			m_eof_ = true;
			return 0u;
		}
		::memcpy(bytes_, m_position_, count_);
		m_position_ += count_;
		return count_;
	}

	/// '\0'-terminates the stored buffer after appending the sequence of bytes read.
	size_t write(const char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg((size_t)(m_position_ - this->data()) < this->size(), "hxstring_stream unallocated");

		// The -1 is to reserve space for the required trailing '\0'.
		size_t available_capacity = capacity_ - (size_t)(m_position_ - this->data()) - 1;
		if(count_ > available_capacity) {
			m_failed_ = true;
			return 0u;
		}
		::memcpy(m_position_, bytes_, count_);
		m_position_ += count_;
		*m_position_ = '\0';
		return count_;
	}

	template<size_t buffer_size_>
	bool getline(char(&buffer_)[buffer_size_]);
	bool getline(char* buffer_, int buffer_size_) hxattr_nonnull(2) hxattr_hot;

	template<size_t string_length_>
	hxstring_stream& operator<<(const char(&str_)[string_length_]) {
		this->write(str_, string_length_ - 1u);
		return *this;
	}

	void reserve(size_t size_, hxsystem_allocator_t allocator_=hxsystem_allocator_current) {
		hxarray<char, capacity_>::reserve(size_, allocator_, sizeof(char));
		m_position_ = this->m_end_;
		m_position_[0] = '\0';
	}

	hxstring_stream(const hxstring_stream&) = delete;
	void operator=(const hxstring_stream&) = delete;
	template<typename T_> hxstring_stream& operator>>(const T_* t_) = delete;

private:
	hxarray<char, hxallocator_dynamic_capacity> m_buffer_; // Contains capacity.
	char* m_position_;
	bool m_failed_; // Has any error been encountered.
	bool m_eof_; //
};
