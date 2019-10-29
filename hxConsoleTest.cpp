// Copyright 2017 Adrian Johnston

#include "hxConsole.h"
#include "hxTest.h"
#include "hxFile.h"

HX_REGISTER_FILENAME_HASH;

// ----------------------------------------------------------------------------------

class hxConsoleTest :
	public testing::test
{
public:
};

// ----------------------------------------------------------------------------------
// hxConsoleTest::CommandFactory

namespace {
	enum hxConsoleTestTypeID {
		hxConsoleTestTypeID_Void,
		hxConsoleTestTypeID_Char,
		hxConsoleTestTypeID_Short,
		hxConsoleTestTypeID_Int,
		hxConsoleTestTypeID_Long,
		hxConsoleTestTypeID_LongLong,
		hxConsoleTestTypeID_UChar,
		hxConsoleTestTypeID_UShort,
		hxConsoleTestTypeID_UInt,
		hxConsoleTestTypeID_ULong,
		hxConsoleTestTypeID_ULongLong,
		hxConsoleTestTypeID_Float,
		hxConsoleTestTypeID_Double,
		hxConsoleTestTypeID_MAX
	};

	int c_hxConsoleTestCallFlags = 0;

	const char c_hxConsoleTestExpectedChar = 123;
	const short c_hxConsoleTestExpectedShort = -234;
	const int c_hxConsoleTestExpectedInt = -345;
	const long c_hxConsoleTestExpectedLong = 456l;
	const long long c_hxConsoleTestExpectedLongLong = 567ll;
	const unsigned char c_hxConsoleTestExpectedUChar = 12;
	const unsigned short c_hxConsoleTestExpectedUShort = 2345;
	const unsigned int c_hxConsoleTestExpectedUInt = 3456;
	const unsigned long c_hxConsoleTestExpectedULong = 4567ul;
	const unsigned long long c_hxConsoleTestExpectedULongLong = 5678ull;
	const float c_hxConsoleTestExpectedFloat = 6.78f;
	const double c_hxConsoleTestExpectedDouble = 7.89;

	template<typename T>
	void hxConsoleTestTypeCheckT(T t, hxConsoleTestTypeID id, T expected) {
		c_hxConsoleTestCallFlags |= 1 << (int)id;
		ASSERT_EQ(t, expected);
	}

#define hxConsoleTestTypeCheck(T, t) hxConsoleTestTypeCheckT(t, HX_CONCATENATE(hxConsoleTestTypeID_, T), HX_CONCATENATE(c_hxConsoleTestExpected, T))

	void* hxConsoleTestFn0() {
		c_hxConsoleTestCallFlags |= 1 << (int)hxConsoleTestTypeID_Void;
		return 0;
	}

	char hxConsoleTestFn1(char a0) {
		hxConsoleTestTypeCheck(Char, a0);
		return '1';
	}

	short hxConsoleTestFn2(const short a0, int a1) {
		hxConsoleTestTypeCheck(Short, a0);
		hxConsoleTestTypeCheck(Int, a1);
		return 2;
	}

	int hxConsoleTestFn3(const long a0, long long a1, const unsigned char a2) {
		hxConsoleTestTypeCheck(Long, a0);
		hxConsoleTestTypeCheck(LongLong, a1);
		hxConsoleTestTypeCheck(UChar, a2);
		return 3;
	}

	long hxConsoleTestFn4(unsigned short a0, const unsigned int a1, unsigned long a2, unsigned long long a3) {
		hxConsoleTestTypeCheck(UShort, a0);
		hxConsoleTestTypeCheck(UInt, a1);
		hxConsoleTestTypeCheck(ULong, a2);
		hxConsoleTestTypeCheck(ULongLong, a3);
		return 4;
	}

	void hxConsoleTestFnF(float a0, double a1) {
		hxConsoleTestTypeCheck(Float, a0);
		hxConsoleTestTypeCheck(Double, a1);
	}

#undef hxConsoleTestTypeCheck
} // namespace {

TEST_F(hxConsoleTest, CommandFactory) {
	c_hxConsoleTestCallFlags = 0;

	hxCommand* f0 = hxCommandFactory(hxConsoleTestFn0);
	f0->execute("");

	hxCommand* f1 = hxCommandFactory(hxConsoleTestFn1);
	f1->execute("123");

	hxCommand* f2 = hxCommandFactory(hxConsoleTestFn2);
	f2->execute("-234 -345");

	hxCommand* f3 = hxCommandFactory(hxConsoleTestFn3);
	f3->execute("456 567 12");

	hxCommand* f4 = hxCommandFactory(hxConsoleTestFn4);
	f4->execute("2345 3456 4567 5678");

	hxCommand* ff = hxCommandFactory(hxConsoleTestFnF);
	ff->execute("6.78 7.89");

	// Check that all flags have been set.
	ASSERT_EQ(c_hxConsoleTestCallFlags, (1<<hxConsoleTestTypeID_MAX)-1);

	// The hxCommands are being routed to the permenant heap.  While that
	// allocator is unable to reuse space, free them for correctness.
	g_hxSettings.isShuttingDown = true;
	hxFree(f0);
	hxFree(f1);
	hxFree(f2);
	hxFree(f3);
	hxFree(f4);
	hxFree(ff);
	g_hxSettings.isShuttingDown = false;
}

// ----------------------------------------------------------------------------------
// hxConsoleTest::RegisterCommand

namespace {
	float s_hxConsoleTestResultHook = 0.0f;

	void hxConsoleTestRegister0(int a0, const char* a1) {
		s_hxConsoleTestResultHook = (float)a0 + (float)::strlen(a1);
	}

	void hxConsoleTestRegister1(float a0) {
		s_hxConsoleTestResultHook = a0;
	}

	void hxConsoleTestRegister2(float a0) {
		s_hxConsoleTestResultHook = a0;
	}

	void hxConsoleTestRegister3(unsigned int, float a1) {
		s_hxConsoleTestResultHook = a1;
	}
} // namespace {

hxConsoleCommand(hxConsoleTestRegister0);
hxConsoleCommand(hxConsoleTestRegister1);
hxConsoleCommand(hxConsoleTestRegister2);
hxConsoleCommand(hxConsoleTestRegister3);

TEST_F(hxConsoleTest, RegisterCommand) {

	hxLogRelease("TEST_EXPECTING_WARNINGS:\n");

	s_hxConsoleTestResultHook = 0.0f;
	bool b0 = hxConsoleExecLine("hxConsoleTestRegister0 77 ..."); // 77 + 3 char string 
	ASSERT_TRUE(b0);
	ASSERT_EQ(80.0f, s_hxConsoleTestResultHook);

	s_hxConsoleTestResultHook = 0.0f;
	bool b1 = hxConsoleExecLine("hxConsoleTestRegister1 12.5");
	ASSERT_TRUE(b1);
	ASSERT_EQ(12.5f, s_hxConsoleTestResultHook);

	// *Missing arg*
	s_hxConsoleTestResultHook = -1.0f;
	bool b2 = hxConsoleExecLine("hxConsoleTestRegister2 ");
	ASSERT_FALSE(b2);
	ASSERT_EQ(-1.0f, s_hxConsoleTestResultHook);

	// *Missing second arg*
	s_hxConsoleTestResultHook = -2.0f;
	bool b3 = hxConsoleExecLine("hxConsoleTestRegister3 7 ");
	ASSERT_FALSE(b3);
	ASSERT_EQ(-2.0f, s_hxConsoleTestResultHook);

	// *Extra third arg*
	s_hxConsoleTestResultHook = -2.0f;
	bool b4 = hxConsoleExecLine("hxConsoleTestRegister3 7 8 9 ");
	ASSERT_FALSE(b4);
	ASSERT_EQ(-2.0f, s_hxConsoleTestResultHook);

	// Missing function
	bool b5 = hxConsoleExecLine("NotExist");
	ASSERT_FALSE(b5);
}

// ----------------------------------------------------------------------------------
// hxConsoleTest::RegisterVariable

namespace {
	char s_hxConsoleTestChar = 0;
	short s_hxConsoleTestShort = 0;
	int s_hxConsoleTestInt = 0;
	long s_hxConsoleTestLong = 0;
	long long s_hxConsoleTestLongLong = 0;
	unsigned char s_hxConsoleTestUChar = 0;
	unsigned short s_hxConsoleTestUShort = 0;
	unsigned int s_hxConsoleTestUInt = 0;
	unsigned long s_hxConsoleTestULong = 0;
	unsigned long long s_hxConsoleTestULongLong = 0;
	float s_hxConsoleTestFloat = 0;
	double s_hxConsoleTestDouble = 0;
	bool s_hxConsoleTestBool0 = true;
	bool s_hxConsoleTestBool1 = false;
} // namespace {

hxConsoleVariable(s_hxConsoleTestChar);
hxConsoleVariable(s_hxConsoleTestShort);
hxConsoleVariable(s_hxConsoleTestInt);
hxConsoleVariable(s_hxConsoleTestLong);
hxConsoleVariable(s_hxConsoleTestLongLong);
hxConsoleVariable(s_hxConsoleTestUChar);
hxConsoleVariable(s_hxConsoleTestUShort);
hxConsoleVariable(s_hxConsoleTestUInt);
hxConsoleVariable(s_hxConsoleTestULong);
hxConsoleVariable(s_hxConsoleTestULongLong);
hxConsoleVariable(s_hxConsoleTestFloat);
hxConsoleVariable(s_hxConsoleTestDouble);
hxConsoleVariable(s_hxConsoleTestBool0);
hxConsoleVariable(s_hxConsoleTestBool1);

TEST_F(hxConsoleTest, RegisterVariable) {
	hxConsoleExecLine("s_hxConsoleTestChar 123");
	hxConsoleExecLine("s_hxConsoleTestShort 234");
	hxConsoleExecLine("s_hxConsoleTestInt 345");
	hxConsoleExecLine("s_hxConsoleTestLong 456");
	hxConsoleExecLine("s_hxConsoleTestLongLong 567");
	hxConsoleExecLine("s_hxConsoleTestUChar 12");
	hxConsoleExecLine("s_hxConsoleTestUShort 2345");
	hxConsoleExecLine("s_hxConsoleTestUInt 3456");
	hxConsoleExecLine("s_hxConsoleTestULong 4567");
	hxConsoleExecLine("s_hxConsoleTestULongLong 5678");
	hxConsoleExecLine("s_hxConsoleTestFloat 6.78");
	hxConsoleExecLine("s_hxConsoleTestDouble 7.89");
	hxConsoleExecLine("s_hxConsoleTestBool0 0");
	hxConsoleExecLine("s_hxConsoleTestBool1 1");

	ASSERT_EQ(s_hxConsoleTestChar, 123);
	ASSERT_EQ(s_hxConsoleTestShort, 234);
	ASSERT_EQ(s_hxConsoleTestInt, 345);
	ASSERT_EQ(s_hxConsoleTestLong, 456l);
	ASSERT_EQ(s_hxConsoleTestLongLong, 567ll);
	ASSERT_EQ(s_hxConsoleTestUChar, 12);
	ASSERT_EQ(s_hxConsoleTestUShort, 2345);
	ASSERT_EQ(s_hxConsoleTestUInt, 3456);
	ASSERT_EQ(s_hxConsoleTestULong, 4567ul);
	ASSERT_EQ(s_hxConsoleTestULongLong, 5678ull);
	ASSERT_EQ(s_hxConsoleTestFloat, 6.78f);
	ASSERT_EQ(s_hxConsoleTestDouble, 7.89);
	ASSERT_EQ(s_hxConsoleTestBool0, false);
	ASSERT_EQ(s_hxConsoleTestBool1, true);
}

// ----------------------------------------------------------------------------------
// hxConsoleTest::hxConsoleTestFileTest

namespace {
	volatile float s_hxConsoleTestFileVar1 = 0.0f;
	volatile float s_hxConsoleTestFileVar2 = 0.0f;

	void hxConsoleTestFileFn(float f) {
		s_hxConsoleTestFileVar2 = f;
	}
} // namespace {

hxConsoleVariableNamed(s_hxConsoleTestFileVar1, hxConsoleTestFileVar);
hxConsoleCommandNamed(hxConsoleTestFileFn, hxConsoleTestFileFnName);

TEST_F(hxConsoleTest, FileTest) {
	{
		hxFile f(hxFile::out, "hxConsoleTest_FileTest.txt");
		f.print("hxConsoleTestFileVar 3\n");
		f.print("  # comment!\n");
		f.print("\n");
		f.print("hxConsoleTestFileVar 78\n");
		f.print("hxConsoleTestFileFnName 89\n");
	}
	hxConsoleExecLine("exec hxConsoleTest_FileTest.txt");

	ASSERT_EQ(s_hxConsoleTestFileVar1, 78.0f);
	ASSERT_EQ(s_hxConsoleTestFileVar2, 89.0f);
}
