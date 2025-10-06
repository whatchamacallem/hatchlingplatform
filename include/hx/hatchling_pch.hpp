#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hatchling_pch.hpp Includes a subset of the Hatchling Platform
/// headers for building a C++ precompiled header. See `testmatrix.sh`. Use
/// `hx/hatchling.h` to build a C precompiled header.

// Tries not to include anything that will unnecessarily slow down the build,
// because adding files to a `.pch` incurs a cost as well.

// { stdarg.h, stddef.h, stdint.h, stdlib.h, string.h, signal.h,
//   hxsettings.h, hxmemory_manager.h, hxstring_literal_hash.h }
// Optional: <new>
#include "hatchling.h"

#include "hxarray.hpp"    // hxallocator.hpp, Optional: <initializer_list>
#include "hxfile.hpp"
#include "hxhash_table.hpp"
#include "hxhash_table_nodes.hpp"
#include "hxkey.hpp"
#include "hxprofiler.hpp" // Uses platform-specific includes.
#include "hxsort.hpp"
#include "hxtest.hpp"     // Optional: Google Test <gtest/gtest.h>
#include "hxthread.hpp"   // Optional: { errno.h, pthread.h }
