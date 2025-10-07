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
	hxstring_stream(void) :
		m_buffer_(),
		m_size_(0u),
		m_position_(0u),
		m_failed_(false),
		m_eof_(false) {
	}

	hxstring_stream(hxstring_stream&& other_) :
		m_buffer_(hxmove(other_.m_buffer_)),
		m_size_(other_.m_size_),
		m_position_(other_.m_position_),
		m_failed_(other_.m_failed_),
		m_eof_(other_.m_eof_) {
		other_.m_size_ = 0u;
		other_.m_position_ = 0u;
		other_.m_failed_ = false;
		other_.m_eof_ = false;
	}

	~hxstring_stream(void) = default;

	void operator=(hxstring_stream&& other_) {
		if(this == &other_) { return; }
		m_buffer_ = hxmove(other_.m_buffer_);
		m_size_ = other_.m_size_;
		m_position_ = other_.m_position_;
		m_failed_ = other_.m_failed_;
		m_eof_ = other_.m_eof_;
		other_.m_size_ = 0u;
		other_.m_position_ = 0u;
		other_.m_failed_ = false;
		other_.m_eof_ = false;
	}

	operator bool(void) const { return !m_failed_; }

	bool fail_(void) const { return m_failed_; }
	bool eof(void) const { return m_eof_; }
	void clear(void) {
		m_size_ = 0u;
		m_position_ = 0u;
		m_failed_ = false;
		m_eof_ = false;
		char* data_ = m_buffer_.capacity() ? m_buffer_.data() : hxnull;
		if(data_) {
			data_[0] = '\0';
		}
	}

	size_t get_pos(void) const { return m_position_; }
	bool set_pos(size_t position_) {
		if(position_ > m_size_) {
			m_failed_ = true;
			return false;
		}
		m_position_ = position_;
		m_eof_ = (m_position_ >= m_size_);
		return true;
	}
	size_t read(char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(m_position_ <= m_size_, "stream_position_oob");
		if(count_ == 0u) {
			return 0u;
		}
		if(m_position_ >= m_size_) {
			m_eof_ = true;
			return 0u;
		}
		size_t available_ = m_size_ - m_position_;
		if(count_ > available_) {
			count_ = available_;
		}
		char* data_ = m_buffer_.capacity() ? m_buffer_.data() : hxnull;
		hxassertmsg(data_ || m_buffer_.capacity() == 0u, "buffer_unallocated");
		if(!data_) {
			m_failed_ = true;
			return 0u;
		}
		if(count_ == 0u) {
			m_eof_ = (m_position_ >= m_size_);
			return 0u;
		}
		::memcpy(bytes_, data_ + m_position_, count_);
		m_position_ += count_;
		m_eof_ = (m_position_ >= m_size_);
		return count_;
	}
	size_t write(const char* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot {
		hxassertmsg(m_position_ <= m_size_, "stream_position_oob");
		if(count_ == 0u) {
			return 0u;
		}
		size_t capacity_ = m_buffer_.capacity();
		if(capacity_ == 0u) {
			m_failed_ = true;
			return 0u;
		}
		hxassertmsg(m_size_ <= capacity_ - 1u, "string_size_overflow");
		if(m_position_ > capacity_ - 1u) {
			m_failed_ = true;
			return 0u;
		}
		size_t room_ = (capacity_ - 1u) - m_position_;
		if(count_ > room_) {
			m_failed_ = true;
			return 0u;
		}
		char* data_ = m_buffer_.data();
		hxassertmsg(data_, "buffer_unallocated");
		::memcpy(data_ + m_position_, bytes_, count_);
		m_position_ += count_;
		if(m_position_ > m_size_) {
			m_size_ = m_position_;
		}
		data_[m_size_] = '\0';
		m_eof_ = false;
		return count_;
	}

	template<size_t buffer_size_>
	bool getline(char(&buffer_)[buffer_size_]) {
		return this->getline(buffer_, (int)buffer_size_);
	}

	bool getline(char* buffer_, int buffer_size_) hxattr_nonnull(2) hxattr_hot {
		if(buffer_size_ <= 0) {
			m_failed_ = true;
			return false;
		}
		hxassertmsg(m_position_ <= m_size_, "stream_position_oob");
		if(m_position_ >= m_size_) {
			buffer_[0] = '\0';
			m_eof_ = true;
			return false;
		}
		size_t limit_ = (size_t)(buffer_size_ - 1);
		char* data_ = m_buffer_.capacity() ? m_buffer_.data() : hxnull;
		hxassertmsg(data_, "buffer_unallocated");
		if(limit_ == 0u && data_[m_position_] != '\n') {
			buffer_[0] = '\0';
			m_failed_ = true;
			return false;
		}
		size_t start_ = m_position_;
		size_t remaining_ = m_size_ - start_;
		size_t copy_ = 0u;
		while(copy_ < remaining_ && copy_ < limit_) {
			char ch_ = data_[start_ + copy_];
			if(ch_ == '\n') {
				break;
			}
			++copy_;
		}
		bool saw_newline_ = (copy_ < remaining_ && data_[start_ + copy_] == '\n');
		if(copy_ > 0u) {
			::memcpy(buffer_, data_ + start_, copy_);
		}
		buffer_[copy_] = '\0';
		m_position_ = start_ + copy_;
		if(saw_newline_) {
			++m_position_;
		}
		m_eof_ = (m_position_ >= m_size_);
		return saw_newline_ || copy_ > 0u;
	}

	template<size_t string_length_>
	hxstring_stream& operator<<(const char(&str_)[string_length_]) {
		this->write(str_, string_length_ - 1u);
		return *this;
	}

	void reserve(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT) {
		size_t capacity_ = size_ + 1u;
		if(capacity_ == 0u) {
			m_failed_ = true;
			return;
		}
		m_buffer_.reserve(capacity_, allocator_, alignment_);
		if(m_size_ > size_) {
			m_size_ = size_;
			if(m_position_ > m_size_) {
				m_position_ = m_size_;
			}
		}
		char* data_ = m_buffer_.capacity() ? m_buffer_.data() : hxnull;
		if(data_) {
			hxassertmsg(m_size_ < m_buffer_.capacity(), "string_size_overflow");
			data_[m_size_] = '\0';
		}
		m_eof_ = (m_position_ >= m_size_);
	}

	hxstring_stream(const hxstring_stream&) = delete;
	void operator=(const hxstring_stream&) = delete;
	template<typename T_> hxstring_stream& operator>>(const T_* t_) = delete;

private:

	hxarray<char, hxallocator_dynamic_capacity> m_buffer_; // Contains capacity.
	size_t m_size_;
	size_t m_position_;
	bool m_failed_; // Has any error been encountered.
	bool m_eof_; //
};
