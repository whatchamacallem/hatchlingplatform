#!/bin/bash
set -o xtrace
set -o errexit

gcc -Iinclude -coverage -O0 -Wall -DHX_RELEASE=0 -std=c99 -c src/*.c
g++ -Iinclude -coverage -O0 -Wall -DHX_RELEASE=0 -DHX_USE_CPP11_THREADS=1 -DHX_USE_CPP11_TIME=1 -pthread -lpthread -std=c++11 -Wno-unused-local-typedefs */*.cpp *.o -lstdc++ -o hxtest
./hxtest
gcov -o . */*.c */*.cpp
bash <(curl -s https://codecov.io/bash)

# clean up with:
# git clean -f -d
