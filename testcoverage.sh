#!/bin/bash
set -o xtrace
set -o errexit

gcc -Iinclude --coverage -O0 -Wall -DHX_RELEASE=0 -std=c99 -c src/*.c test/*.c

g++ -Iinclude --coverage -O0 -Wall -DHX_RELEASE=0 -DHX_USE_CPP_THREADS=1 \
	-DHX_MEM_DIAGNOSTIC_LEVEL=0 -DHX_TEST_ERROR_HANDLING=1 \
	-fno-exceptions -pthread -lpthread -std=c++17 -lstdc++ \
	*/*.cpp *.o -o hxtest

echo runtests | ./hxtest printhashes help execstdin

gcovr --html-details coverage.html

# clean up with:
# git clean -f -d
