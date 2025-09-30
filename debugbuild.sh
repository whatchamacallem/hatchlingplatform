#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# This build uses 32-bit pointers because they are easier to read.
#
# The -m32 switch enables 32-bit compilation. You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib gdb-multiarch
#
# Do not use a .pch with ccache. It won't work as expected.

set -o errexit

export POSIXLY_CORRECT=1

# Usage: wait_or_exit <pid>...
wait_or_exit() {
    for pid in "$@"; do
        if ! wait "$pid"; then
            exit 1
        fi
    done
}

RELEASE="-DHX_RELEASE=0"

# Compiler optimization level. Allows a fast debug build.
OPTIMIZATION="-O0"

ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

FLAGS="-m32 -ggdb3 -fdiagnostics-absolute-paths -fdiagnostics-color=always"

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

ccache clang $RELEASE $OPTIMIZATION $ERRORS $FLAGS -I../include \
	-std=c17 -c ../src/*.c ../test/*.c & PIDS="$!"

for FILE in ../*/*.cpp; do
	ccache clang++ $RELEASE $OPTIMIZATION $ERRORS $FLAGS -I../include \
		-std=c++20 -pthread -fno-exceptions -fno-rtti  \
		-c $FILE & PIDS="$PIDS $!"
done

wait_or_exit $PIDS

ccache clang++ $RELEASE $OPTIMIZATION $ERRORS $FLAGS -I../include \
	-std=c++20 -pthread -fno-exceptions -fno-rtti \
	*.o -lpthread -lstdc++ -lm -o hxtest

echo 🐉🐉🐉
