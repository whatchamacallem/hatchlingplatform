#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# Tests Hatchling Platform with gcc and clang in a variety of configurations.
# Tests C99, C17, C++98, C++11 and C++17.
#
# Adds script args to compiler command line using "$@" so for example calling
#
#   ./test.sh -DHX_TEST_ERROR_HANDLING=1
#
# will run the tests with HX_TEST_ERROR_HANDLING defined to be 1.

export POSIXLY_CORRECT=1

set -o errexit

# Fatal warning flags. preceded by -pedantic-errors except with c++98.
HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-Wfatal-errors"

HX_FLAGS="-ffast-math -ggdb3"

HX_SANITIZE="-fsanitize=undefined,address -fsanitize-recover=undefined,address"

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin
mkdir ./bin
cd ./bin

# The -m32 switch enables 32-bit compilation. You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib
#
# Test gcc with both -std=c++98 and -std=c++14. Not using -pedantic-errors with
# c++98 as "anonymous variadic macros were introduced in c++11."  (This code base
# and gcc's defaults cheat slightly by pretending c99 was available in c++98.)
# -Wno-unused-local-typedefs is only for the c++98 version of static_assert.

gcc --version | grep gcc
for I in 0 1 2 3; do
echo gcc c++98 -O$I "$@"...
# -std=c99
gcc -I$HX_DIR/include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -pedantic-errors \
	-std=c99 -m32 "$@" -c $HX_DIR/src/*.c $HX_DIR/test/*.c
# -std=c++98
gcc -I$HX_DIR/include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -std=c++98 \
	-fno-exceptions -fno-rtti -Wno-unused-local-typedefs "$@" $HX_DIR/*/*.cpp *.o \
	-lstdc++ -m32 -o hxtest
./hxtest runtests | grep '\[  PASSED  \]' || ./hxtest runtests
rm -f hxtest *.txt *.bin *.json
echo gcc c++14 -O$I "$@"...
# -std=c++14
gcc -I$HX_DIR/include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -pedantic-errors \
	-pthread -std=c++14 -fno-exceptions -fno-rtti "$@" $HX_DIR/*/*.cpp *.o \
	-lpthread -lstdc++ -m32 -o hxtest
./hxtest runtests | grep '\[  PASSED  \]' || ./hxtest runtests
rm -f hxtest *.o *.txt *.bin *.json
done

# Test undefined behavior/address use with clang. Uses pch and allows exceptions
# just to make sure there are none.
clang --version | grep clang
for I in 0 1 2 3; do
echo clang UBSan -O$I "$@"...
# compile C
clang -I../include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -pedantic-errors \
	-fdiagnostics-absolute-paths -std=c17 $HX_SANITIZE "$@" -c ../src/*.c ../test/*.c
# generate pch. clang does this automatically when a c++ header file is the target.
clang++ -I../include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -pedantic-errors \
	-DHX_USE_CPP_THREADS=$I -pthread -std=c++17 -fno-exceptions -fdiagnostics-absolute-paths \
	$HX_SANITIZE "$@" ../include/hx/hatchlingPch.hpp -o hatchlingPch.hpp.pch
# compile C++ and link
clang++ -I../include -DHX_RELEASE=$I -O$I $HX_FLAGS $HX_ERRORS -pedantic-errors \
	-DHX_USE_CPP_THREADS=$I -pthread -std=c++17 -fno-exceptions -fdiagnostics-absolute-paths \
	$HX_SANITIZE "$@" -include-pch hatchlingPch.hpp.pch ../*/*.cpp *.o \
	-lpthread -lstdc++ -o hxtest
./hxtest runtests | grep '\[  PASSED  \]' || ./hxtest runtests
rm -f hxtest *.o *.txt *.bin *.json
done

# Make sure the script returns 0.
echo 🐉🐉🐉 all tests passed.
