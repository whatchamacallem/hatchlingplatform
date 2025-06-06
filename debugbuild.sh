# Copyright 2017-2025 Adrian Johnston
#
#!/bin/bash

set -o errexit

# The -m32 switch enables 32-bit compilation.  You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib

# c++ warning flags.  preceded by -pedantic-errors.
WARNINGS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter"

# DHX_RELEASE level and optimization level.
I=0

set -o xtrace

# leave floating point exceptions enabled and provide full gdb info.
gcc -Iinclude -O$I -ggdb3 -pedantic-errors -Wfatal-errors $WARNINGS -DHX_RELEASE=$I "$@" \
	-std=c99 -m32 -c src/*.c test/*.c
gcc -Iinclude -O$I -ggdb3 -pedantic-errors -Wfatal-errors $WARNINGS -DHX_RELEASE=$I "$@" -pthread \
	-std=c++17 -fno-exceptions -fno-rtti */*.cpp *.o -lpthread -lstdc++ -lm -m32 -o hxtest

echo $?
