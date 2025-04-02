// Copyright 2017-2019 Adrian Johnston

#include <hx/hxConsole.h>
#include <hx/hxFile.h>
#include <hx/hxTest.h>

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
#if HX_USE_64_BIT_TYPES
		hxConsoleTestTypeID_LongLong,
		hxConsoleTestTypeID_ULongLong,
		hxConsoleTestTypeID_Double,
#endif
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

#if HX_USE_64_BIT_TYPES
	const int64_t c_hxConsoleTestExpectedLongLong = 56789ll;
	const uint64_t c_hxConsoleTestExpectedULongLong = 67890ull;
	const double c_hxConsoleTestExpectedDouble = 7.89;
#endif // HX_USE_64_BIT_TYPES

	template<typename T>
	void hxConsoleTestTypeCheckT(T t, hxConsoleTestTypeID id, T expected) {
		c_hxConsoleTestCallFlags |= 1 << (int32_t)id;
		ASSERT_EQ(t, expected);
	}

#define hxConsoleTestTypeCheck(T, t) hxConsoleTestTypeCheckT(t, HX_CONCATENATE(hxConsoleTestTypeID_, T), \
	HX_CONCATENATE(c_hxConsoleTestExpected, T))

	void* hxConsoleTestFn0() {
		c_hxConsoleTestCallFlags |= 1 << (int32_t)hxConsoleTestTypeID_Void;
		return 0;
	}

	int8_t hxConsoleTestFn1(int8_t a0) {
		hxConsoleTestTypeCheck(Char, a0);
		return '1';
	}

	int16_t hxConsoleTestFn2(const int16_t a0, int32_t a1) {
		hxConsoleTestTypeCheck(Short, a0);
		hxConsoleTestTypeCheck(Int, a1);
		return 2;
	}

	int32_t hxConsoleTestFn3(const bool a0, const uint8_t a1) {
		hxConsoleTestTypeCheck(Bool, a0);
		hxConsoleTestTypeCheck(UChar, a1);
		return 3;
	}

	int32_t hxConsoleTestFn4(uint16_t a0, const uint32_t a1, uint32_t a2, float a3) {
		hxConsoleTestTypeCheck(UShort, a0);
		hxConsoleTestTypeCheck(UInt, a1);
		hxConsoleTestTypeCheck(UInt, a2);
		hxConsoleTestTypeCheck(Float, a3);
		return 4;
	}
#if HX_USE_64_BIT_TYPES
	void hxConsoleTestFn8(int64_t a0, uint64_t a1, double a2) {
		hxConsoleTestTypeCheck(LongLong, a0);
		hxConsoleTestTypeCheck(ULongLong, a1);
		hxConsoleTestTypeCheck(Double, a2);
	}
#endif

#undef hxConsoleTestTypeCheck
} // namespace {

TEST(hxConsoleTest, CommandFactory) {
	hxLogConsole("TEST_EXPECTING_WARNINGS:\n");

	c_hxConsoleTestCallFlags = 0;

	hxCommand* f0 = hxCommandFactory(hxConsoleTestFn0);
	ASSERT_TRUE(f0->execute(""));
	ASSERT_FALSE(f0->execute("unexpected text"));

	hxCommand* f1 = hxCommandFactory(hxConsoleTestFn1);
	ASSERT_TRUE(f1->execute("123"));
	ASSERT_FALSE(f1->execute("256"));

	hxCommand* f2 = hxCommandFactory(hxConsoleTestFn2);
	ASSERT_TRUE(f2->execute("-234 -345"));
	ASSERT_FALSE(f2->execute("32768 -345"));

	hxCommand* f3 = hxCommandFactory(hxConsoleTestFn3);
	ASSERT_TRUE(f3->execute("1 12"));
	ASSERT_FALSE(f3->execute("2 12"));

	hxCommand* f4 = hxCommandFactory(hxConsoleTestFn4);
	ASSERT_TRUE(f4->execute("2345 3456 3456 6.78"));
	ASSERT_FALSE(f4->execute("$*"));

#if HX_USE_64_BIT_TYPES
	hxCommand* ff = hxCommandFactory(hxConsoleTestFn8);
	ASSERT_TRUE(ff->execute("56789 67890 7.89"));
	ASSERT_FALSE(ff->execute("56d789 67890 7.89"));
	hxFree(ff);
#endif

	// Check that all flags have been set.
	ASSERT_EQ(c_hxConsoleTestCallFlags, (1<<hxConsoleTestTypeID_MAX)-1);

	// The hxCommands are being routed to the permanent heap.  While that
	// allocator is unable to reuse space, free them for correctness.
	g_hxSettings.isShuttingDown = true;
	hxFree(f0);
	hxFree(f1);
	hxFree(f2);
	hxFree(f3);
	hxFree(f4);
	g_hxSettings.isShuttingDown = false;
}

// ----------------------------------------------------------------------------
// hxConsoleTest::RegisterCommand

namespace {
	float s_hxConsoleTestResultHook = 0.0f;

	void hxConsoleTestRegister0(int32_t a0, const char* a1) {
		s_hxConsoleTestResultHook = (float)a0 + (float)::strlen(a1);
	}

	void hxConsoleTestRegister1(float a0) {
		s_hxConsoleTestResultHook = a0;
	}

	void hxConsoleTestRegister2(float a0) {
		s_hxConsoleTestResultHook = a0;
	}

	void hxConsoleTestRegister3(uint32_t, float a1) {
		s_hxConsoleTestResultHook = a1;
	}
} // namespace {

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
	int8_t s_hxConsoleTestChar = 0;
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

#if HX_USE_64_BIT_TYPES
	int64_t s_hxConsoleTestLongLong = 0;
	uint64_t s_hxConsoleTestULongLong = 0;
	double s_hxConsoleTestDouble = 0;
	double s_hxConsoleTestDoubleLarge = (double)(1ull << 63) * (double)(1ull << 63);
#endif
} // namespace {

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

#if HX_USE_64_BIT_TYPES
hxConsoleVariable(s_hxConsoleTestLongLong);
hxConsoleVariable(s_hxConsoleTestULongLong);
hxConsoleVariable(s_hxConsoleTestDouble);
hxConsoleVariable(s_hxConsoleTestDoubleLarge);
#endif

TEST(hxConsoleTest, RegisterVariable) {
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestChar 123"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestShort 234"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestInt 345"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestLong 456"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUChar 12"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUShort 2345"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestUInt 3456"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestULong 4567"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestFloat 6.78"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestBool0 0"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestBool1 1"));

#if HX_USE_64_BIT_TYPES
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestLongLong 567"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestULongLong 5678"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestDouble 7.89"));
#endif

	hxLogConsole("TEST_EXPECTING_WARNINGS:\n");
	ASSERT_FALSE(hxConsoleExecLine("s_hxConsoleTestInt 3.5"));
	ASSERT_TRUE(hxConsoleExecLine("s_hxConsoleTestInt"));

	ASSERT_EQ(s_hxConsoleTestChar, 123);
	ASSERT_EQ(s_hxConsoleTestShort, 234);
	ASSERT_EQ(s_hxConsoleTestInt, 345);
	ASSERT_EQ(s_hxConsoleTestLong, 456l);
	ASSERT_EQ(s_hxConsoleTestUChar, 12);
	ASSERT_EQ(s_hxConsoleTestUShort, 2345);
	ASSERT_EQ(s_hxConsoleTestUInt, 3456);
	ASSERT_EQ(s_hxConsoleTestULong, 4567ul);
	ASSERT_EQ(s_hxConsoleTestFloat, 6.78f);
	ASSERT_EQ(s_hxConsoleTestBool0, false);
	ASSERT_EQ(s_hxConsoleTestBool1, true);

#if HX_USE_64_BIT_TYPES
	ASSERT_EQ(s_hxConsoleTestLongLong, 567ll);
	ASSERT_EQ(s_hxConsoleTestULongLong, 5678ull);
	ASSERT_EQ(s_hxConsoleTestDouble, 7.89);
#endif
}

// ----------------------------------------------------------------------------
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

#if defined(__GNUC__)
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

#if defined(__GNUC__)
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

#if (HX_RELEASE) < 2 && !HX_USE_WASM
TEST(hxConsoleTest, FilePeekPoke) {
	int target[16] = { 137, 396 };
	{
		hxFile f(hxFile::out, "hxConsoleTest_FileTest.txt");
#if HX_USE_64_BIT_TYPES
		f.print("peek %lld 4\n", (unsigned long long int)(target + 0));
		f.print("hex %lld 4\n", (unsigned long long int)(target + 0));
		f.print("poke %lld 4 175\n", (unsigned long long int)(target + 0));
#else
		f.print("peek %ld 4\n", (unsigned long int)(target + 0));
		f.print("hex %ld 4\n", (unsigned long int)(target + 0));
		f.print("poke %ld 4 175\n", (unsigned long int)(target + 0));
#endif
	}
	bool isOk = hxConsoleExecLine("exec hxConsoleTest_FileTest.txt");
	ASSERT_TRUE(isOk);

	ASSERT_EQ(target[0], 175);
	ASSERT_EQ(target[1], 396);
}
#endif
