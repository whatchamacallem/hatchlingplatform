// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// Include everything first to catch conflicts.
#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp> // May include Google Test.

#include "hxctest.h"

// Nothing else in hx depends on this.
#include <ctype.h>

HX_REGISTER_FILENAME_HASH

int hxtest_main(int argc, char**argv);

// Run all the C tests.
TEST(hxctest, all_tests) {
	EXPECT_TRUE(hxctest_all());
}

// Make sure new and delete plausibly exist. Make sure hxnullptr compiles.
TEST(hxnew, smoketest) {
	unsigned int* t = new unsigned int(3);
	ASSERT_TRUE(t);
	hxassertrelease(t, "new"); // Should be impossible.
	*t = 0xdeadbeefu;
	delete t;
	t = hxnullptr;
	ASSERT_FALSE(t);
}

// These two tests test the test framework by failing.
#if HX_TEST_ERROR_HANDLING
TEST(death_test, fail) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	for(int i = 10; i--;) {
		FAIL() << "this message is intentionally blank.\n";
	}
	SUCCEED();
}
TEST(death_test, nothing_asserted) {
	hxlog("EXPECTING_TEST_FAILURE\n");
}
#endif

TEST(hxisspace, compare_with_standard) {
	// Don't use non-ASCII or setlocale because it might not exist.

	for (int c = 0; c < 128; ++c) {
		const bool hx = hxisspace((char)c);
		const bool st = ::isspace((unsigned char)c) != 0;
		EXPECT_EQ(hx, st);
	}
	for (int c = 128; c < 256; ++c) {
		const bool hx = hxisspace((char)c);
		EXPECT_EQ(hx, false);
	}
}

TEST(hxisgraph, compare_with_standard) {
	// Don't use non-ASCII or setlocale because it might not exist.

	for (int c = 0; c <= 255; ++c) {
		const bool hx = hxisgraph((char)c);

		const bool expected = (c >= 0x21 && c <= 0x7E) || (c >= 0x80);
		EXPECT_EQ(hx, expected);

		if (c < 0x80) {
			const bool st = ::isgraph(c) != 0;
			EXPECT_EQ(hx, st);
		}
	}
}

static bool hxrun_all_tests(void) {
	hxlogconsole("hatchling platform 🐉🐉🐉 " HATCHLING_TAG "\n");
	hxlogconsole("release: %d profile: %d\n", (int)(HX_RELEASE), (int)(HX_PROFILE));

	// RUN_ALL_TESTS is a Google Test symbol.
	size_t tests_failing = (size_t)RUN_ALL_TESTS();

#if HX_TEST_ERROR_HANDLING
	// The 2 above and one in the console tests.
	const int s_hxexpected_failures = 3;

	hxassertrelease(tests_failing == s_hxexpected_failures,
		"unexpected_failures expected %d tests to fail", s_hxexpected_failures);
	// there are no asserts at level 3.
	if(tests_failing == s_hxexpected_failures) {
		hxloghandler(hxloglevel_warning,
			"expected_failures Expected exactly %d tests to fail...", s_hxexpected_failures);
	}
	return tests_failing == s_hxexpected_failures;
#else
	return tests_failing == 0;
#endif
}

static bool hxexecute_stdin(void) {
	return hxconsole_exec_file(hxin);
}

// Comand line parameter to run all tests.
hxconsole_command_named(hxrun_all_tests, runtests);

// Comand line parameter to execute stdin.
hxconsole_command_named(hxexecute_stdin, execstdin);

// hxtest_main - Command line console command dispatcher. Each parameter is treated
// as a separate command.
int hxtest_main(int argc, char**argv) {
	hxinit();

	bool is_ok = true;
	if(argc > 1) {
		for(int i=1; i<argc; ++i) {
			is_ok = is_ok && hxconsole_exec_line(argv[i]);
		}
	}
	else {
		hxrun_all_tests();
	}

	// Logging and asserts are actually unaffected by a shutdown.
#if (HX_RELEASE) < 3
	hxshutdown();
#endif
	return is_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

// main - calls hxtest_main.
int main(int argc, char**argv) {
	testing::InitGoogleTest(&argc, argv);

#if (HX_USE_GOOGLE_TEST) && (HX_RELEASE) == 0
	GTEST_FLAG_SET(break_on_failure, true);
#endif

	return hxtest_main(argc, argv);
}
