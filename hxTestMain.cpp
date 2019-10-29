// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxTest.h"
#include "hxConsole.h"

HX_REGISTER_FILENAME_HASH;

// HX_HOST may be configured to use GoogleTest instead.
#if !(HX_GOOGLE_TEST)

// Ensures constructor runs before tests are registered by global constructors.
hxTestRunner& hxTestRunner::get() {
	static hxTestRunner s_hxTestRunner;
	return s_hxTestRunner;
}

void hxTestMain() {
	hxInit();
	// g_hxSettings may have a compiler generated default constructor clearing it to
	// zero.  Or something much much worse is going on.
	hxAssertRelease(g_hxSettings.settingsIntegrityCheck == hxSettings::c_settingsIntegrityCheck, "g_hxSettings overwritten");

	const char bytes[] = "The quick brown fox jumps over the lazy dog. "
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"1234567890qwertyuiopasdfghjklzxcvbnm"
		"123456";

	uint32_t hash1 = hxHashStringLiteral("");
	uint32_t hash2 = hxHashStringLiteral("abc");
	uint32_t hash3 = hxHashStringLiteral(bytes);

	hxAssertRelease(hash1 == hxHashStringLiteralDebug(""), "hxHashStringLiteral mismatch"); (void)hash1;
	hxAssertRelease(hash2 == hxHashStringLiteralDebug("abc"), "hxHashStringLiteral mismatch"); (void)hash2;
	hxAssertRelease(hash3 == hxHashStringLiteralDebug(bytes), "hxHashStringLiteral mismatch"); (void)hash3;

	hxHexDump(bytes, 64, "hex dumping string");

	const float floats[] = { 0.0f, 1.0f, 2.0f };
	hxFloatDump(floats, sizeof(floats) / sizeof(float), "{ 0.0f, 1.0f, 2.0f }");

	hxConsoleHelp();

	// Filter using e.g. hxTestRunner::get().setFilterStaticString("hxArrayTest").
	// All tests already registered by global constructors.
	hxTestRunner::get().executeAllTests();

#if (HX_RELEASE) < 3
	hxShutdown();
#endif
}

extern "C"
int main() {
	hxTestMain();
	return 0;
}

#endif // !HX_GOOGLE_TEST
