#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# sudo apt install musl musl-dev musl-tools

set -o errexit

export POSIXLY_CORRECT=1

BUILD="-DHX_RELEASE=3 -Os -static"

ERRORS="-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual \
	-Wdisabled-optimization -Wshadow -Wundef -Wconversion -Wdate-time \
	-Wmissing-declarations"

FLAGS="-DHX_USE_THREADS=1 -g -ffunction-sections -fdata-sections -ffast-math"

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

set -o xtrace

musl-gcc $BUILD $ERRORS $FLAGS -I$HX_DIR/include \
	-std=c17 -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# Includes lld specific instruction to dead-strip. musl is the only library.
musl-gcc $BUILD $ERRORS $FLAGS -I$HX_DIR/include \
	-std=c++17 -Wl,--gc-sections -fno-exceptions -fno-rtti \
	$HX_DIR/src/*.cpp $HX_DIR/test/*.cpp *.o -o hxtest

strip -o hxtest-strip --strip-unneeded hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +o xtrace; } 2> /dev/null

# Prints [  PASSED  ]
./hxtest-strip

cd ..

echo ==========================================================================
echo = Largest elf symbols...
echo ==========================================================================
./listsymbols.sh

echo ==========================================================================
# prints summary stats for the necessary components of the executable.
size bin/hxtest-strip

echo 🐉🐉🐉
