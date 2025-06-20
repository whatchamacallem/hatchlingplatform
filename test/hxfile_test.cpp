// Copyright 2017-2025 Adrian Johnston

#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// Console_test.cpp provides coverage for normal operation.

class hxfile_test :
	public testing::Test
{
public:
	struct X {
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

TEST_F(hxfile_test, Empty_name) {
	hxfile f(hxfile::in | hxfile::failable, "");
	ASSERT_EQ(f.good(), false);
	ASSERT_EQ(f.is_open(), false);
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST_F(hxfile_test, Read_write) {
	hxfile f(hxfile::in | hxfile::out | hxfile::failable, "hxfile_test_read_write.txt");
	f << "hxfile_test_read_write.txt";

	ASSERT_EQ(f.good(), true);
	ASSERT_EQ(f.is_open(), true);
}

TEST_F(hxfile_test, Not_exist) {
	hxfile f(hxfile::in | hxfile::failable, "test_file_does_not_exist_%d", 123);
	ASSERT_EQ(f.good(), false);
	ASSERT_EQ(f.is_open(), false);
	ASSERT_EQ((bool)(f.mode() & hxfile::failable), true);
}

TEST_F(hxfile_test, Operators) {
	hxfile f(hxfile::out | hxfile::failable, "hxfile_test_operators.bin");
	X x;
	int a = -3;
	x.a = 77777u;
	x.b = -555;
	x.c = 77u;
	x.d = -55;
	f <= x <= a;
	ASSERT_TRUE(f.good());
	ASSERT_FALSE(f.eof());
	f.close();

	f.open(hxfile::in | hxfile::failable, "hxfile_test_operators.bin");
	X y;
	int b;
	ASSERT_TRUE(f.good());
	f >= y >= b;
	ASSERT_EQ(y.a, 77777u);
	ASSERT_EQ(y.b, -555);
	ASSERT_EQ(y.c, 77u);
	ASSERT_EQ(y.d, -55);
	ASSERT_EQ(b, -3);

	ASSERT_TRUE(f.good());
	ASSERT_FALSE(f.eof());
	char t;
	size_t extra_byte = f.read(&t, 1); // fails!
	ASSERT_TRUE(f.eof());
	ASSERT_EQ(extra_byte, 0u);
	ASSERT_FALSE(f.good());
	f.clear();
	ASSERT_TRUE(f.good()); // clear EOF event.
	f.close();
	ASSERT_FALSE(f.good());
}
