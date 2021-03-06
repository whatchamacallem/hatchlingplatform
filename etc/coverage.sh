#!/bin/bash
set -o xtrace
set -o errexit

gcc -Iinclude -coverage -Og -Wall -DHX_RELEASE=0 -Dhxvsnprintf=vsnprintf -std=c99 -c src/*.c

g++ -Iinclude -coverage -Og -Wall -DHX_RELEASE=0 -DHX_USE_CPP11_THREADS=1 \
	-DHX_USE_CPP11_TIME=1 -DHX_MEM_DIAGNOSTIC_LEVEL=0 -DHX_TEST_DIE_AT_THE_END=1 \
	-pthread -lpthread -std=c++11 -lstdc++ -Wno-unused-local-typedefs \
	-Dhxvsnprintf=vsnprintf */*.cpp *.o -o hxtest

./hxtest

gcov -o . */*.cpp src/*.c

bash <(curl -s https://codecov.io/bash)

# clean up with:
# git clean -f -d
