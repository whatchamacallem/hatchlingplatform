# Copyright 2017-2025 Adrian Johnston
#
#!/bin/bash

set -o errexit

# DHX_RELEASE level. Allows an unoptimized release build for debugging.
HX_RELEASE="-DHX_RELEASE=0 -m32"

# Compiler optimization level. Allows a fast debug build.
HX_OPTIMIZATION=-O0

# The -m32 switch enables 32-bit compilation.  You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib

# c++ warning flags.
HX_ERRORS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter \
	-pedantic-errors -Wfatal-errors"

set -o xtrace

# leave floating point exceptions enabled and provide full gdb info.
clang $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS -Iinclude -ggdb3 \
	-std=c17 -m32 -c src/*.c test/*.c

# make a pch just in case it helps
clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS -Iinclude -ggdb3 \
	-std=c++17 -m32 -pthread -fno-exceptions -fno-rtti include/hx/hatchlingPch.hpp \
	-o hatchlingPch.hpp.pch

clang++ $HX_RELEASE $HX_OPTIMIZATION $HX_ERRORS -Iinclude -ggdb3 \
	-std=c++17 -m32 -pthread -fno-exceptions -fno-rtti -include-pch hatchlingPch.hpp.pch \
	*/*.cpp *.o -lpthread -lstdc++ -lm -o hxtest
