#pragma once
// Copyright 2017-2025 Adrian Johnston

// Includes all the Hatchling headers for use building a precompiled header.
// This is C++ and has an appropriate extension for that.  See test.sh for
// Clang usage that does not require -cc1 or -Xclang.

#include <hx/hatchling.h>
#include <hx/hxAllocator.hpp>
#include <hx/hxArray.hpp>
#include <hx/hxConsole.hpp>
#include <hx/hxDma.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxHashTableNodes.hpp>
#include <hx/hxMemoryManager.h>
#include <hx/hxProfiler.hpp>
#include <hx/hxSort.hpp>
#include <hx/hxStringLiteralHash.h>
#include <hx/hxTask.hpp>
#include <hx/hxTaskQueue.hpp>
#include <hx/hxTest.hpp> // May include Google Test.
#include <hx/hxTime.hpp>
