#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# This build uses 32-bit pointers because they are easier to read.
#
# The -m32 switch enables 32-bit compilation. You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib gdb-multiarch

set -o errexit

export POSIXLY_CORRECT=1

HX_RELEASE="-DHX_RELEASE=0"

# Compiler optimization level. Allows a fast debug build.
HX_OPTIMIZATION="-O0"

HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

HX_FLAGS="-m32 -ggdb3 -fdiagnostics-absolute-paths -fdiagnostics-color=always"

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

set -o xtrace

clang $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -I../include \
	-std=c17 -c ../src/*.c ../test/*.c

# Make a pch just in case it helps.
clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -I../include \
	-std=c++20 -pthread -fno-exceptions -fno-rtti ../include/hx/hatchling_pch.hpp \
	-o hatchling_pch.hpp.pch

clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -I../include \
	-std=c++20 -pthread -fno-exceptions -fno-rtti -include-pch hatchling_pch.hpp.pch \
	../*/*.cpp *.o -lpthread -lstdc++ -lm -o hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +o xtrace; } 2> /dev/null
echo 🐉🐉🐉
