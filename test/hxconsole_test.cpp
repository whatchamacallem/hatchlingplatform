// Copyright 2017-2025 Adrian Johnston

#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxconsole_test::Command_factory

namespace {
	enum hxconsole_test_type_id {
		hxconsole_test_type_id_Void,
		hxconsole_test_type_id_Char,
		hxconsole_test_type_id_Short,
		hxconsole_test_type_id_Int,
		hxconsole_test_type_id_Bool,
		hxconsole_test_type_id_UChar,
		hxconsole_test_type_id_UShort,
		hxconsole_test_type_id_UInt,
		hxconsole_test_type_id_Float,
		hxconsole_test_type_id_LongLong,
		hxconsole_test_type_id_ULongLong,
		hxconsole_test_type_id_Double,
		hxconsole_test_type_id_MAX
	};

	int32_t c_hxconsole_test_call_flags = 0;

	const int8_t c_hxconsole_test_expectedChar = 123;
	const int16_t c_hxconsole_test_expectedShort = -234;
	const int32_t c_hxconsole_test_expectedInt = -345;
	const bool c_hxconsole_test_expectedBool = true;
	const uint8_t c_hxconsole_test_expectedUChar = 12;
	const uint16_t c_hxconsole_test_expectedUShort = 2345;
	const uint32_t c_hxconsole_test_expectedUInt = 3456;
	const float c_hxconsole_test_expectedFloat = 6.78f;

	const int64_t c_hxconsole_test_expectedLongLong = 56789ll;
	const uint64_t c_hxconsole_test_expectedULongLong = 67890ull;
	const double c_hxconsole_test_expectedDouble = 7.89;

	template<typename T>
	void hxconsole_test_type_check_t(T t, hxconsole_test_type_id id, T expected) {
		c_hxconsole_test_call_flags |= 1 << (int32_t)id;
		ASSERT_EQ(t, expected);
	}
	void hxconsole_test_type_check_t(float t, hxconsole_test_type_id id, float expected) {
		c_hxconsole_test_call_flags |= 1 << (int32_t)id;
		ASSERT_NEAR(expected, t, 0.00001f); // This loses precision with -ffast-math.
	}

#define hxconsole_test_type_check(T, t) \
	hxconsole_test_type_check_t(t, hxconsole_test_type_id_##T, c_hxconsole_test_expected##T)

	bool hxconsole_test_fn0() {
		c_hxconsole_test_call_flags |= 1 << (int32_t)hxconsole_test_type_id_Void;
		return true;
	}

	bool hxconsole_test_fn1(hxconsolenumber_t a0) {
		hxconsole_test_type_check(Char, (int8_t)a0);
		return true;
	}

	bool hxconsole_test_fn2(hxconsolenumber_t a0, hxconsolenumber_t a1) {
		hxconsole_test_type_check(Short, (int16_t)a0);
		hxconsole_test_type_check(Int, (int32_t)a1);
		return true;
	}

	bool hxconsole_test_fn3(hxconsolenumber_t a0, hxconsolenumber_t a1) {
		hxconsole_test_type_check(Bool, (bool)a0);
		hxconsole_test_type_check(UChar, (uint8_t)a1);
		return true;
	}

	bool hxconsole_test_fn4(hxconsolenumber_t a0, hxconsolenumber_t a1, hxconsolenumber_t a2, hxconsolenumber_t a3) {
		hxconsole_test_type_check(UShort, (uint16_t)a0);
		hxconsole_test_type_check(UInt, (uint32_t)a1);
		hxconsole_test_type_check(UInt, (uint32_t)a2);
		hxconsole_test_type_check(Float, (float)a3);
		return 4;
	}
	bool hxconsole_test_fn8(hxconsolenumber_t a0, hxconsolenumber_t a1, hxconsolenumber_t a2) {
		hxconsole_test_type_check(LongLong, (int64_t)a0);
		hxconsole_test_type_check(ULongLong, (uint64_t)a1);
		hxconsole_test_type_check(Double, (double)a2);
		return true;
	}

#undef hxconsole_test_type_check
} // namespace

TEST(hxconsole_test, Command_factory) {
	c_hxconsole_test_call_flags = 0;

	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn0).execute_(""));
	ASSERT_FALSE(hxconsole_command_factory_(hxconsole_test_fn0).execute_("unexpected text"));

	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn1).execute_("123"));
	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn2).execute_("-234 -345"));

	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn3).execute_("1 12"));

	// This will pass because 2 is a valid bool.
	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn3).execute_("2 12"));

	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn4).execute_("2345 3456 3456 6.78"));
	ASSERT_FALSE(hxconsole_command_factory_(hxconsole_test_fn4).execute_("$*"));

	ASSERT_TRUE(hxconsole_command_factory_(hxconsole_test_fn8).execute_("56789 67890 7.89"));
	ASSERT_FALSE(hxconsole_command_factory_(hxconsole_test_fn8).execute_("56d789 67890 7.89"));

	// Check that all flags have been set.
	ASSERT_EQ(c_hxconsole_test_call_flags, (1<<hxconsole_test_type_id_MAX)-1);
}

// Trigger some asserts and then call ASSERT_FALSE a few times. Show that asserts
// are hit and can be skipped. And then show that the above test would fail if
// bad commands were submitted.
#if HX_TEST_ERROR_HANDLING
TEST(hxconsole_test, Overflow) {
#if (HX_RELEASE) < 1
	// Test that asserts are triggered by overflow.
	hxconsole_exec_line("skipasserts 2");
#endif

	// These will all ASSERT_FALSE due to parameter mismatch.
	hxconsole_command_factory_(hxconsole_test_fn1).execute_("256");
	hxconsole_command_factory_(hxconsole_test_fn2).execute_("32768 -345");

#if (HX_RELEASE) < 1
	// Test that asserts are triggered by overflow.
	hxconsole_exec_line("checkasserts");
#endif
}
#endif // HX_TEST_ERROR_HANDLING

// ----------------------------------------------------------------------------
// hxconsole_test::Register_command

namespace {
	float s_hxconsole_test_result_hook = 0.0f;

	bool hxconsole_test_register0(hxconsolenumber_t a0, const char* a1) {
		s_hxconsole_test_result_hook = (float)a0 + (float)::strlen(a1);
		return true;
	}

	bool hxconsole_test_register1(hxconsolenumber_t a0) {
		s_hxconsole_test_result_hook = a0;
		return true;
	}

	bool hxconsole_test_register2(hxconsolenumber_t a0) {
		s_hxconsole_test_result_hook = a0;
		return true;
	}

	bool hxconsole_test_register3(hxconsolenumber_t, hxconsolenumber_t a1) {
		s_hxconsole_test_result_hook = a1;
		return true;
	}
} // namespace

hxconsole_command(hxconsole_test_register0);
hxconsole_command(hxconsole_test_register1);
hxconsole_command(hxconsole_test_register2);
hxconsole_command(hxconsole_test_register3);

TEST(hxconsole_test, Register_command) {
	hxlogconsole("test_expecting_warnings:\n");

	s_hxconsole_test_result_hook = 0.0f;
	bool b0 = hxconsole_exec_line("hxconsole_test_register0 77 ..."); // 77 + 3 int8_t string
	ASSERT_TRUE(b0);
	ASSERT_EQ(80.0f, s_hxconsole_test_result_hook);

	s_hxconsole_test_result_hook = 0.0f;
	bool b1 = hxconsole_exec_line("hxconsole_test_register1 12.5");
	ASSERT_TRUE(b1);
	ASSERT_EQ(12.5f, s_hxconsole_test_result_hook);

	// *Missing arg*
	s_hxconsole_test_result_hook = -1.0f;
	bool b2 = hxconsole_exec_line("hxconsole_test_register2 ");
	ASSERT_FALSE(b2);
	ASSERT_EQ(-1.0f, s_hxconsole_test_result_hook);

	// *Missing second arg*
	s_hxconsole_test_result_hook = -2.0f;
	bool b3 = hxconsole_exec_line("hxconsole_test_register3 7 ");
	ASSERT_FALSE(b3);
	ASSERT_EQ(-2.0f, s_hxconsole_test_result_hook);

	// *Extra third arg*
	s_hxconsole_test_result_hook = -2.0f;
	bool b4 = hxconsole_exec_line("hxconsole_test_register3 7 8 9 ");
	ASSERT_FALSE(b4);
	ASSERT_EQ(-2.0f, s_hxconsole_test_result_hook);

	// Missing function
	bool b5 = hxconsole_exec_line("Not_exist");
	ASSERT_FALSE(b5);

	// add code coverage for unmade calls.
	hxconsole_test_register2(1.0f);
	hxconsole_test_register3(1, 1.0f);

	hxconsole_deregister("hxconsole_test_register0");
	bool b6 = hxconsole_exec_line("hxconsole_test_register0 77 ..."); // same as before
	ASSERT_FALSE(b6);
}

// ----------------------------------------------------------------------------
// hxconsole_test::Register_variable

namespace {
	char s_hxconsole_test_char = 0;
	int16_t s_hxconsole_test_short = 0;
	int32_t s_hxconsole_test_int = 0;
	int32_t s_hxconsole_test_long = 0;
	uint8_t s_hxconsole_test_uChar = 0;
	uint16_t s_hxconsole_test_uShort = 0;
	uint32_t s_hxconsole_test_uInt = 0;
	uint32_t s_hxconsole_test_uLong = 0;
	float s_hxconsole_test_float = 0;
	bool s_hxconsole_test_bool0 = true;
	bool s_hxconsole_test_bool1 = false;
	size_t s_hxconsole_test_size = 0u;
	int64_t s_hxconsole_test_long_long = 0;
	uint64_t s_hxconsole_test_uLong_long = 0;
	double s_hxconsole_test_double = 0;
} // namespace

hxconsole_variable(s_hxconsole_test_char);
hxconsole_variable(s_hxconsole_test_short);
hxconsole_variable(s_hxconsole_test_int);
hxconsole_variable(s_hxconsole_test_long);
hxconsole_variable(s_hxconsole_test_uChar);
hxconsole_variable(s_hxconsole_test_uShort);
hxconsole_variable(s_hxconsole_test_uInt);
hxconsole_variable(s_hxconsole_test_uLong);
hxconsole_variable(s_hxconsole_test_float);
hxconsole_variable(s_hxconsole_test_bool0);
hxconsole_variable(s_hxconsole_test_bool1);
hxconsole_variable(s_hxconsole_test_size);
hxconsole_variable(s_hxconsole_test_long_long);
hxconsole_variable(s_hxconsole_test_uLong_long);
hxconsole_variable(s_hxconsole_test_double);

TEST(hxconsole_test, Register_variable) {
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_char 123"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_short 234"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_int 345"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_long 456"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_uChar 12"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_uShort 2345"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_uInt 3456"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_uLong 4567"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_float 678.0"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_bool0 0"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_bool1 1"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_long_long 567"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_size 1000"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_uLong_long 5678"));
	ASSERT_TRUE(hxconsole_exec_line("s_hxconsole_test_double 789.0"));

	ASSERT_EQ(s_hxconsole_test_char, 123);
	ASSERT_EQ(s_hxconsole_test_short, 234);
	ASSERT_EQ(s_hxconsole_test_int, 345);
	ASSERT_EQ(s_hxconsole_test_long, 456l);
	ASSERT_EQ(s_hxconsole_test_uChar, 12);
	ASSERT_EQ(s_hxconsole_test_uShort, 2345);
	ASSERT_EQ(s_hxconsole_test_uInt, 3456);
	ASSERT_EQ(s_hxconsole_test_uLong, 4567ul);
	ASSERT_EQ(s_hxconsole_test_float, 678.0f);
	ASSERT_EQ(s_hxconsole_test_bool0, false);
	ASSERT_EQ(s_hxconsole_test_bool1, true);
	ASSERT_EQ(s_hxconsole_test_size, 1000l);
	ASSERT_EQ(s_hxconsole_test_long_long, 567ll);
	ASSERT_EQ(s_hxconsole_test_uLong_long, 5678ull);
	ASSERT_EQ(s_hxconsole_test_double, 789.0);
}

// Show rounding errors while setting a variable are ignored. It isn't ideal
// but code bloat is a consideration.
TEST(hxconsole_test, Variable_errors) {
	s_hxconsole_test_int = -1;
	hxconsole_exec_line("s_hxconsole_test_int 3.5");
	ASSERT_EQ(s_hxconsole_test_int, 3);
}

// ----------------------------------------------------------------------------
// hxconsole_test::hxconsole_test_file_test

namespace {
	volatile float s_hxconsole_test_file_var1 = 0.0f;
	volatile float s_hxconsole_test_file_var2 = 0.0f;

	bool hxconsole_test_file_fn(hxconsolenumber_t f) {
		s_hxconsole_test_file_var2 = f;
		return true;
	}
} // namespace

hxconsole_variable_named(s_hxconsole_test_file_var1, hxconsole_test_file_var);
hxconsole_command_named(hxconsole_test_file_fn, hxconsole_test_file_fn_name);

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST(hxconsole_test, Null_test) {
	uint8_t prev = g_hxsettings.log_level;
	g_hxsettings.log_level = hxloglevel_warning;
	hxloghandler(hxloglevel_console, "hidden\n");
	g_hxsettings.log_level = prev;

	hxlog("");
	SUCCEED();
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxconsole_test, File_test) {
	{
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f << "hxconsole_test_file_var 3\n"
			"  # comment!\n"
			"\n"
			"hxconsole_test_file_var 78\n"
			"hxconsole_test_file_fn_name 89\n"
			"\n";
	}
	bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	ASSERT_TRUE(is_ok);

	ASSERT_EQ(s_hxconsole_test_file_var1, 78.0f);
	ASSERT_EQ(s_hxconsole_test_file_var2, 89.0f);
}

bool hxconsole_test_failing_command(void) {
	return false;
}

hxconsole_command_named(hxconsole_test_failing_command, hxconsole_test_failing_command);

TEST(hxconsole_test, File_fail) {
	hxlogconsole("test_expecting_warnings:\n");

	// test garbage in a script
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "<unknown symbols>\n";
	}
	ASSERT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));

	// test a bad function call
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "exec\n";
	}
	ASSERT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));

	// test a failing command
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "hxconsole_test_failing_command\n";
	}
	ASSERT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));
}

#if (HX_RELEASE) < 2 && !defined __EMSCRIPTEN__
TEST(hxconsole_test, File_peek_poke) {
	uint32_t target[] = { 111, 777, 333 };
	{
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f.print("peek %zx 4\n", (size_t)target);
		f.print("poke %zx 4 de\n", (size_t)(target + 1));
		f.print("hexdump %zx 12\n", (size_t)target);
	}
	bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	ASSERT_TRUE(is_ok);

	ASSERT_EQ(target[0], 111);
	ASSERT_EQ(target[1], 222);
	ASSERT_EQ(target[2], 333);
}
TEST(hxconsole_test, File_peek_poke_floats) {
	float target[] = { 111.0f, 777.0f, 333.0f };
	{
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f.print("poke %zx 4 435E0000\n", (size_t)(target + 1));
		f.print("floatdump %zx 3\n", (size_t)target);
	}
	bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	ASSERT_TRUE(is_ok);

	ASSERT_EQ(target[0], 111.0f);
	ASSERT_EQ(target[1], 222.0f);
	ASSERT_EQ(target[2], 333.0f);
}
#endif
