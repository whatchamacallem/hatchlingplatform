// Copyright 2017-2025 Adrian Johnston

#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxTest.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxConsoleTest::CommandFactory

namespace {
	enum hxConsoleTestTypeID {
		hxConsoleTestTypeID_Void,
		hxConsoleTestTypeID_Char,
		hxConsoleTestTypeID_Short,
		hxConsoleTestTypeID_Int,
		hxConsoleTestTypeID_Bool,
		hxConsoleTestTypeID_UChar,
		hxConsoleTestTypeID_UShort,
		hxConsoleTestTypeID_UInt,
		hxConsoleTestTypeID_Float,
		hxConsoleTestTypeID_LongLong,
		hxConsoleTestTypeID_ULongLong,
		hxConsoleTestTypeID_Double,
		hxConsoleTestTypeID_MAX
	};

	int32_t c_hxConsoleTestCallFlags = 0;

	const int8_t c_hxConsoleTestExpectedChar = 123;
	const int16_t c_hxConsoleTestExpectedShort = -234;
	const int32_t c_hxConsoleTestExpectedInt = -345;
	const bool c_hxConsoleTestExpectedBool = true;
	const uint8_t c_hxConsoleTestExpectedUChar = 12;
	const uint16_t c_hxConsoleTestExpectedUShort = 2345;
	const uint32_t c_hxConsoleTestExpectedUInt = 3456;
	const float c_hxConsoleTestExpectedFloat = 6.78f;

	const int64_t c_hxConsoleTestExpectedLongLong = 56789ll;
	const uint64_t c_hxConsoleTestExpectedULongLong = 67890ull;
	const double c_hxConsoleTestExpectedDouble = 7.89;

	template<typename T>
	void hxConsoleTestTypeCheckT(T t, hxConsoleTestTypeID id, T expected) {
		c_hxConsoleTestCallFlags |= 1 << (int32_t)id;
		ASSERT_EQ(t, expected);
	}

#define hxConsoleTestTypeCheck(T, t) hxConsoleTestTypeCheckT(t, HX_CONCATENATE(hxConsoleTestTypeID_, T), \
	HX_CONCATENATE(c_hxConsoleTestExpected, T))

	bool hxConsoleTestFn0() {
		c_hxConsoleTestCallFlags |= 1 << (int32_t)hxConsoleTestTypeID_Void;
		return true;
	}

	bool hxConsoleTestFn1(hxconsolenumber_t a0) {
		hxConsoleTestTypeCheck(Char, (int8_t)a0);
		return true;
	}

	bool hxConsoleTestFn2(hxconsolenumber_t a0, hxconsolenumber_t a1) {
		hxConsoleTestTypeCheck(Short, (int16_t)a0);
		hxConsoleTestTypeCheck(Int, (int32_t)a1);
		return true;
	}

	bool hxConsoleTestFn3(hxconsolenumber_t a0, hxconsolenumber_t a1) {
		hxConsoleTestTypeCheck(Bool, (bool)a0);
		hxConsoleTestTypeCheck(UChar, (uint8_t)a1);
		return true;
	}

	bool hxConsoleTestFn4(hxconsolenumber_t a0, hxconsolenumber_t a1, hxconsolenumber_t a2, hxconsolenumber_t a3) {
		hxConsoleTestTypeCheck(UShort, (uint16_t)a0);
		hxConsoleTestTypeCheck(UInt, (uint32_t)a1);
		hxConsoleTestTypeCheck(UInt, (uint32_t)a2);
		hxConsoleTestTypeCheck(Float, (float)a3);
		return 4;
	}
	bool hxConsoleTestFn8(hxconsolenumber_t a0, hxconsolenumber_t a1, hxconsolenumber_t a2) {
		hxConsoleTestTypeCheck(LongLong, (int64_t)a0);
		hxConsoleTestTypeCheck(ULongLong, (uint64_t)a1);
		hxConsoleTestTypeCheck(Double, (double)a2);
		return true;
	}

#undef hxConsoleTestTypeCheck
} // namespace

TEST(hxConsoleTest, CommandFactory) {

	c_hxConsoleTestCallFlags = 0;

	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn0).execute_(""));
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn0).execute_("unexpected text"));

	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn1).execute_("123"));
	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn2).execute_("-234 -345"));

	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn3).execute_("1 12"));

	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn4).execute_("2345 3456 3456 6.78"));
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn4).execute_("$*"));

	ASSERT_TRUE(hxConsoleCommandFactory_(hxConsoleTestFn8).execute_("56789 67890 7.89"));
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn8).execute_("56d789 67890 7.89"));

#if HX_TEST_ERROR_HANDLING
	// These all fail due to precision.
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn1).execute_("256"));
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn2).execute_("32768 -345"));
	ASSERT_FALSE(hxConsoleCommandFactory_(hxConsoleTestFn3).execute_("2 12"));
#endif

	// Check that all flags have been set.
	ASSERT_EQ(c_hxConsoleTestCallFlags, (1<<hxConsoleTestTypeID_MAX)-1);
}

// ----------------------------------------------------------------------------
// hxConsoleTest::RegisterCommand

namespace {
	float s_hxConsoleTestResultHook = 0.0f;

	void hxConsoleTestRegister0(hxconsolenumber_t a0, const char* a1) {
		s_hxConsoleTestResultHook = (float)a0 + (float)::strlen(a1);
	}

	bool hxConsoleTestRegister1(hxconsolenumber_t a0) {
		s_hxConsoleTestResultHook = a0;
		return true;
	}

	int hxConsoleTestRegister2(hxconsolenumber_t a0) {
		s_hxConsoleTestResultHook = a0;
		return 2;
	}

	float hxConsoleTestRegister3(hxconsolenumber_t, hxconsolenumber_t a1) {
		s_hxConsoleTestResultHook = a1;
		return 0.1f;
	}
} // namespace

hxConsoleCommand(hxConsoleTestRegister0);
hxConsoleCommand(hxConsoleTestRegister1);
hxConsoleCommand(hxConsoleTestRegister2);
hxConsoleCommand(hxConsoleTestRegister3);

TEST(hxConsoleTest, RegisterCommand) {
	hxLogConsole("TEST_EXPECTING_WARNINGS:\n");

	s_hxConsoleTestResultHook = 0.0f;
	bool b0 = hxConsoleExecLine("hxConsoleTestRegister0 77 ..."); // 77 + 3 int8_t string
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

	// add code coverage for unmade calls.
	hxConsoleTestRegister2(1.0f);
	hxConsoleTestRegister3(1, 1.0f);

	hxConsoleDeregister("hxConsoleTestRegister0");
	bool b6 = hxConsoleExecLine("hxConsoleTestRegister0 77 ..."); // same as before
	ASSERT_FALSE(b6);
}

// ----------------------------------------------------------------------------
// hxConsoleTest::RegisterVariable

namespace {
	char s_hxConsoleTestChar = 0;
	int16_t s_hxConsoleTestShort = 0;
	int32_t s_hxConsoleTestInt = 0;
	int32_t s_hxConsoleTestLong = 0;
	uint8_t s_hxConsoleTestUChar = 0;
	uint16_t s_hxConsoleTestUShort = 0;
	uint32_t s_hxConsoleTestUInt = 0;
	uint32_t s_hxConsoleTestULong = 0;
	float s_hxConsoleTestFloat = 0;
	bool s_hxConsoleTestBool0 = true;
	bool s_hxConsoleTestBool1 = false;
	size_t s_hxConsoleTestSize = 0u;
	int64_t s_hxConsoleTestLongLong = 0;
	uint64_t s_hxConsoleTestULongLong = 0;
	double s_hxConsoleTestDouble = 0;
} // namespace

hxConsoleVariable(s_hxConsoleTestChar);
hxConsoleVariable(s_hxConsoleTestShort);
hxConsoleVariable(s_hxConsoleTestInt);
hxConsoleVariable(s_hxConsoleTestLong);
hxConsoleVariable(s_hxConsoleTestUChar);
hxConsoleVariable(s_hxConsoleTestUShort);
hxConsoleVariable(s_hxConsoleTestUInt);
hxConsoleVariable(s_hxConsoleTestULong);
hxConsoleVariable(s_hxConsoleTestFloat);
hxConsoleVariable(s_hxConsoleTestBool0);
hxConsoleVariable(s_hxConsoleTestBool1);
hxConsoleVariable(s_hxConsoleTestSize);
hxConsoleVariable(s_hxConsoleTestLongLong);
hxConsoleVariable(s_hxConsoleTestULongLong);
hxConsoleVariable(s_hxConsoleTestDouble);

TEST(hxConsoleTest, RegisterVariable) {
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestChar 123"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestShort 234"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestInt 345"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestLong 456"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUChar 12"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUShort 2345"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUInt 3456"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestULong 4567"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestFloat 678.0"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestBool0 0"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestBool1 1"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestLongLong 567"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestSize 1000"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestULongLong 5678"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestDouble 789.0"));

#if HX_TEST_ERROR_HANDLING
	hxLogConsole("TEST_EXPECTING_WARNINGS:\n");
	ASSERT_FALSE(hxConsoleExecLine("s_hxConsoleTestInt 3.5"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestInt"));
#endif

	ASSERT_EQ(s_hxConsoleTestChar, 123);
	ASSERT_EQ(s_hxConsoleTestShort, 234);
	ASSERT_EQ(s_hxConsoleTestInt, 345);
	ASSERT_EQ(s_hxConsoleTestLong, 456l);
	ASSERT_EQ(s_hxConsoleTestUChar, 12);
	ASSERT_EQ(s_hxConsoleTestUShort, 2345);
	ASSERT_EQ(s_hxConsoleTestUInt, 3456);
	ASSERT_EQ(s_hxConsoleTestULong, 4567ul);
	ASSERT_EQ(s_hxConsoleTestFloat, 678.0f);
	ASSERT_EQ(s_hxConsoleTestBool0, false);
	ASSERT_EQ(s_hxConsoleTestBool1, true);
	ASSERT_EQ(s_hxConsoleTestSize, 1000l);
	ASSERT_EQ(s_hxConsoleTestLongLong, 567ll);
	ASSERT_EQ(s_hxConsoleTestULongLong, 5678ull);
	ASSERT_EQ(s_hxConsoleTestDouble, 789.0);
}

// ----------------------------------------------------------------------------
// hxConsoleTest::hxConsoleTestFileTest

namespace {
	volatile float s_hxConsoleTestFileVar1 = 0.0f;
	volatile float s_hxConsoleTestFileVar2 = 0.0f;

	void hxConsoleTestFileFn(hxconsolenumber_t f) {
		s_hxConsoleTestFileVar2 = f;
	}
} // namespace

hxConsoleVariableNamed(s_hxConsoleTestFileVar1, hxConsoleTestFileVar);
hxConsoleCommandNamed(hxConsoleTestFileFn, hxConsoleTestFileFnName);

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST(hxConsoleTest, NullTest) {
	uint8_t prev = g_hxSettings.logLevel;
	g_hxSettings.logLevel = hxLogLevel_Warning;
	hxLogHandler(hxLogLevel_Console, "hidden\n");
	g_hxSettings.logLevel = prev;

	hxLog("");
	SUCCEED();
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxConsoleTest, FileTest) {
	{
		hxFile f(hxFile::out, "hxConsoleTest_FileTest.txt");
		f << "hxConsoleTestFileVar 3\n"
			"  # comment!\n"
			"\n"
			"hxConsoleTestFileVar 78\n"
			"hxConsoleTestFileFnName 89\n"
			"\n";
	}
	bool isOk = hxConsoleExecLine("exec hxConsoleTest_FileTest.txt");
	ASSERT_TRUE(isOk);

	ASSERT_EQ(s_hxConsoleTestFileVar1, 78.0f);
	ASSERT_EQ(s_hxConsoleTestFileVar2, 89.0f);
}

bool hxConsoleTestFailingCommand(void) {
	return false;
}

hxConsoleCommandNamed(hxConsoleTestFailingCommand, hxConsoleTestFailingCommand);

TEST(hxConsoleTest, FileFail) {
	hxLogConsole("TEST_EXPECTING_WARNINGS:\n");

	// test garbage in a script
	{
		hxFile(hxFile::out, "hxConsoleTest_FileTest.txt") << "<unknown symbols>\n";
	}
	ASSERT_FALSE(hxConsoleExecFilename("hxConsoleTest_FileTest.txt"));

	// test a bad function call
	{
		hxFile(hxFile::out, "hxConsoleTest_FileTest.txt") << "exec\n";
	}
	ASSERT_FALSE(hxConsoleExecFilename("hxConsoleTest_FileTest.txt"));

	// test a failing command
	{
		hxFile(hxFile::out, "hxConsoleTest_FileTest.txt") << "hxConsoleTestFailingCommand\n";
	}
	ASSERT_FALSE(hxConsoleExecFilename("hxConsoleTest_FileTest.txt"));
}

#if (HX_RELEASE) < 2 && !defined __EMSCRIPTEN__
TEST(hxConsoleTest, FilePeekPoke) {
	uint32_t target[] = { 111, 777, 333 };
	{
		hxFile f(hxFile::out, "hxConsoleTest_FileTest.txt");
		f.print("peek %llx 4\n", (unsigned long long int)target);
		f.print("poke %llx 4 de\n", (unsigned long long int)(target + 1));
		f.print("hexdump %llx 12\n", (unsigned long long int)target);
	}
	bool isOk = hxConsoleExecLine("exec hxConsoleTest_FileTest.txt");
	ASSERT_TRUE(isOk);

	ASSERT_EQ(target[0], 111);
	ASSERT_EQ(target[1], 222);
	ASSERT_EQ(target[2], 333);
}
TEST(hxConsoleTest, FilePeekPokeFloats) {
	float target[] = { 111.0f, 777.0f, 333.0f };
	{
		hxFile f(hxFile::out, "hxConsoleTest_FileTest.txt");
		f.print("poke %llx 4 435E0000\n", (unsigned long long int)(target + 1));
		f.print("floatdump %llx 3\n", (unsigned long long int)target);
	}
	bool isOk = hxConsoleExecLine("exec hxConsoleTest_FileTest.txt");
	ASSERT_TRUE(isOk);

	ASSERT_EQ(target[0], 111.0f);
	ASSERT_EQ(target[1], 222.0f);
	ASSERT_EQ(target[2], 333.0f);
}
#endif
