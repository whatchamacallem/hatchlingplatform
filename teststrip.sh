#!/bin/bash
# Copyright 2017-2025 Adrian Johnston

set -o errexit

HX_RELEASE="-DHX_RELEASE=3"

HX_OPTIMIZATION="-O2"

HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

HX_FLAGS="-ffunction-sections -fdata-sections -ffast-math -g"

# Allow demangled C++ names to pass through awk.
HX_AWK_HACK='{print $3, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18 }'

set -x

clang $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c17 -c src/*.c test/*.c

# Make a pch just in case it helps.
clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c++17 -pthread -fno-exceptions -fno-rtti include/hx/hatchlingPch.hpp \
	-o hatchlingPch.hpp.pch

# lld specific instruction to dead-strip.
clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c++17 -Wl,--gc-sections -pthread -fno-exceptions -fno-rtti \
	-include-pch hatchlingPch.hpp.pch \
	*/*.cpp *.o -lpthread -lstdc++ -lm -o hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +x; } 2> /dev/null

echo ==========================================================================
echo = Largest elf symbols...
echo ==========================================================================
readelf --wide --symbols --demangle hxtest | awk "$HX_AWK_HACK" | sort -nr | grep -v Test | head -n 128

echo ==========================================================================
# prints summary stats for the necessary components of the executable.
strip -o hxtest-strip --strip-unneeded hxtest
size hxtest-strip

echo 🐉🐉🐉
