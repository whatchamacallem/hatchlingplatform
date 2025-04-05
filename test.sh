#!/bin/bash
# Copyright 2017-2025 Adrian Johnston
#
# Tests Hatchling Platform with gcc and clang in a variety of configurations.
#
# Adds script args to compiler command line using "$@" so for example calling
#
#   ./test.sh -DHX_TEST_DIE_AT_THE_END=1
#
# will run the tests with HX_TEST_DIE_AT_THE_END defined to be 1.
set -o errexit
set -x

export GREP_COLORS='mt=0;32' # green

# c++ warning flags.  preceded by -pedantic-errors except with c++98.
WARNINGS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter"

# Test undefined behavior/address use with clang. Uses pch.
clang --version | grep clang
for I in 0 1 2 3; do
echo clang UBSan -O$I "$@"...
# compile C
clang -Iinclude -O$I -g $WARNINGS -pedantic-errors -DHX_RELEASE=$I "$@" \
	-fsanitize=undefined,address -fno-sanitize-recover=undefined,address \
	-std=c99 -c src/*.c
# generate pch. clang does this automatically when a c++ header file is the target. 
clang++ -Iinclude -pedantic-errors $WARNINGS -DHX_RELEASE=$I \
	-DHX_USE_CPP11_THREADS=$I -DHX_USE_CPP11_TIME=$I "$@" -std=c++14 \
	-fno-exceptions include/hx/hatchling_pch.hpp -o hatchling_pch.hpp.pch
# compile C++ and link
clang++ -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I \
	-DHX_USE_CPP11_THREADS=$I -DHX_USE_CPP11_TIME=$I "$@" -pthread -std=c++14 \
	-fsanitize=undefined,address -fno-sanitize-recover=undefined,address -lubsan \
	-fno-exceptions hatchling_pch.hpp.pch */*.cpp *.o -lpthread -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest *.o
done


# The -m32 switch enables 32-bit compilation.  You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib
#
# Test gcc with both -std=c++98 and -std=c++14.  Not using -pedantic-errors with
# c++98 as "anonymous variadic macros were introduced in c++11."  (This code base
# and gcc's defaults cheat slightly by pretending c99 was available in c++98.)
# -Wno-unused-local-typedefs is only for the c++98 version of static_assert. 
gcc --version | grep gcc
for I in 0 1 2 3; do
echo gcc c++98 -O$I "$@"...
gcc -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" \
	-std=c99 -m32 -c src/*.c
gcc -Iinclude -O$I -g $WARNINGS -DHX_RELEASE=$I "$@" -std=c++98 -fno-exceptions \
	-fno-rtti -Wno-unused-local-typedefs */*.cpp *.o -lstdc++ -m32 -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest
echo gcc c++14 -O$I "$@"...
gcc -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" -pthread \
	-std=c++14 -fno-exceptions -fno-rtti */*.cpp *.o -lpthread -lstdc++ -m32 -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest *.o
done

# Remove output.
rm profile.json hxConsoleTest_FileTest.txt hxFileTest_Operators.bin \
	hxFileTest_ReadWrite.txt

echo all tests passed.
