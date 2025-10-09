// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxstringstream.hpp>
#include <hx/hxstringstream.hpp>
#include <hx/hxtest.hpp>


HX_REGISTER_FILENAME_HASH

TEST(hxstringstream_test, capacity_off_by_1) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxstringstream<8u> stream;
	stream.reserve(8u);
	stream << 1234567u; // Fits within seven data bytes plus the trailing NUL.
	EXPECT_FALSE(stream.fail());
	EXPECT_STREQ(stream.data(), "1234567");

	stream.clear();
	stream << 12345678u; // Would require 8 non-NULL bytes; only seven are available.
	EXPECT_TRUE(stream.fail());

	stream.clear();
	const char payload[] = "abcdefg";
	EXPECT_EQ(stream.write(payload, sizeof payload - 1u), 7u);
	EXPECT_TRUE(stream.set_pos(0u));
	char buffer[8];
	EXPECT_EQ(stream.read(buffer, sizeof buffer), sizeof buffer);
	EXPECT_STREQ(buffer, "abcdefg");
	EXPECT_FALSE(stream.fail());
	EXPECT_FALSE(stream.eof());
	EXPECT_EQ(stream.read(buffer, 1u), 0u);
	EXPECT_TRUE(stream.fail());
	EXPECT_TRUE(stream.eof());
}

#if HX_CPLUSPLUS >= 201703L // C++17 only.
TEST(hxstringstream_test, write_fundamental_types) {
	hxsystem_allocator_scope temporary_stack_scope(hxsystem_allocator_temporary_stack);
	hxstringstream stream;
	stream.reserve(128u);
	stream.clear();

	auto expect_stream = [&](auto value, const char* expected) {
		stream.clear();
		stream << value;
		EXPECT_STREQ(stream.data(), expected);
	};

	expect_stream(true, "1");
	expect_stream(false, "0");
	expect_stream('Z', "Z");
	expect_stream((signed char)-5, "-5");
	expect_stream((unsigned char)250, "250");
	expect_stream((short)-123, "-123");
	expect_stream((unsigned short)456, "456");
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
#endif // HX_CPLUSPLUS >= 201402L
