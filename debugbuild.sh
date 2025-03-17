#!/bin/bash
# Copyright 2025 Adrian Johnston

set -o errexit

# c++ warning flags.  preceded by -pedantic-errors.
WARNINGS="-Wall -Wextra -Werror -Wcast-qual -Wdisabled-optimization -Wshadow \
	-Wwrite-strings -Wundef -Wendif-labels -Wstrict-overflow=1 -Wunused-parameter"

I=0

set -o xtrace

gcc -Iinclude -O$I -g -Wall -Wextra -Werror -pedantic-errors -DHX_RELEASE=$I "$@" \
	-std=c99 -c src/*.c
gcc -Iinclude -O$I -g -pedantic-errors $WARNINGS -DHX_RELEASE=$I "$@" -pthread \
	-std=c++14 -fno-exceptions -fno-rtti */*.cpp *.o -lpthread -lstdc++ -o hxtest

echo $?
