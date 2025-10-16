// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hatchling.h"

HX_REGISTER_FILENAME_HASH

// The global settings object. Constructed by hxinit after some global
// constructors may have run and before setting up memory management.
// Logging and asserts default to on until construction completes.

struct hxsettings g_hxsettings;

void hxsettings_construct(void) {
	g_hxsettings.log_level = hxloglevel_log;
	g_hxsettings.deallocate_permanent = false;

#if (HX_RELEASE) < 1
	g_hxsettings.asserts_to_be_skipped = 0;
#endif
}
