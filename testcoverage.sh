#!/bin/bash
set -o xtrace
set -o errexit

gcc -Iinclude --coverage -Og -Wall -DHX_RELEASE=0 -std=c99 -c src/*.c test/*.c

g++ -Iinclude --coverage -Og -Wall -DHX_RELEASE=0 -DHX_USE_CPP_THREADS=1 \
	-DHX_USE_CHRONO=1 -DHX_MEM_DIAGNOSTIC_LEVEL=0 -DHX_TEST_DIE_AT_THE_END=1 \
	-fno-exceptions -pthread -lpthread -std=c++11 -lstdc++ \
	*/*.cpp *.o -o hxtest

./hxtest

gcovr --html-details coverage.html

# clean up with:
# git clean -f -d
