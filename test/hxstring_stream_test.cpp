// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxstring_stream.hpp>
#include <hx/hxtest.hpp>
#include <hx/hxutility.h>

#include <string.h>

HX_REGISTER_FILENAME_HASH

TEST(hxstring_stream_test, write_and_read_roundtrip) {
	hxstring_stream stream;
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
