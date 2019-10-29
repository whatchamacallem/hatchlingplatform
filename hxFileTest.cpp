// Copyright 2018 Adrian Johnston

#include "hxFile.h"
#include "hxTest.h"

// hxDeterministicReplayTest.cpp and ConsoleTest.cpp provide basic coverage.

// ----------------------------------------------------------------------------------

class hxFileTest :
	public testing::test
{
public:
	struct X {
		uint32_t a;
		int16_t b;
		uint8_t c;
		int8_t d;
	};
};

// ----------------------------------------------------------------------------------

TEST_F(hxFileTest, NotExist) {
	hxWarn("TEST_EXPECTING_WARNINGS:");
	hxFile f((hxFile::in | hxFile::fallible), "TEST_FILE_DOES_NOT_EXIST");
	ASSERT_EQ(f.good(), false);
	ASSERT_EQ(f.is_open(), false);
}

TEST_F(hxFileTest, Operators) {
	hxFile f((hxFile::out | hxFile::fallible), "hx_filetest_ops.bin");
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

	f.open((hxFile::in | hxFile::fallible), "hx_filetest_ops.bin");
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
