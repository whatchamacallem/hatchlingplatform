// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// Include everything first to catch conflicts.
#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp> // May include Google Test.

#include "hxctest.h"

HX_REGISTER_FILENAME_HASH

int hxtest_main(int argc, char**argv);

static bool hxrun_all_tests(void) {
	hxlogconsole("hatchling platform 🐉🐉🐉 " HATCHLING_TAG " %d\n", g_hxinit_ver_);
	hxlogconsole("release: %d profile: %d\n", (int)(HX_RELEASE), (int)(HX_PROFILE));

	// RUN_ALL_TESTS is a Google Test symbol.
	const size_t tests_failing = (size_t)RUN_ALL_TESTS();

#if HX_TEST_ERROR_HANDLING
	// The 4 in hxtest_test and one in the console tests.
	const int s_hxexpected_failures = 5;

	hxassertrelease(tests_failing == s_hxexpected_failures,
		"unexpected_failures expected %d tests to fail", s_hxexpected_failures);
	// There are no asserts at level 3.
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

// Command line parameter to run all tests.
hxconsole_command_named(hxrun_all_tests, runtests);

// Command line parameter to execute stdin.
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
