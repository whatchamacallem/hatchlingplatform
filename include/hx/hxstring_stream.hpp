#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"
#include "hxarray.hpp"
#include "hxutility.h"

#include <errno.h>
#include <stdio.h>

class hxstring_stream {
public:
	hxstring_stream(void);
	hxstring_stream(hxstring_stream&& other_);
	~hxstring_stream(void);

	void operator=(hxstring_stream&& other_);

	operator bool(void) const { return m_good_; }

	bool good(void) const { return m_good_; }
	bool eof(void) const { return m_eof_; }
	void clear(void);

	size_t get_pos(void) const { return m_position_; }
	bool set_pos(size_t position_);
	size_t read(void* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot;
	size_t write(const void* bytes_, size_t count_) hxattr_nonnull(2) hxattr_hot;

	template<size_t buffer_size_>
	bool getline(char(&buffer_)[buffer_size_]) {
		return this->getline(buffer_, (int)buffer_size_);
	}

	bool getline(char* buffer_, int buffer_size_) hxattr_nonnull(2) hxattr_hot;

	bool print(const char* format_, ...) hxattr_format_printf(2, 3) hxattr_hot;
	int scan(const char* format_, ...) hxattr_format_scanf(2, 3) hxattr_hot;

	template<typename T_>
	hxstring_stream& operator>=(T_& t_) {
		this->read(&t_, sizeof t_);
		return *this;
	}

	template<typename T_>
	hxstring_stream& operator<=(const T_& t_) {
		this->write(&t_, sizeof t_);
		return *this;
	}

	template<size_t string_length_>
	hxstring_stream& operator<<(const char(&str_)[string_length_]) {
		this->write(str_, string_length_ - 1u);
		return *this;
	}

	void reserve(size_t size_,
			hxsystem_allocator_t allocator_=hxsystem_allocator_current,
			hxalignment_t alignment_=HX_ALIGNMENT);

	hxstring_stream(const hxstring_stream&) = delete;
	void operator=(const hxstring_stream&) = delete;
	template<typename T_> hxstring_stream& operator>>(const T_* t_) = delete;

private:

	hxarray<char, hxallocator_dynamic_capacity> m_buffer_; // Contains capacity.
	size_t m_size_;
	size_t m_position_;
	bool m_good_; // Has an error been encountered.
	bool m_eof_; //
};
