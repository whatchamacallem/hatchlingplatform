// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>

HX_REGISTER_FILENAME_HASH

// hxsettings - g_hxsettings is declared in hxc_utils.c for maximum portability.

namespace hxx_ {
#if (HX_RELEASE) < 1
// Confirm the correct number of asserts were triggered and consume the remaining
// allowance.
static bool checkasserts(void) {
	int unused_asserts = g_hxsettings.asserts_to_be_skipped;
	g_hxsettings.asserts_to_be_skipped = 0;
	return unused_asserts == 0;
}

hxconsole_command(checkasserts);
hxconsole_variable_named(g_hxsettings.asserts_to_be_skipped, skipasserts);
#endif

hxconsole_variable_named(g_hxsettings.log_level, loglevel);
} // hxx_

void hxsettings_construct(void) {
	g_hxsettings.log_level = hxloglevel_log;
	g_hxsettings.deallocate_permanent = false;

#if (HX_RELEASE) < 1
	g_hxsettings.asserts_to_be_skipped = 0;
#endif
}
