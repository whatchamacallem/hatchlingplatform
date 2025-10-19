// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>
#include <hx/hxutility.h>

HX_REGISTER_FILENAME_HASH

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST(hxfile_test, empty_name_rejects_empty_path) {
	// "Constructs and opens a file with a formatted filename." Empty path under skip_asserts should trip failure.
	hxfile f(hxfile::in | hxfile::skip_asserts, "");
	EXPECT_EQ(f.fail(), true);
	EXPECT_EQ(f.is_open(), false);
	EXPECT_EQ(f, false);
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxfile_test, read_write_round_trip) {
	if(hxfile f = hxfile(hxfile::in | hxfile::out | hxfile::skip_asserts, "hxfile_test_read_write.txt")) {
		f << "hxfile_test_read_write.txt";

		EXPECT_EQ(f.fail(), false);
		EXPECT_EQ(f.is_open(), true);
		EXPECT_EQ(f, true);
		EXPECT_TRUE(f.flush());
		EXPECT_FALSE(f.fail());
	}
	else {
		ADD_FAILURE();
	}

	hxout << "smoke test hxout" << ".";
	hxout.print("..");
	hxout << hxendl;
	EXPECT_TRUE(hxout.flush());
	EXPECT_FALSE(hxout.fail());
	hxerr << "smoke test hxerr" << ".";
	hxerr.print("..");
	hxerr << hxendl;
	EXPECT_TRUE(hxerr.flush());
	EXPECT_FALSE(hxerr.fail());
}

TEST(hxfile_test, missing_file_reports_expectations) {
	hxfile f(hxfile::in | hxfile::skip_asserts, "test-file-does-not-exist-%d", 123);
	EXPECT_EQ(f.fail(), true);
	EXPECT_EQ(f.is_open(), false);

	// EXPECT_EQ should return hxdev_null.
	EXPECT_EQ((bool)(f.mode() & hxfile::skip_asserts), true)
		<< "dev/null should not exist";

	hxdev_null << "dev/null should not exist";
}

TEST(hxfile_test, seek_and_read_maintain_state) {
	// Write a test file to exercise get/set_position and read1/write1.

	struct hxfile_test_arbitrary_t { uint32_t x; } a { 0xefefefefu }, b { 0x01020304u }, c { 0x0u };

	// Write the expected value surrounded by poison.
	hxfile f(hxfile::in | hxfile::out, "hxfile_test_offset.bin");
	f.write1(a);
	f.write1(b);
	f.write1(a);

	// "Returns the current position in the file." After 3 structs expect 12 bytes, then seek into middle block.
	EXPECT_EQ(f.get_pos(), 12u);
	f.set_pos(4);
	EXPECT_TRUE(!f.fail());
	EXPECT_FALSE(f.eof());
	// "Reads a single unformatted native-endian object from the file." Confirm middle payload survives.
	f.read1(c);
	EXPECT_TRUE(!f.fail());
	EXPECT_FALSE(f.eof());

	EXPECT_EQ(b.x, c.x);
	EXPECT_EQ(f.get_pos(), 8u);
}

TEST(hxfile_test, move_copy_and_stream_operators) {
	// Write a test file and exercise the copy operators.

	class hxfile_test_record {
	public:
		uint32_t a;
		int16_t b;
		uint8_t c;
		int8_t d;
	};

	// C++17 uses "guaranteed copy elision," requiring hxmove here to invoke the
	// constructor from a temporary correctly.
	hxfile ft(hxfile::out | hxfile::skip_asserts, "hxfile_test_operators.bin");
	// "Move constructor. No copy constructor is provided." Source handle closes once transferred.
	hxfile f(hxmove(ft));
	EXPECT_TRUE(!ft.is_open());
	EXPECT_TRUE(f.is_open());
	hxfile_test_record x { 77777u, -555, 77u, -55 };
	int a = -3;

	// "Writes a single unformatted native-endian object to a stream." Chain writes for struct+int payload.
	f <= x <= a;
	// "Writes a formatted UTF-8 string to the file." Append text payload for later scan.
	f.print("(%d,%d)", 30, 70);

	EXPECT_TRUE(!f.fail());
	EXPECT_FALSE(f.eof());
	f.close();

	// Read the test file and verify.

	// "Opens a file with the specified mode and formatted filename." Rehydrate into fresh handle.
	ft.open(hxfile::in | hxfile::skip_asserts, "hxfile_test_operators.bin");
	f = hxmove(ft);
	EXPECT_TRUE(!ft.is_open());
	EXPECT_TRUE(f.is_open());

	hxfile_test_record y;
	::memset(&y, 0x00, sizeof y);
	int b = 0;
	int thirty = 0;
	int seventy = 0;

	// "Reads a single unformatted native-endian object from a stream." Recover struct + trailing int.
	f >= y >= b;
	// "Reads a formatted UTF-8 string from the file. Uses scanf conventions." Parse text footer.
	f.scan("(%d,%d)", &thirty, &seventy);

	// Expect struct contents -> { 77777u, -555, 77u, -55 } and trailing int -3.
	EXPECT_EQ(y.a, 77777u);
	EXPECT_EQ(y.b, -555);
	EXPECT_EQ(y.c, 77u);
	EXPECT_EQ(y.d, -55);
	EXPECT_EQ(b, -3);

	EXPECT_EQ(thirty, 30);
	EXPECT_EQ(seventy, 70);

	EXPECT_TRUE(!f.fail());
	EXPECT_FALSE(f.eof());
	char t;
	const size_t extra_byte = f.read(&t, 1); // This call fails.
	EXPECT_TRUE(f.eof());
	EXPECT_EQ(extra_byte, 0u);
	EXPECT_TRUE(f.fail());
	// "Resets the failure and EOF flags." Clear after synthetic EOF before final close.
	f.clear();
	EXPECT_TRUE(!f.fail()); // Clear the EOF event.
	f.close();
	EXPECT_TRUE(!f.is_open());
}

TEST(hxfile_test, eof_variants) {
	const char filename[] = "hxfile_test_empty.bin";

	// Create an empty test file.
	{
		hxfile writer(hxfile::out | hxfile::skip_asserts, filename);
		EXPECT_TRUE(writer.is_open());
		EXPECT_FALSE(writer.fail());
	}

	// Keep all the EOF conditions under test.

	{
		hxfile reader(hxfile::in | hxfile::skip_asserts, filename);
		uint8_t buffer[4];
		EXPECT_TRUE(reader.is_open());
		EXPECT_FALSE(reader.fail());
		EXPECT_EQ(reader.read(buffer, sizeof buffer), 0u);
		EXPECT_TRUE(reader.fail());
		EXPECT_TRUE(reader.eof());
	}

	{
		hxfile reader(hxfile::in | hxfile::skip_asserts, filename);
		char line[8];
		EXPECT_TRUE(reader.is_open());
		EXPECT_FALSE(reader.fail());
		EXPECT_FALSE(reader.getline(line));
		EXPECT_TRUE(reader.fail());
		EXPECT_TRUE(reader.eof());
	}

	{
		hxfile reader(hxfile::in | hxfile::skip_asserts, filename);
		int scanned = 0;
		EXPECT_TRUE(reader.is_open());
		EXPECT_FALSE(reader.fail());
		const int result = reader.scan("%d", &scanned);
		EXPECT_EQ(result, -1); // EOF == -1
		EXPECT_TRUE(reader.fail());
		EXPECT_TRUE(reader.eof());
	}
}
