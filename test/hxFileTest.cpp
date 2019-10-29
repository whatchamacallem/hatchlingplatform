// Copyright 2018 Adrian Johnston

#include <hx/hxFile.h>
#include <hx/hxTest.h>

HX_REGISTER_FILENAME_HASH

// ConsoleTest.cpp provides coverage for normal operation.

// ----------------------------------------------------------------------------

class hxFileTest :
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

// ----------------------------------------------------------------------------
#if __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST(hxFileTest, EmptyName) {
	hxFile f(hxFile::in | hxFile::fallible, "");
	ASSERT_EQ(f.good(), false);
	ASSERT_EQ(f.is_open(), false);
}

#if __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxFileTest, ReadWrite) {
	hxFile f(hxFile::in | hxFile::out | hxFile::fallible, "hxFileTest_ReadWrite.txt");
	f << "hxFileTest_ReadWrite.txt";

	ASSERT_EQ(f.good(), true);
	ASSERT_EQ(f.is_open(), true);
}

TEST(hxFileTest, NotExist) {
	hxFile f(hxFile::in | hxFile::fallible, "TEST_FILE_DOES_NOT_EXIST_%d", 123);
	ASSERT_EQ(f.good(), false);
	ASSERT_EQ(f.is_open(), false);
}

TEST_F(hxFileTest, Operators) {
	hxFile f(hxFile::out | hxFile::fallible, "hxFileTest_Operators.bin");
	X x;
	int a = -3;
	x.a = 77777u;
	x.b = -555;
	x.c = 77u;
	x.d = -55;
	f << x << a;
	ASSERT_TRUE(f.good());
	ASSERT_FALSE(f.eof());
	f.close();

	f.open(hxFile::in | hxFile::fallible, "hxFileTest_Operators.bin");
	X y;
	int b;
	ASSERT_TRUE(f.good());
	f >> y >> b;
	ASSERT_EQ(y.a, 77777u);
	ASSERT_EQ(y.b, -555);
	ASSERT_EQ(y.c, 77u);
	ASSERT_EQ(y.d, -55);
	ASSERT_EQ(b, -3);

	ASSERT_TRUE(f.good());
	ASSERT_FALSE(f.eof());
	char t;
	size_t extraByte = f.read(&t, 1); // fails!
	ASSERT_TRUE(f.eof());
	ASSERT_EQ(extraByte, 0u);
	ASSERT_FALSE(f.good());
	f.clear();
	ASSERT_TRUE(f.good()); // clear EOF event.
	f.close();
	ASSERT_FALSE(f.good());
}
