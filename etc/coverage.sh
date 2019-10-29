#!/bin/bash
set -o xtrace
set -o errexit

# coverage for C is 100%. Coverage for printf is here: https://codecov.io/gh/mpaland/printf
gcc -Iinclude -O0 -Wall -DHX_RELEASE=0 -std=c99 -c */*.c
g++ -Iinclude -coverage -O0 -Wall -DHX_RELEASE=0 -DHX_USE_CPP11_THREADS=1 -DHX_USE_CPP11_TIME=1 -DHX_MEM_DIAGNOSTIC_LEVEL=0 -DHX_TEST_DIE_AT_THE_END -pthread -lpthread -std=c++11 -lstdc++ -Wno-unused-local-typedefs */*.cpp *.o -o hxtest
./hxtest
gcov -o . */*.cpp
bash <(curl -s https://codecov.io/bash)

# clean up with:
# git clean -f -d
