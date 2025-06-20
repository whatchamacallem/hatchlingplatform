#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install musl musl-dev musl-tools

export POSIXLY_CORRECT=1

set -o errexit

HX_RELEASE="-DHX_RELEASE=3"

HX_OPTIMIZATION="-Os -static"

HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

HX_FLAGS="-DHX_USE_CPP_THREADS=0 -g -ffunction-sections -fdata-sections -ffast-math"

# Allow demangled C++ names to pass through awk.
HX_AWK_HACK='{print $3, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18 }'

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

set -x

musl-gcc $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -I$HX_DIR/include \
	-std=c17 -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# Includes lld specific instruction to dead-strip. musl is the only library.
musl-gcc $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -I$HX_DIR/include \
	-std=c++17 -Wl,--gc-sections -fno-exceptions -fno-rtti \
	$HX_DIR/*/*.cpp *.o -o hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +x; } 2> /dev/null

echo ==========================================================================
echo = Largest elf symbols...
echo ==========================================================================
readelf --wide --symbols --demangle hxtest | awk "$HX_AWK_HACK" | sort -nr \
	| grep -v test | head -n 128

echo ==========================================================================
# prints summary stats for the necessary components of the executable.
strip -o hxtest-strip --strip-unneeded hxtest
size hxtest-strip

echo 🐉🐉🐉
