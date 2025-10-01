#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# This build uses 32-bit pointers because they are easier to read.
#
# The -m32 switch enables 32-bit compilation. See ubuntu_packages.sh.
#
# Do not use a .pch with ccache. It won't work as expected.

set -o errexit

export POSIXLY_CORRECT=1

# Should detect threading and standard library.
BUILD="-DHX_RELEASE=0 -O0"

# -Wdate-time is for ccache.
ERRORS="-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual \
	-Wdisabled-optimization -Wshadow -Wundef -Wconversion -Wdate-time \
	-Wmissing-declarations"

FLAGS="-m32 -ggdb3 -fdiagnostics-absolute-paths -fdiagnostics-color=always"

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

for FILE in ../src/*.c ../test/*.c; do
	ccache clang $BUILD $ERRORS $FLAGS -I../include \
		-std=c17 -pthread -c $FILE & PIDS="$PIDS $!"
done

for FILE in ../src/*.cpp ../test/*.cpp; do
	ccache clang++ $BUILD $ERRORS $FLAGS -I../include \
		-std=c++20 -pthread -fno-exceptions -fno-rtti  \
		-c $FILE & PIDS="$PIDS $!"
done

for PID in $PIDS; do
	if ! wait "$PID"; then
		exit 1
	fi
done

ccache clang++ $BUILD $FLAGS *.o -lpthread -lstdc++ -lm -o hxtest

echo 🐉🐉🐉
