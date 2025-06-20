// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>

HX_REGISTER_FILENAME_HASH

// hxsettings - g_hxsettings is declared in hxcUtils.c for maximum portability.

namespace {
#if (HX_RELEASE) < 1
// Confirm the correct number of asserts were triggered.
static bool checkasserts(void) {
	int unused_asserts = g_hxsettings.asserts_to_be_skipped;
	g_hxsettings.asserts_to_be_skipped = 0;
	hxassert_msg(unused_asserts == 0, "expected more asserts");
	return unused_asserts == 0;
}

hxconsole_command(checkasserts);
hxconsole_variable_named(g_hxsettings.asserts_to_be_skipped, skipasserts);
#endif

hxconsole_variable_named(g_hxsettings.log_level, loglevel);
} // namespace

void hxsettings_construct() {
	g_hxsettings.log_level = hxlog_level_Log;
	g_hxsettings.deallocate_permanent = false;

#if (HX_RELEASE) < 1
	g_hxsettings.asserts_to_be_skipped = 0;
#endif
}
