#!/bin/bash
# Copyright 2019 Adrian Johnston
#
# Tests Hatchling Platform with gcc and clang in a variety of configurations.
#
# Adds script args to compiler command line using "$@" so for example calling
#
#   etc/test.sh -DHX_TEST_DIE_AT_THE_END=1
#
# will run the tests with HX_TEST_DIE_AT_THE_END defined to be 1.
set -o errexit

export GREP_COLOR='0;32' # green

# c++ warning flags.  preceded by -pedantic-errors except with c++98.
WARNINGS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter"

# Test gcc with both -std=c++98 and -std=c++14.  Not using -pedantic-errors with
# c++98 as "anonymous variadic macros were introduced in c++11."  (This code base
# and gcc's defaults cheat slightly by pretending c99 was available in c++98.)
# -Wno-unused-local-typedefs is only for the c++98 version of static_assert. 
gcc --version | grep gcc
for I in 0 1 2 3; do
echo gcc c++98 -O$I "$@"
gcc -Iinclude -O$I -Wall -Wextra -Werror -pedantic-errors -DHX_RELEASE=$I "$@" \
	-std=c99 -c src/*.c
gcc -Iinclude -O$I $WARNINGS -DHX_RELEASE=$I "$@" -std=c++98 -fno-exceptions \
	-fno-rtti  -Wno-unused-local-typedefs */*.cpp *.o -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest
echo gcc c++14 -O$I "$@"
gcc -Iinclude -O$I -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" -pthread \
	-std=c++14 -fno-exceptions -fno-rtti */*.cpp *.o -lpthread -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest *.o
done

# Test undefined behavior/address use with clang.
clang --version | grep clang
for I in 0 1 2 3; do
echo clang UBSan -O$I "$@"
clang -Iinclude -O$I -Wall -Wextra -Werror -DHX_RELEASE=$I "$@" \
	-fsanitize=undefined,address -fno-sanitize-recover=undefined,address \
	-std=c99 -c src/*.c
clang -Iinclude -O$I -pedantic-errors $WARNINGS -DHX_RELEASE=$I \
	-DHX_USE_CPP11_THREADS=$I -DHX_USE_CPP11_TIME=$I "$@" -pthread -std=c++14 \
	-fsanitize=undefined,address -fno-sanitize-recover=undefined,address -lubsan \
	*/*.cpp *.o -lpthread -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest *.o
done

# Remove output.
rm log.txt profile.json hxConsoleTest_FileTest.txt hxFileTest_Operators.bin \
	hxFileTest_ReadWrite.txt

echo test.sh passed
