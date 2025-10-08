// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxstringstream.hpp>
#include <hx/hxtest.hpp>
#include <hx/hxutility.h>

#include <string.h>

HX_REGISTER_FILENAME_HASH

TEST(hxstringstream_test, write_and_read_roundtrip) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxstringstream stream;
	stream.reserve(16u);
	const char payload_[] = "abc";
	EXPECT_EQ(stream.write(payload_, sizeof payload_ - 1u), 3u);
	EXPECT_EQ(stream.get_pos(), 3u);
	EXPECT_FALSE(stream.fail());
	EXPECT_TRUE(stream.set_pos(0u));
	char buffer_[4];
	EXPECT_EQ(stream.read(buffer_, 3u), 3u);
	buffer_[3] = '\0';
	EXPECT_STREQ(buffer_, "abc");
	EXPECT_FALSE(stream.eof());
	EXPECT_FALSE(stream.fail());
}

TEST(hxstringstream_test, write_fundamental_types) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxstringstream stream;
	stream.reserve(128u);
	stream.clear();

	auto expect_stream = [&](auto value_, const char* expected_) {
		stream.clear();
		stream << value_;
		EXPECT_STREQ(stream.data(), expected_);
	};

	expect_stream(true, "1");
	expect_stream(false, "0");
	expect_stream('Z', "Z");
	expect_stream(static_cast<signed char>(-5), "-5");
	expect_stream(static_cast<unsigned char>(250), "250");
	expect_stream(static_cast<short>(-123), "-123");
	expect_stream(static_cast<unsigned short>(456), "456");
	expect_stream(-7890, "-7890");
	expect_stream(67890u, "67890");
	expect_stream(-123456l, "-123456");
	expect_stream(123456ul, "123456");
	expect_stream(-9876543210ll, "-9876543210");
	expect_stream(9876543210ull, "9876543210");
	expect_stream(1.25f, "1.25");
	expect_stream(2.5, "2.5");
	expect_stream(0.5L, "0.5");
}

TEST(hxstringstream_test, insertion_uses_full_available_capacity) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxstringstream<8u> stream;
	stream.reserve(8u);
	stream << 1234567u; // Fits within seven data bytes plus the trailing NUL.
	EXPECT_FALSE(stream.fail());
	EXPECT_STREQ(stream.data(), "1234567");

	stream.clear();
	stream << 123456789u; // Would require nine data bytes; only seven are available.
	EXPECT_TRUE(stream.fail());
}
