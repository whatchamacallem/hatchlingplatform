#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxarray.hpp"
#include "hxutility.h"

#include <string.h>

class hxstring_stream {
public:
	hxstring_stream(void) {
		::memset((void*)this, 0, sizeof(*this));
	}

	hxstring_stream(hxstring_stream&& other_) {
		::memcpy((void*)this, &other_, sizeof(other_));
		::memset((void*)&other_, 0, sizeof(other_));
	}

	~hxstring_stream(void) = default;

	void operator=(hxstring_stream&& other_) {
		if(this == &other_) { return; }
		hxstring_stream temporary_(hxmove(*this));
		::memcpy((void*)this, &other_, sizeof(other_));
		::memset((void*)&other_, 0, sizeof(other_));
		(void)temporary_;
	}

	operator bool(void) const { return !m_failed_; }

	bool fail_(void) const { return m_failed_; }
	bool eof(void) const { return m_eof_; }
	void clear(void) {
		m_position_ = 0u;
		m_failed_ = false;
		m_eof_ = false;
	}

	size_t get_pos(void) const { return m_position_ - m_buffer_.data(); }
	bool set_pos(size_t position_) {
		if(position_ > m_buffer_.size()) {
			m_failed_ = true;
			return false;
		}
		m_position_ = m_buffer_.data() + position_;
		m_eof_ = false;
		return true;
	}

	size_t read(char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(m_position_ >= m_buffer_.data() && m_position_ < m_buffer.end(),
			"hxstring_stream unallocated");

		// May or may not read NUL.
		size_t available_ = (size_t)(m_buffer.end() - m_position_);
		if(count_ > available_) {
			m_failed_ = true;
			m_eof_ = true;
			return 0u;
		}
		::memcpy(bytes_, m_position_, count_);
		m_position_ += count_;
		return count_;
	}

	size_t write(const char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(m_position_ >= m_buffer_.data() && m_position_ < m_buffer.end(),
			"hxstring_stream unallocated");

		// May or may not write NUL or overwrite the last NUL.
		size_t available_ = (size_t)(m_buffer.end() - m_position_);
		if(count_ > available_) {
			m_failed_ = true;
			return 0u;
		}
		::memcpy(m_position_, bytes_, count_);
		m_position_ += count_;
		return count_;
	}

	template<size_t buffer_size_>
	bool getline(char(&buffer_)[buffer_size_]) {
		return this->getline(buffer_, (int)buffer_size_);
	}

	bool getline(char* buffer_, int buffer_size_) hxattr_nonnull(2) hxattr_hot;
	}

	template<size_t string_length_>
	hxstring_stream& operator<<(const char(&str_)[string_length_]) {
		this->write(str_, string_length_ - 1u);
		return *this;
	}

	void reserve(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		m_buffer_.reserve(size_, allocator_, alignment_);
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
