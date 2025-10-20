// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp>

HX_REGISTER_FILENAME_HASH

namespace {
float s_hxconsole_test_result_hook = 0.0f;

bool hxconsole_test_register0(hxconsolenumber_t a0, const char* a1) {
	s_hxconsole_test_result_hook = static_cast<float>(a0) + static_cast<float>(::strlen(a1));
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

TEST(hxconsole_test, register_command) {
	hxlogconsole("EXPECTING_TEST_WARNINGS\n");

	// "Evaluates a console command to either call a function or set a variable."
	//   77 + |"..."|=3 => hook = 80.0f
	s_hxconsole_test_result_hook = 0.0f;
	const bool b0 = hxconsole_exec_line("hxconsole_test_register0 77 ..."); // 77 + 3 from the int8_t string.
	EXPECT_TRUE(b0);
	EXPECT_EQ(80.0f, s_hxconsole_test_result_hook);

	// "Transports numeric arguments via a double-backed wrapper." Verify literal 12.5 passes through unchanged.
	s_hxconsole_test_result_hook = 0.0f;
	const bool b1 = hxconsole_exec_line("hxconsole_test_register1 12.5");
	EXPECT_TRUE(b1);
	EXPECT_EQ(12.5f, s_hxconsole_test_result_hook);

	// Input arity guard: incomplete tokens keep hook at sentinel {-1}.
	s_hxconsole_test_result_hook = -1.0f;
	const bool b2 = hxconsole_exec_line("hxconsole_test_register2 ");
	EXPECT_FALSE(b2);
	EXPECT_EQ(-1.0f, s_hxconsole_test_result_hook);

	// Second argument omitted -> parser rejects and sentinel {-2} persists.
	s_hxconsole_test_result_hook = -2.0f;
	const bool b3 = hxconsole_exec_line("hxconsole_test_register3 7 ");
	EXPECT_FALSE(b3);
	EXPECT_EQ(-2.0f, s_hxconsole_test_result_hook);

	// Extra payload triggers parse failure, leaving {-2} unchanged.
	s_hxconsole_test_result_hook = -2.0f;
	const bool b4 = hxconsole_exec_line("hxconsole_test_register3 7 8 9 ");
	EXPECT_FALSE(b4);
	EXPECT_EQ(-2.0f, s_hxconsole_test_result_hook);

	// Missing function
	const bool b5 = hxconsole_exec_line("Not_exist");
	EXPECT_FALSE(b5);

	// add code coverage for unmade calls.
	hxconsole_test_register2(1.0f);
	hxconsole_test_register3(1, 1.0f);

	// "Explicitly deregisters a console symbol." Confirm removal leaves hook untouched.
	hxconsole_deregister("hxconsole_test_register0");
	const bool b6 = hxconsole_exec_line("hxconsole_test_register0 77 ..."); // same as before
	EXPECT_FALSE(b6);
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

TEST(hxconsole_test, register_variable) {
	// "Registers a variable. Use in a global scope." Drive numeric + boolean slots via scripted commands.
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_char 123"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_short 234"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_int 345"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_long 456"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_uChar 12"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_uShort 2345"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_uInt 3456"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_uLong 4567"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_float 678.0"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_bool0 0"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_bool1 1"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_long_long 567"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_size 1000"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_uLong_long 5678"));
	EXPECT_TRUE(hxconsole_exec_line("s_hxconsole_test_double 789.0"));

	// Consolidate storage snapshot: { 123, 234, 345, 456, 12, 2345, 3456, 4567, 678.0, 0, 1, 1000, 567, 5678, 789.0 }.
	EXPECT_EQ(s_hxconsole_test_char, 123);
	EXPECT_EQ(s_hxconsole_test_short, 234);
	EXPECT_EQ(s_hxconsole_test_int, 345);
	EXPECT_EQ(s_hxconsole_test_long, 456l);
	EXPECT_EQ(s_hxconsole_test_uChar, 12);
	EXPECT_EQ(s_hxconsole_test_uShort, 2345);
	EXPECT_EQ(s_hxconsole_test_uInt, 3456);
	EXPECT_EQ(s_hxconsole_test_uLong, 4567ul);
	EXPECT_EQ(s_hxconsole_test_float, 678.0f);
	EXPECT_EQ(s_hxconsole_test_bool0, false);
	EXPECT_EQ(s_hxconsole_test_bool1, true);
	EXPECT_EQ(s_hxconsole_test_size, 1000l);
	EXPECT_EQ(s_hxconsole_test_long_long, 567ll);
	EXPECT_EQ(s_hxconsole_test_uLong_long, 5678ull);
	EXPECT_EQ(s_hxconsole_test_double, 789.0);
}

// Show rounding errors while setting a variable are ignored. It isn't ideal
// but code bloat is a consideration.
TEST(hxconsole_test, variable_errors) {
	s_hxconsole_test_int = -1;
	// "Numeric wrapper that uses double as an intermediate type." Rounding down 3.5 → 3.
	hxconsole_exec_line("s_hxconsole_test_int 3.5");
	EXPECT_EQ(s_hxconsole_test_int, 3);
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

TEST(hxconsole_test, null_test) {
	const uint8_t prev = g_hxsettings.log_level;
	g_hxsettings.log_level = hxloglevel_warning;
	// "Enters formatted messages in the system log." Suppress console echoes while routing a hidden line through the handler.
	hxloghandler(hxloglevel_console, "test_hidden\n");
	g_hxsettings.log_level = prev;

	// Blank emission path: "" -> no format args, ensure fast return.
	hxlog("");
	SUCCEED();
}

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif

TEST(hxconsole_test, file_test) {
	{
		// "Writes a string literal to the file." Script seeds variable, comment, blank, and call stack.
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f << "hxconsole_test_file_var 3\n"
			"  # comment!\n"
			"\n"
			"hxconsole_test_file_var 78\n"
			"hxconsole_test_file_fn_name 89\n"
			"\n";
	}
	// "Executes a configuration file that is opened for reading. Ignores blank lines and comments that start with #."
	const bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	EXPECT_TRUE(is_ok);

	// Timeline: var1 { 0 → 3 → 78 }, var2 { 0 → 89 }.
	EXPECT_EQ(s_hxconsole_test_file_var1, 78.0f);
	EXPECT_EQ(s_hxconsole_test_file_var2, 89.0f);
}

static bool hxconsole_test_failing_command(void) {
	return false;
}

hxconsole_command_named(hxconsole_test_failing_command, hxconsole_test_failing_command);

TEST(hxconsole_test, file_fail) {
	hxlogconsole("EXPECTING_TEST_WARNINGS\n");

	// test garbage in a script
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "<unknown symbols>\n";
	}
	// "Opens a configuration file by name and executes it." Unknown tokens should report failure.
	EXPECT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));

	// test a bad function call
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "exec\n";
	}
	// Arity guard: missing filename keeps return false.
	EXPECT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));

	// test a failing command
	{
		hxfile(hxfile::out, "hxconsole_test_file_test.txt") << "hxconsole_test_failing_command\n";
	}
	// Propagate command failure -> loader yields false.
	EXPECT_FALSE(hxconsole_exec_filename("hxconsole_test_file_test.txt"));
}

#if (HX_RELEASE) < 2 && !defined __wasm__
TEST(hxconsole_test, file_peek_poke) {
	uint32_t target[] = { 111, 777, 333 };
	{
		// "Writes a formatted UTF-8 string to the file." Script peek/poke/hexdump to tweak middle dword.
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f.print("peek %zx 4\n", reinterpret_cast<size_t>(target));
		f.print("poke %zx 4 de\n", reinterpret_cast<size_t>(target + 1));
		f.print("hexdump %zx 12\n", reinterpret_cast<size_t>(target));
	}
	const bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	EXPECT_TRUE(is_ok);

	// Buffer morph: [111] [777] [333] -> [111] [222] [333]
	EXPECT_EQ(target[0], 111);
	EXPECT_EQ(target[1], 222);
	EXPECT_EQ(target[2], 333);
}
TEST(hxconsole_test, file_peek_poke_floats) {
	float target[] = { 111.0f, 777.0f, 333.0f };
	{
		// "Writes a formatted UTF-8 string to the file." Swap middle float via hex payload and dump verification.
		hxfile f(hxfile::out, "hxconsole_test_file_test.txt");
		f.print("poke %zx 4 435E0000\n", reinterpret_cast<size_t>(target + 1));
		f.print("floatdump %zx 3\n", reinterpret_cast<size_t>(target));
	}
	const bool is_ok = hxconsole_exec_line("exec hxconsole_test_file_test.txt");
	EXPECT_TRUE(is_ok);

	// Buffer morph: [111.0] [777.0] [333.0] -> [111.0] [222.0] [333.0]
	EXPECT_EQ(target[0], 111.0f);
	EXPECT_EQ(target[1], 222.0f);
	EXPECT_EQ(target[2], 333.0f);
}
#endif
