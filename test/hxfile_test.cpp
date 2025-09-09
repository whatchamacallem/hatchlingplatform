// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// Console_test.cpp provides coverage for normal operation.

class hxfile_test : public testing::Test
{
public:
	class test_t_ {
	public:
		uint32_t a;
		int16_t b;
		uint8_t c;
		int8_t d;
	};
};

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST_F(hxfile_test, empty_name) {
	hxfile f(hxfile::in | hxfile::skip_asserts, "");
	EXPECT_EQ(f.good(), false);
	EXPECT_EQ(f.is_open(), false);
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST_F(hxfile_test, read_write) {
	hxfile f(hxfile::in | hxfile::out | hxfile::skip_asserts, "hxfile_test_read_write.txt");
	f << "hxfile_test_read_write.txt";

	EXPECT_EQ(f.good(), true);
	EXPECT_EQ(f.is_open(), true);

	hxout << "smoke test hxout" << ".";
	hxout.print("..\n");
	hxerr << "smoke test hxerr" << ".";
	hxerr.print("..\n");
}

TEST_F(hxfile_test, not_exist) {
	hxfile f(hxfile::in | hxfile::skip_asserts, "test_file_does_not_exist_%d", 123);
	EXPECT_EQ(f.good(), false);
	EXPECT_EQ(f.is_open(), false);

	// EXPECT_EQ should return hxdev_null.
	EXPECT_EQ((bool)(f.mode() & hxfile::skip_asserts), true)
		<< "dev/null should not exist";

	hxdev_null << "dev/null should not exist";
}

TEST_F(hxfile_test, operators) {
	hxfile f(hxfile::out | hxfile::skip_asserts, "hxfile_test_operators.bin");
	test_t_ x;
	int a = -3;
	x.a = 77777u;
	x.b = -555;
	x.c = 77u;
	x.d = -55;
	f <= x <= a;
	EXPECT_TRUE(f.good());
	EXPECT_FALSE(f.eof());
	f.close();

	f.open(hxfile::in | hxfile::skip_asserts, "hxfile_test_operators.bin");
	test_t_ y;
	int b;
	EXPECT_TRUE(f.good());
	f >= y >= b;
	EXPECT_EQ(y.a, 77777u);
	EXPECT_EQ(y.b, -555);
	EXPECT_EQ(y.c, 77u);
	EXPECT_EQ(y.d, -55);
	EXPECT_EQ(b, -3);

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
