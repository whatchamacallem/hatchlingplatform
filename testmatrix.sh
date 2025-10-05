#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# Tests Hatchling Platform with gcc and clang in a variety of configurations.
# Tests C99, C17, C++11 and C++17.
#
# The -m32 switch enables 32-bit compilation. You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib
#
# Adds script args to compiler command line using "$@" so for example calling
#
#   ./testmatrix.sh -DHX_TEST_ERROR_HANDLING=1
#
# will run the tests with HX_TEST_ERROR_HANDLING defined to be 1.

set -o errexit

export POSIXLY_CORRECT=1

# Fatal warning flags.
ERRORS="-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual \
	-Wdisabled-optimization -Wshadow -Wundef -Wconversion -Wdate-time \
	-Wmissing-declarations"

FLAGS="-ffast-math -ggdb3"

SANITIZE="-fsanitize=undefined,address -fsanitize-recover=undefined,address"

HX_DIR=`pwd`

run_hxtest() {
	if ./hxtest runtests > console_output.txt 2>&1; then
		grep -e '\[  PASSED  \]' -e '\[  FAILED  \]' -e 'FAILED TESTS' \
			console_output.txt || cat console_output.txt
	else
		cat console_output.txt
		echo error: hxtest non-zero exit.
		exit 1
	fi
}

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

gcc --version | grep gcc
for I in 0 1 2 3; do
echo gcc c99/c++11 -O$I "$@" ...

gcc -I$HX_DIR/include -DHX_RELEASE=$I -O$I $FLAGS $ERRORS \
	-pthread -std=c99 -m32 "$@" -c $HX_DIR/src/*.c $HX_DIR/test/*.c

gcc -I$HX_DIR/include -DHX_RELEASE=$I -O$I $FLAGS $ERRORS \
	-pthread -std=c++11 -fno-exceptions -fno-rtti "$@" $HX_DIR/src/*.cpp \
	$HX_DIR/test/*.cpp *.o -lpthread -lstdc++ -m32 -o hxtest

run_hxtest

rm -f hxtest *.o *.txt *.bin *.json
done

# Test undefined behavior/address use with clang. Uses pch and allows exceptions
# just to make sure there are none.
clang --version | grep clang
for I in 0 1 2 3; do
echo clang c17/c++20 UBSan -O$I "$@" ...
# compile C17
clang -I../include -DHX_RELEASE=$I -O$I $FLAGS $ERRORS -pedantic-errors \
	-fdiagnostics-absolute-paths -pthread -std=c17 $SANITIZE "$@" -c ../src/*.c ../test/*.c
# generate C++17 pch. clang does this automatically when a c++ header file is the target.
clang++ -I../include -DHX_RELEASE=$I -O$I $FLAGS $ERRORS -pedantic-errors \
	-DHX_USE_THREADS=$I -pthread -std=c++20 -fno-exceptions -fdiagnostics-absolute-paths \
	$SANITIZE "$@" ../include/hx/hatchling_pch.hpp -o hatchling_pch.hpp.pch
# compile C++17 and link
clang++ -I../include -DHX_RELEASE=$I -O$I $FLAGS $ERRORS -pedantic-errors \
	-DHX_USE_THREADS=$I -pthread -std=c++20 -fno-exceptions -fdiagnostics-absolute-paths \
	$SANITIZE "$@" -include-pch hatchling_pch.hpp.pch ../src/*.cpp ../test/*.cpp *.o \
	-lpthread -lstdc++ -o hxtest

run_hxtest

rm -f hxtest *.o *.txt *.bin *.json *.pch
done

# Make sure the script returns 0.
echo 🐉🐉🐉 all tests passed.
