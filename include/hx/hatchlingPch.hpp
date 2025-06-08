#pragma once
// Copyright 2017-2025 Adrian Johnston

// Includes all the Hatchling headers for use building a C++ precompiled header.
// Use hx/hatchling.h for building a C precompiled header.
// See test.sh. Set gcc/clangs target to a .pch file e.g. -o "sdks.pch".

#define HX_HATCHLING_PCH_USED 1

#include <hx/hatchling.h>
#include <hx/hxAllocator.hpp>
#include <hx/hxArray.hpp>
#include <hx/hxConsole.hpp>
#include <hx/hxDma.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxHashTableNodes.hpp>
#include <hx/hxKey.hpp>
#include <hx/hxMemoryManager.h>
#include <hx/hxProfiler.hpp>
#include <hx/hxSort.hpp>
#include <hx/hxStringLiteralHash.h>
#include <hx/hxTask.hpp>
#include <hx/hxTaskQueue.hpp>
#include <hx/hxTest.hpp> // May include Google Test.
