# Copyright 2017-2025 Adrian Johnston
#
#!/bin/bash

set -o errexit

# The -m32 switch enables 32-bit compilation.  You will need these packages on Ubuntu:
#   sudo apt-get install gcc-multilib g++-multilib

# c++ warning flags.  preceded by -pedantic-errors.
WARNINGS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter"

# optimization level
I=0

set -o xtrace

gcc -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" \
	-std=c99 -m32 -c src/*.c
gcc -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" -pthread \
	-std=c++14 -fno-exceptions -fno-rtti */*.cpp *.o -lpthread -lstdc++ -m32 -o hxtest

echo $?
