// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// Console_test.cpp provides coverage for normal operation.

namespace {

struct hxfile_test_record {
	uint32_t a;
	int16_t b;
	uint8_t c;
	int8_t d;
};

} // namespace

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST(hxfile_io, empty_name_rejects_empty_path) {
	hxfile f(hxfile::in | hxfile::skip_asserts, "");
	EXPECT_EQ(f.good(), false);
	EXPECT_EQ(f.is_open(), false);
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxfile_io, read_write_round_trip) {
	if(hxfile f = hxfile(hxfile::in | hxfile::out | hxfile::skip_asserts, "hxfile_test_read_write.txt")) {
		f << "hxfile_test_read_write.txt";

		EXPECT_EQ(f.good(), true);
		EXPECT_EQ(f.is_open(), true);
	}
	else {
		FAIL();
	}

	hxout << "smoke test hxout" << ".";
	hxout.print("..");
	hxout << hxendl;
	hxerr << "smoke test hxerr" << ".";
	hxerr.print("..");
	hxerr << hxendl;
}

TEST(hxfile_io, missing_file_reports_expectations) {
	hxfile f(hxfile::in | hxfile::skip_asserts, "test_file_does_not_exist_%d", 123);
	EXPECT_EQ(f.good(), false);
	EXPECT_EQ(f.is_open(), false);

	// EXPECT_EQ should return hxdev_null.
	EXPECT_EQ((bool)(f.mode() & hxfile::skip_asserts), true)
		<< "dev/null should not exist";

	hxdev_null << "dev/null should not exist";
}

TEST(hxfile_io, seek_and_read_maintain_state) {
	// Write a test file. Test get/set_position and read1/write1.

	struct { uint32_t x; } a { 0xefefefefu }, b { 0x01020304u }, c { 0x0u };

	// Write expected value surrounded by poison.
	hxfile f(hxfile::in | hxfile::out, "hxfile_test_offset.bin");
	f.write1(a);
	f.write1(b);
	f.write1(a);

	EXPECT_EQ(f.get_pos(), 12u);
	f.set_pos(4);
	EXPECT_TRUE(f.good());
	EXPECT_FALSE(f.eof());
	f.read1(c);
	EXPECT_TRUE(f.good());
	EXPECT_FALSE(f.eof());

	EXPECT_EQ(b.x, c.x);
	EXPECT_EQ(f.get_pos(), 8u);
}

TEST(hxfile_io, move_copy_and_stream_operators) {
	// Write a test file. Take the copy operators for a spin.

	// C++17 Uses "guaranteed copy elision" requiring hxmove here to invoke
	// the constructor from a temporary correctly.
	hxfile ft(hxfile::out | hxfile::skip_asserts, "hxfile_test_operators.bin");
	hxfile f(hxmove(ft));
	EXPECT_FALSE(ft.good());
	hxfile_test_record x { 77777u, -555, 77u, -55 };
	int a = -3;

	f <= x <= a;
	f.print("(%d,%d)", 30, 70);

	EXPECT_TRUE(f.good());
	EXPECT_FALSE(f.eof());
	f.close();

	// Read the test file and verify.

	ft.open(hxfile::in | hxfile::skip_asserts, "hxfile_test_operators.bin");
	f = hxmove(ft);
	EXPECT_TRUE(f.good());
	EXPECT_FALSE(ft.good());

	hxfile_test_record y;
	::memset(&y, 0x00, sizeof y);
	int b = 0;
	int thirty = 0;
	int seventy = 0;

	f >= y >= b;
	f.scan("(%d,%d)", &thirty, &seventy);

	EXPECT_EQ(y.a, 77777u);
	EXPECT_EQ(y.b, -555);
	EXPECT_EQ(y.c, 77u);
	EXPECT_EQ(y.d, -55);
	EXPECT_EQ(b, -3);

	EXPECT_EQ(thirty, 30);
	EXPECT_EQ(seventy, 70);

	EXPECT_TRUE(f.good());
	EXPECT_FALSE(f.eof());
	char t;
	size_t extra_byte = f.read(&t, 1); // fails!
	EXPECT_TRUE(f.eof());
	EXPECT_EQ(extra_byte, 0u);
	EXPECT_FALSE(f.good());
	f.clear();
	EXPECT_TRUE(f.good()); // clear EOF event.
	f.close();
	EXPECT_FALSE(f.good());
}
