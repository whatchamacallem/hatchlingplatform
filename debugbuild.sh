#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# The -m32 switch enables 32-bit compilation. You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib

set -o errexit

HX_RELEASE="-DHX_RELEASE=0"

# Compiler optimization level. Allows a fast debug build.
HX_OPTIMIZATION="-O0"

HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

HX_FLAGS="-m32 -ffast-math -ggdb3"

set -x

clang $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c17 -c src/*.c test/*.c

# Make a pch just in case it helps.
clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c++17 -pthread -fno-exceptions -fno-rtti include/hx/hatchlingPch.hpp \
	-o hatchlingPch.hpp.pch

clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS $HX_FLAGS -Iinclude \
	-std=c++17 -pthread -fno-exceptions -fno-rtti -include-pch hatchlingPch.hpp.pch \
	*/*.cpp *.o -lpthread -lstdc++ -lm -o hxtest

# turn off tracing silently and make sure the command returns 0.
{ set +x; } 2> /dev/null
echo 🐉🐉🐉
