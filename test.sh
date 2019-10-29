#!/bin/bash
set -o xtrace
set -o errexit

# test gcc with both -std=c++98 and -std=c++14
for i in 0 1 2 3; do
gcc -O3 -Wall -Werror -DHX_RELEASE=$i -c hxCUtils.c
gcc -O3 -Wall -Werror -DHX_RELEASE=$i -std=c++98 -fno-exceptions -fno-rtti -Wno-unused-local-typedefs *.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest
gcc -O3 -Wall -Werror -DHX_RELEASE=$i -pthread -lpthread -std=c++14 -fno-exceptions -fno-rtti -Wno-unused-local-typedefs *.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest
done

# test undefined behavior/address use with clang
for i in 0 1 2 3; do
clang -O3 -Wall -Werror -DHX_RELEASE=$i -fsanitize=undefined,address -fno-sanitize-recover=undefined,address -c hxCUtils.c
clang -O3 -Wall -Werror -DHX_RELEASE=$i -DHX_HAS_CPP11_THREADS=$i -DHX_HAS_CPP11_TIME=$i -pthread -lpthread -std=c++14 -lubsan -fsanitize=undefined,address -fno-sanitize-recover=undefined,address -Wno-unused-local-typedefs *.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest
done

rm -f hxtest hxCUtils.o

gcc --version | grep gcc
clang --version | grep clang
