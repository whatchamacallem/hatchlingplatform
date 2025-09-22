#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hatchling_pch.hpp Includes some of the Hatchling Platform headers
/// for use building a C++ precompiled header. See `testmatrix.sh`. Use
/// `hx/hatchling.h` for building a C precompiled header.

/// `HX_HATCHLING_PCH_USED` - Allows checking for correct use of -include-pch.
#define HX_HATCHLING_PCH_USED 1

// Tries not to include anything that will unnecessarily slow down the build.
// As adding files to a .pch has a cost too.

// { stdarg.h, stddef.h, stdint.h, stdlib.h, string.h, signal.h,
//   hxsettings.h, hxmemory_manager.h, hxstring_literal_hash.h }
// Optional: <new>
#include "hatchling.h"

#include "hxarray.hpp"    // hxallocator.hpp, Optional: <initializer_list>
#include "hxfile.hpp"
#include "hxhash_table.hpp"
#include "hxhash_table_nodes.hpp"
#include "hxkey.hpp"
#include "hxprofiler.hpp" // Uses platform specific includes.
#include "hxsort.hpp"
#include "hxtest.hpp"     // Optional: Google Test <gtest/gtest.h>
#include "hxthread.hpp"   // Optional: { errno.h, pthread.h }
