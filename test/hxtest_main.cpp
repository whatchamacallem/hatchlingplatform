// Copyright 2017-2025 Adrian Johnston

// Include everything first to catch conflicts.
#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxtest.hpp> // May include Google Test.

#include "hxctest.h"

HX_REGISTER_FILENAME_HASH

// Run all the C tests.
TEST(hxctest, all_tests) {
	EXPECT_TRUE(hxctest_all());
}

// These two tests test the test framework by failing.
#if HX_TEST_ERROR_HANDLING
TEST(hxdeath_test, fail) {
	hxlog("EXPECTING_TEST_FAILURE\n");
	SUCCEED();
	for (int i = 10; i--;) {
		FAIL() << "this message is intentionally blank.\n";
	}
}
TEST(hxdeath_test, nothing_asserted) {
	hxlog("EXPECTING_TEST_FAILURE\n");
}
#endif

static bool hxrun_all_tests(void) {
	hxlogconsole("hatchling platform 🐉🐉🐉 " HATCHLING_TAG "\n");
	hxlogconsole("release: %d profile: %d " __DATE__ " " __TIME__ "\n",
		(int)(HX_RELEASE), (int)(HX_PROFILE));
	hxwarnmsg(HX_HATCHLING_PCH_USED, "pch not used");

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
			"expected_failures expected exactly %d tests to fail", s_hxexpected_failures);
	}
	return tests_failing == s_hxexpected_failures;
#else
	return tests_failing == 0;
#endif
}

static bool hxexecute_stdin(void) {
	hxfile f(hxfile::in | hxfile::stdio);
	return hxconsole_exec_file(f);
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
