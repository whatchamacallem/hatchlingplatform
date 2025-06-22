#ifndef HX_HATCHLING_PCH_USED
// Copyright 2017-2025 Adrian Johnston

// hatchling_pch.hpp - Includes all the Hatchling headers for use building a C++
// precompiled header. Use hx/hatchling.h for building a C precompiled header.
// See test.sh.

// HX_HATCHLING_PCH_USED - Non-zero if this header is correctly included before
// the other hx headers.
#define HX_HATCHLING_PCH_USED 1

#include <hx/hatchling.h>
#include <hx/hxallocator.hpp>
#include <hx/hxarray.hpp>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxhash_table_nodes.hpp>
#include <hx/hxkey.hpp>
#include <hx/hxmemory_manager.h>
#include <hx/hxprofiler.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxstring_literal_hash.h>
#include <hx/hxtask.hpp>
#include <hx/hxtask_queue.hpp>
#include <hx/hxtest.hpp> // May include Google Test.
#include <hx/hxthread.hpp>

#endif // HX_HATCHLING_PCH_USED
