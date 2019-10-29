#!/bin/bash
#set -o xtrace
set -o errexit

# Will re-run failing tests without grep and then exit.
export GREP_COLOR='0;32'

# Testing Hatchling Platform...
echo
echo "$(tput bold)Testing Hatchling Platform...$(tput sgr 0)"
echo

# test gcc with both -std=c++98 and -std=c++14
gcc --version | grep gcc
for i in 0 1 2 3; do
echo gcc c++98 -O$i
gcc -Iinclude -O$i -Wall -pedantic-errors -Werror -DHX_RELEASE=$i -c src/hxCUtils.c
gcc -Iinclude -O$i -Wall -Werror -DHX_RELEASE=$i -std=c++98 -fno-exceptions -fno-rtti -Wno-unused-local-typedefs */*.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest
echo gcc c++14 -O$i
gcc -Iinclude -O$i -Wall -pedantic-errors -Werror -DHX_RELEASE=$i -pthread -lpthread -std=c++14 -fno-exceptions -fno-rtti -Wno-unused-local-typedefs */*.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest hxCUtils.o
done

# test undefined behavior/address use with clang
clang --version | grep clang
for i in 0 1 2 3; do
echo clang UBSan -O$i
clang -Iinclude -O$i -Wall -Werror -DHX_RELEASE=$i -fsanitize=undefined,address -fno-sanitize-recover=undefined,address -c src/hxCUtils.c
clang -Iinclude -O$i -Wall -Werror -DHX_RELEASE=$i -DHX_HAS_CPP11_THREADS=$i -DHX_HAS_CPP11_TIME=$i -pthread -lpthread -std=c++14 -lubsan -fsanitize=undefined,address -fno-sanitize-recover=undefined,address -Wno-unused-local-typedefs */*.cpp hxCUtils.o -lstdc++ -o hxtest
./hxtest | grep '\[  PASSED  \]' --color || ./hxtest
rm hxtest hxCUtils.o
done

# remove non-deterministic output
rm log.txt profile.json

# confirm working directory is clean
if [[ $(git status --porcelain) ]]; then
    echo "unexpected files:"
	git status --porcelain
	exit 1
fi

# ALL_PASSED
echo 
echo "$(tput setaf 2)$(tput bold)ALL_PASSED$(tput sgr 0)"
echo 
